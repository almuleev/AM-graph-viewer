#include "frf_worker.hpp"

namespace lvm {
FrfWorker::~FrfWorker() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
        if (active_cancel_) active_cancel_->store(true);
        pending_.reset();
    }
    ready_.notify_one();
    if (thread_.joinable()) thread_.join();
}
void FrfWorker::submit(FrfInput data, FrfOptions options, std::uint64_t generation) {
    FrfBatchInput batch;
    batch.time=std::move(data.time);
    batch.references.push_back(std::move(data.reference));
    batch.responses.push_back(std::move(data.response));
    submit(std::move(batch),options,generation);
}
void FrfWorker::submit(FrfBatchInput data, FrfOptions options, std::uint64_t generation) {
    auto flag = std::make_shared<std::atomic<bool>>(false);
    std::lock_guard<std::mutex> lock(mutex_);
    if (!thread_.joinable()) thread_ = std::thread(&FrfWorker::run, this);
    if (active_cancel_) active_cancel_->store(true);
    if (pending_) pending_->cancelled->store(true);
    active_cancel_ = flag;
    pending_ = Request{std::move(data), options, generation, flag};
    result_.reset(); ready_.notify_one();
}
void FrfWorker::cancel() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (active_cancel_) active_cancel_->store(true);
    pending_.reset(); result_.reset();
}
std::optional<FrfWorker::Result> FrfWorker::take_result() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto r = std::move(result_); result_.reset(); return r;
}
void FrfWorker::run() {
    for (;;) {
        std::optional<Request> request;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            ready_.wait(lock, [&] { return stopping_ || pending_.has_value(); });
            if (stopping_) return;
            request = std::move(pending_); pending_.reset();
        }
        FrfBatchResult r; r.options=request->options;
        try { r = analyze_frf_batch(std::move(request->data), request->options, request->cancelled.get()); }
        catch (...) { r.error = FrfError::Overflow; }
        std::lock_guard<std::mutex> lock(mutex_);
        if (!stopping_ && !request->cancelled->load()) result_ = Result{request->generation, std::move(r)};
    }
}
} // namespace lvm
