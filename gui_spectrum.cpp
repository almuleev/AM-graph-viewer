// Spectrum: native viewer implementation.
#include "gui_spectrum.hpp"
#include "gui_processing.hpp"
#include "gui_state.hpp"
#include "gui_text.hpp"

namespace gui {

lvm::SpectrumWorker g_spectrum_worker;

bool has_fft_window() {
    return g.fft_window_active && g.fft_window_end > g.fft_window_start;
}

void clear_fft_window() {
    g.fft_window_active = false;
    g.fft_window_start = 0.0;
    g.fft_window_end = 0.0;
}

void clamp_time_window(double& start, double& end) {
    if (start > end) std::swap(start, end);
    start = std::max(start, g.data_t0);
    end = std::min(end, g.data_t1);
}

void set_fft_window(double start, double end) {
    clamp_time_window(start, end);
    if (end <= start) {
        clear_fft_window();
        return;
    }
    g.fft_window_active = true;
    g.fft_window_start = start;
    g.fft_window_end = end;
}

bool current_fft_source_window(double& start, double& end, bool& from_selection) {
    if (!has_data()) return false;
    if (has_fft_window()) {
        start = g.fft_window_start;
        end = g.fft_window_end;
        from_selection = true;
        return true;
    }
    start = g.win_start;
    end = g.win_end;
    clamp_time_window(start, end);
    from_selection = false;
    return end > start;
}

bool fft_window_contains_time(double t) {
    return has_fft_window() && t >= g.fft_window_start && t <= g.fft_window_end;
}

bool last_fft_source_window(double& start, double& end, bool& from_selection) {
    if (!g.spec_source_valid || g.spec_source_end <= g.spec_source_start) return false;
    start = g.spec_source_start;
    end = g.spec_source_end;
    from_selection = g.spec_source_from_selection;
    return true;
}

std::wstring fft_window_status(double start, double end, bool from_selection) {
    wchar_t buf[220];
    if (g_str == &kEn) {
        swprintf(buf, 220, from_selection
            ? L"   |   FFT window: selected %.6g..%.6g s (Shift-click or right-click to clear)"
            : L"   |   FFT window: visible %.6g..%.6g s",
            start, end);
    } else {
        swprintf(buf, 220, from_selection
            ? L"   |   FFT окно: выбранный участок %.6g..%.6g c (Shift-клик или ПКМ для сброса)"
            : L"   |   FFT окно: видимый участок %.6g..%.6g c",
            start, end);
    }
    return buf;
}

std::wstring spectrum_sampling_status(const lvm::Spectrum& spec) {
    if (!spec.ok) return {};
    std::wstring text;
    if (spec.gaps_ignored) {
        wchar_t buf[180];
        swprintf(buf, 180, g_str == &kEn
            ? L"   |   FFT ignored timestamp gaps; all selected samples used"
            : L"   |   FFT без учёта пропусков времени; использованы все выбранные отсчёты");
        text = buf;
    }
    if (spec.resampled) text += g_str == &kEn
        ? L"   |   Uneven timestamps: linear interpolation"
        : L"   |   Неравномерное время: линейная интерполяция";
    return text;
}

void clear_spectrum_cache_state() {
    g_spectrum_worker.cancel();
    ++g.spec_generation;
    g.spec_pending = false;
    g.spec_attempted = false;
    g.spec_valid = false;
    g.spec = lvm::Spectrum{};
    g.spec_channel_indices.clear();
    g.spec_visible_state.clear();
}

void refresh_spec_channel_indices() {
    g.spec_channel_indices.assign(g.spec.names.size(), -1);
    for (std::size_t i = 0; i < g.spec.names.size(); ++i) {
        if (i < g.spec.source_channels.size()) g.spec_channel_indices[i] = static_cast<int>(g.spec.source_channels[i]);
    }
}

void apply_spectrum_result(lvm::Spectrum spectrum) {
    g.spec = std::move(spectrum);
    g.spec_valid = g.spec.ok;
    g.spec_pending = false;
    refresh_spec_channel_indices();
    if (g.spec_valid && g.spec_fit_pending) { g.freq_start = 0; g.freq_end = g.spec.nyquist; }
    g.spec_fit_pending = false;
}

bool spectrum_needs_visible_channels() {
    if (!g.light_mode) return false;
    // This mask describes channels included in the current attempt (including
    // an in-flight request), not the current display. Hiding a channel cannot
    // invalidate its FFT. Keep attempted channels even when they have too few
    // finite values, so repainting does not endlessly retry the same failure.
    for (std::size_t c = 0; c < g.visible.size(); ++c) {
        if (g.visible[c] && (c >= g.spec_visible_state.size() || !g.spec_visible_state[c])) return true;
    }
    return false;
}

bool build_time_window_dataset(const lvm::Dataset& in, double start, double end, lvm::Dataset& out,
                               const std::vector<std::size_t>* selected_channels, bool apply_processing) {
    out = lvm::Dataset{};
    out.stats = in.stats;
    out.frequency_axis = in.frequency_axis;
    out.export_comments = in.export_comments;
    out.ok = true;
    ensure_channel_formula_vectors();
    if (in.time.empty()) return true;

    std::vector<std::size_t> channel_indices;
    if (selected_channels && !selected_channels->empty()) {
        channel_indices = *selected_channels;
    } else {
        channel_indices.resize(in.channels.size());
        for (std::size_t i = 0; i < in.channels.size(); ++i) channel_indices[i] = i;
    }
    out.names.reserve(channel_indices.size());
    out.channels.resize(channel_indices.size());
    for (std::size_t channel_index : channel_indices) {
        if (channel_index < in.names.size()) out.names.push_back(in.names[channel_index]);
        else out.names.push_back("Channel_" + std::to_string(channel_index + 1));
    }

    const std::size_t lo = static_cast<std::size_t>(
        std::lower_bound(in.time.begin(), in.time.end(), start) - in.time.begin());
    const std::size_t hi = static_cast<std::size_t>(
        std::upper_bound(in.time.begin(), in.time.end(), end) - in.time.begin());
    if (lo >= hi) return true;

    out.time.assign(in.time.begin() + static_cast<std::ptrdiff_t>(lo),
                    in.time.begin() + static_cast<std::ptrdiff_t>(hi));
    for (std::size_t out_index = 0; out_index < channel_indices.size(); ++out_index) {
        const std::size_t c = channel_indices[out_index];
        auto& dst = out.channels[out_index];
        dst.reserve(hi - lo);
        const TransformRuntimeKind kind = (c < g.channel_transform_kind.size())
            ? g.channel_transform_kind[c]
            : TransformRuntimeKind::Identity;
        if (!apply_processing || kind == TransformRuntimeKind::Identity) {
            const std::size_t base = dst.size();
            dst.insert(dst.end(),
                       in.channels[c].begin() + static_cast<std::ptrdiff_t>(lo),
                       in.channels[c].begin() + static_cast<std::ptrdiff_t>(hi));
            if (apply_processing && g.noise_threshold_enabled) {
                for (std::size_t i = base; i < dst.size(); ++i) {
                    dst[i] = rendered_channel_sample(c, lo + (i - base));
                }
            }
            continue;
        }
        if (kind == TransformRuntimeKind::Affine) {
            for (std::size_t r = lo; r < hi; ++r) {
                dst.push_back(rendered_channel_sample(c, r));
            }
            continue;
        }
        ensure_transformed_channel_cache(c);
        const auto& cache = g.transformed_channel_cache[c];
        const std::size_t base = dst.size();
        dst.insert(dst.end(),
                   cache.begin() + static_cast<std::ptrdiff_t>(lo),
                   cache.begin() + static_cast<std::ptrdiff_t>(hi));
        if (g.noise_threshold_enabled) {
            for (std::size_t i = base; i < dst.size(); ++i) {
                dst[i] = rendered_channel_sample(c, lo + (i - base));
            }
        }
    }
    return true;
}

void compute_spectrum_for_window(double start, double end, bool from_selection) {
    if (!has_data()) return;
    clamp_time_window(start, end);
    g.spec_fit_pending = g.spec_fit_pending || !g.spec_source_valid || g.spec_source_start != start || g.spec_source_end != end;
    g.spec_source_start = start;
    g.spec_source_end = end;
    g.spec_source_from_selection = from_selection;
    g.spec_source_valid = end > start;
    bool has_visible_channel = false;
    for (char visible : g.visible) {
        if (visible) {
            has_visible_channel = true;
            break;
        }
    }
    if (!has_visible_channel) {
        clear_spectrum_cache_state();
        g.spec_attempted = true;
        g.spec_source_valid = end > start;
        g.spec_visible_state = g.visible;
        return;
    }
    std::vector<std::size_t> visible_channels;
    if (g.light_mode) {
        visible_channels.reserve(g.visible.size());
        for (std::size_t i = 0; i < g.visible.size(); ++i) {
            if (g.visible[i]) visible_channels.push_back(i);
        }
    } else {
        visible_channels.resize(g.ds.channel_count());
        for (std::size_t i = 0; i < visible_channels.size(); ++i) visible_channels[i] = i;
    }
    if (g.light_mode && visible_channels.empty()) {
        clear_spectrum_cache_state();
        g.spec_source_valid = end > start;
        g.spec_visible_state = g.visible;
        return;
    }
    try {
        lvm::Dataset view;
        build_time_window_dataset(g.ds, start, end, view, &visible_channels);
        g.spec_attempted = true;
        if (g.main) {
            g.spec_pending = true;
            g.spec_valid = false;
            g_spectrum_worker.submit(std::move(view), visible_channels, ++g.spec_generation);
        } else {
            auto spectrum = lvm::compute_spectrum(view, 0);
            for (auto& c : spectrum.source_channels) c = visible_channels[c];
            apply_spectrum_result(std::move(spectrum));
        }
    } catch (const std::exception& ex) {
        g.spec = lvm::Spectrum{};
        g.spec.error = ex.what();
        g.spec_valid = false; g.spec_pending = false; g.spec_attempted = true;
    }
    g.spec_visible_state = g.visible;
}

void compute_spectrum_from_current_source() {
    if (!has_data()) return;
    double start = 0.0, end = 0.0;
    bool from_selection = false;
    if (current_fft_source_window(start, end, from_selection)) {
        compute_spectrum_for_window(start, end, from_selection);
    }
}

void compute_spectrum() {
    if (!has_data()) return;
    double start = 0.0, end = 0.0;
    bool from_selection = false;
    if (g.freq_mode && last_fft_source_window(start, end, from_selection)) {
        compute_spectrum_for_window(start, end, from_selection);
        return;
    }
    compute_spectrum_from_current_source();
}

bool ensure_current_spectrum() {
    if (!has_data()) return false;
    if (!g.spec_attempted || spectrum_needs_visible_channels()) compute_spectrum();
    return g.spec_valid;
}

} // namespace gui
