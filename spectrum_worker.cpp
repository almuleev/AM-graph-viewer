#include "spectrum_worker.hpp"

namespace lvm {
SpectrumWorker::~SpectrumWorker() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
        if (active_cancel_) active_cancel_->store(true);
        pending_.reset();
    }
    ready_.notify_one();
    if (thread_.joinable()) thread_.join();
}

void SpectrumWorker::submit(Dataset data, std::vector<std::size_t> channels, std::uint64_t generation) {
    auto flag = std::make_shared<std::atomic<bool>>(false);
    std::lock_guard<std::mutex> lock(mutex_);
    if (!thread_.joinable()) thread_ = std::thread(&SpectrumWorker::run, this);
    if (active_cancel_) active_cancel_->store(true);
    if (pending_) pending_->cancelled->store(true);
    active_cancel_ = flag;
    pending_ = Request{std::move(data), std::move(channels), generation, flag};
    result_.reset();
    ready_.notify_one();
}

void SpectrumWorker::cancel() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (active_cancel_) active_cancel_->store(true);
    pending_.reset(); result_.reset();
}

std::optional<SpectrumWorker::Result> SpectrumWorker::take_result() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto result = std::move(result_); result_.reset(); return result;
}

void SpectrumWorker::run() {
    for (;;) {
        std::optional<Request> request;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            ready_.wait(lock, [&] { return stopping_ || pending_.has_value(); });
            if (stopping_) return;
            request = std::move(pending_); pending_.reset();
        }
        Spectrum spectrum;
        try {
            spectrum = compute_spectrum(request->data, 0, request->cancelled.get());
            for (auto& c : spectrum.source_channels) c = request->source_channels.at(c);
        } catch (...) {
            spectrum = Spectrum{}; // Publishing failure requires no error-string allocation.
        }
        std::lock_guard<std::mutex> lock(mutex_);
        if (!stopping_ && !request->cancelled->load()) result_ = Result{request->generation, std::move(spectrum)};
    }
}

} // namespace lvm
