std::wstring channel_display_label(std::size_t ci) {
    if (ci < g.channel_labels.size() && !g.channel_labels[ci].empty()) return g.channel_labels[ci];
    if (ci < g.ds.names.size() && !g.ds.names[ci].empty()) return to_w(g.ds.names[ci]);
    return std::wstring(L"Channel_") + std::to_wstring(ci + 1);
}

std::string current_channel_label(std::size_t ci) {
    return to_acp(channel_display_label(ci).c_str());
}

void invalidate_formula_runtime() {
    g.formula_runtime_dirty = true;
    invalidate_transformed_channel_cache();
    invalidate_filtered_channel_cache();
    invalidate_plot_analysis_cache();
}

void invalidate_formula_runtime_channel(std::size_t channel_index) {
    g.formula_runtime_dirty = true;
    if (channel_index < g.transformed_channel_cache_valid.size()) {
        g.transformed_channel_cache_valid[channel_index] = 0;
    } else {
        invalidate_transformed_channel_cache();
    }
    if (channel_index < g.filtered_channel_cache_valid.size()) {
        g.filtered_channel_cache_valid[channel_index] = 0;
    } else {
        invalidate_filtered_channel_cache();
    }
    invalidate_plot_analysis_cache();
}

void invalidate_transformed_channel_cache() {
    const std::size_t n = g.ds.channel_count();
    g.transformed_channel_cache.resize(n);
    g.transformed_channel_cache_valid.assign(n, 0);
}

void invalidate_filtered_channel_cache() {
    const std::size_t n = g.ds.channel_count();
    g.filtered_channel_cache.resize(n);
    g.filtered_channel_cache_valid.assign(n, 0);
}

void ensure_channel_formula_storage() {
    const std::size_t n = g.ds.channel_count();
    if (g.channel_formulas.size() != n) g.channel_formulas.assign(n, default_channel_formula_text());
    if (g.channel_formula_rpn.size() != n) g.channel_formula_rpn.assign(n, {});
    if (g.channel_formula_identity.size() != n) g.channel_formula_identity.assign(n, 1);
    if (g.channel_transform_kind.size() != n) g.channel_transform_kind.assign(n, TransformRuntimeKind::Identity);
    if (g.channel_transform_mul.size() != n) g.channel_transform_mul.assign(n, 1.0);
    if (g.channel_transform_add.size() != n) g.channel_transform_add.assign(n, 0.0);
    if (g.transformed_channel_cache.size() != n || g.transformed_channel_cache_valid.size() != n) {
        invalidate_transformed_channel_cache();
    }
}

void ensure_channel_formulas_loaded() {
    if (!g.formula_ini_deferred) return;
    g.formula_ini_deferred = false;
    load_channel_formulas_from_ini();
}

void ensure_channel_formula_vectors() {
    const std::size_t n = g.ds.channel_count();
    ensure_channel_formula_storage();
    const bool vectors_ready =
        g.channel_formulas.size() == n &&
        g.channel_formula_rpn.size() == n &&
        g.channel_formula_identity.size() == n &&
        g.channel_transform_kind.size() == n &&
        g.channel_transform_mul.size() == n &&
        g.channel_transform_add.size() == n &&
        g.transformed_channel_cache.size() == n &&
        g.transformed_channel_cache_valid.size() == n;
    if (!g.formula_runtime_dirty && vectors_ready) return;

    ensure_channel_formulas_loaded();
    if (g.global_formula.empty()) g.global_formula = default_channel_formula_text();
    if (g.global_formula_rpn.empty()) {
        std::wstring error;
        compile_formula_rpn(g.global_formula, g.global_formula_rpn, error, g_str == &kEn);
        if (g.global_formula_rpn.empty()) {
            g.global_formula = default_channel_formula_text();
            compile_formula_rpn(g.global_formula, g.global_formula_rpn, error, g_str == &kEn);
        }
    }
    g.global_formula_identity = formula_rpn_is_identity(g.global_formula_rpn);
    const AffineFormulaInfo global_affine = analyze_formula_rpn_affine(g.global_formula_rpn);
    g.global_formula_affine = global_affine.valid;
    g.global_formula_mul = global_affine.valid ? global_affine.mul : 1.0;
    g.global_formula_add = global_affine.valid ? global_affine.add : 0.0;
    bool has_non_identity = !g.global_formula_identity;
    bool has_non_affine = false;
    for (std::size_t i = 0; i < n; ++i) {
        if (g.channel_formulas[i].empty()) g.channel_formulas[i] = default_channel_formula_text();
        if (g.channel_formula_rpn[i].empty()) {
            std::wstring error;
            compile_formula_rpn(g.channel_formulas[i], g.channel_formula_rpn[i], error, g_str == &kEn);
        }
        g.channel_formula_identity[i] = formula_rpn_is_identity(g.channel_formula_rpn[i]) ? 1 : 0;
        const AffineFormulaInfo local_affine = analyze_formula_rpn_affine(g.channel_formula_rpn[i]);
        TransformRuntimeKind kind = TransformRuntimeKind::CachedFormula;
        double mul = 1.0;
        double add = 0.0;
        if (g.global_formula_affine && local_affine.valid) {
            mul = local_affine.mul * g.global_formula_mul;
            add = local_affine.mul * g.global_formula_add + local_affine.add;
            kind = (mul == 1.0 && add == 0.0) ? TransformRuntimeKind::Identity : TransformRuntimeKind::Affine;
        } else if (g.global_formula_identity && g.channel_formula_identity[i]) {
            kind = TransformRuntimeKind::Identity;
        } else {
            has_non_affine = true;
        }
        g.channel_transform_kind[i] = kind;
        g.channel_transform_mul[i] = mul;
        g.channel_transform_add[i] = add;
        if (kind != TransformRuntimeKind::Identity) has_non_identity = true;
    }
    g.has_non_identity_formula = has_non_identity;
    g.has_non_affine_formula = has_non_affine;
    g.formula_runtime_dirty = false;
}

void ensure_transformed_channel_cache(std::size_t channel_index) {
    ensure_channel_formula_vectors();
    if (channel_index >= g.ds.channel_count()) return;
    if (channel_index >= g.channel_transform_kind.size() ||
        g.channel_transform_kind[channel_index] != TransformRuntimeKind::CachedFormula) {
        return;
    }
    if (channel_index >= g.transformed_channel_cache_valid.size()) invalidate_transformed_channel_cache();
    if (g.transformed_channel_cache_valid[channel_index]) return;

    const auto& src = g.ds.channels[channel_index];
    auto& dst = g.transformed_channel_cache[channel_index];
    dst.resize(src.size());
    for (std::size_t i = 0; i < src.size(); ++i) {
        dst[i] = transform_channel_value(channel_index, src[i]);
    }
    g.transformed_channel_cache_valid[channel_index] = 1;
}

double transform_channel_value(std::size_t ci, double raw) {
    if (std::isnan(raw)) return raw;
    if (!g.has_non_identity_formula) return raw;
    if (ci >= g.channel_transform_kind.size()) return raw;
    const TransformRuntimeKind kind = g.channel_transform_kind[ci];
    if (kind == TransformRuntimeKind::Identity) return raw;
    if (kind == TransformRuntimeKind::Affine) {
        return raw * g.channel_transform_mul[ci] + g.channel_transform_add[ci];
    }
    const double global_value = g.global_formula_identity
        ? raw
        : (g.global_formula_affine
            ? (raw * g.global_formula_mul + g.global_formula_add)
            : eval_formula_rpn(g.global_formula_rpn, raw));
    const double base = std::isfinite(global_value) ? global_value : raw;
    if (ci < g.channel_formula_identity.size() && g.channel_formula_identity[ci]) return base;
    const double value = eval_formula_rpn(g.channel_formula_rpn[ci], base);
    return std::isfinite(value) ? value : raw;
}

std::wstring format_edit_number(double value) {
    wchar_t buf[64];
    swprintf(buf, 64, L"%.12g", value);
    return buf;
}

std::wstring format_optional_edit_number(double value) {
    if (!std::isfinite(value)) return L"";
    return format_edit_number(value);
}

bool parse_wide_double_text(const wchar_t* text, double& out) {
    if (!text) return false;
    std::wstring s = text;
    for (wchar_t& ch : s) if (ch == L',') ch = L'.';
    const wchar_t* begin = s.c_str();
    while (*begin == L' ' || *begin == L'\t' || *begin == L'\r' || *begin == L'\n') ++begin;
    if (*begin == 0) return false;
    wchar_t* end = nullptr;
    double value = wcstod(begin, &end);
    if (begin == end) return false;
    while (*end == L' ' || *end == L'\t' || *end == L'\r' || *end == L'\n') ++end;
    if (*end != 0) return false;
    out = value;
    return true;
}

constexpr double kFilterSliderRange = 1000.0;
constexpr double kFilterSliderGamma = 4.0;
constexpr double kButterworthQ = 0.70710678118654752440;
constexpr double kBesselQ = 0.57735026918962576451;

struct BiquadCoefficients {
    double b0 = 1.0;
    double b1 = 0.0;
    double b2 = 0.0;
    double a1 = 0.0;
    double a2 = 0.0;
};

struct BiquadState {
    double z1 = 0.0;
    double z2 = 0.0;
};

double current_filter_sample_step() {
    if (!has_data() || g.ds.time.size() < 2) return 0.0;
    if (g.cached_global_gap_step_ready && g.cached_global_gap_step > 0.0 && std::isfinite(g.cached_global_gap_step)) {
        return g.cached_global_gap_step;
    }
    const double step = effective_time_gap_step(g.ds.time, 0, g.ds.time.size());
    if (step > 0.0 && std::isfinite(step)) {
        g.cached_global_gap_step = step;
        g.cached_global_gap_step_ready = true;
        return step;
    }
    return 0.0;
}

double current_filter_nyquist() {
    const double step = current_filter_sample_step();
    return (step > 0.0) ? (0.5 / step) : 0.0;
}

double clamp_filter_cutoff(double hz, double nyquist) {
    if (!std::isfinite(hz) || hz < 0.0) hz = 0.0;
    if (nyquist > 0.0 && hz > nyquist) hz = nyquist;
    return hz;
}

void normalize_filter_bounds() {
    g.noise_threshold_min = std::isfinite(g.noise_threshold_min) ? std::max(0.0, g.noise_threshold_min) : 0.0;
    g.noise_threshold_max = std::isfinite(g.noise_threshold_max) ? std::max(0.0, g.noise_threshold_max) : 0.0;
    if (g.noise_threshold_min > g.noise_threshold_max) {
        std::swap(g.noise_threshold_min, g.noise_threshold_max);
    }
}

double filter_slider_to_frequency(int pos, double nyquist) {
    if (!(nyquist > 0.0)) return 0.0;
    pos = std::clamp(pos, 0, static_cast<int>(kFilterSliderRange));
    if (pos <= 0) return 0.0;
    const double t = static_cast<double>(pos) / kFilterSliderRange;
    return nyquist * std::pow(t, kFilterSliderGamma);
}

int frequency_to_filter_slider(double hz, double nyquist) {
    if (!(nyquist > 0.0) || !(hz > 0.0)) return 0;
    hz = clamp_filter_cutoff(hz, nyquist);
    if (!(hz > 0.0)) return 0;
    const double t = std::pow(hz / nyquist, 1.0 / kFilterSliderGamma);
    return std::clamp(static_cast<int>(std::lround(t * kFilterSliderRange)), 0, static_cast<int>(kFilterSliderRange));
}

std::wstring filter_frequency_text(double hz) {
    if (!(hz > 0.0) || !std::isfinite(hz)) {
        return g_str == &kEn ? L"off" : L"РІС‹РєР»";
    }
    wchar_t buf[64];
    if (hz >= 1000.0) {
        swprintf(buf, 64, g_str == &kEn ? L"%.6g kHz" : L"%.6g РєР“С†", hz / 1000.0);
    } else {
        swprintf(buf, 64, g_str == &kEn ? L"%.6g Hz" : L"%.6g Р“С†", hz);
    }
    return buf;
}

BiquadCoefficients design_biquad(FilterMode mode, int topology, double cutoff_hz, double sample_rate) {
    BiquadCoefficients coeffs;
    if (!(sample_rate > 0.0) || !(cutoff_hz > 0.0) || !std::isfinite(cutoff_hz)) return coeffs;
    if (mode != FilterModeLowPass && mode != FilterModeHighPass) return coeffs;
    cutoff_hz = std::clamp(cutoff_hz, 0.0, sample_rate * 0.499);
    if (!(cutoff_hz > 0.0)) return coeffs;
    const double q = (topology == FilterTopologyBessel) ? kBesselQ :
                     (topology == FilterTopologyChebyshev) ? 0.92 :
                     kButterworthQ;
    const double omega = 2.0 * 3.14159265358979323846 * cutoff_hz / sample_rate;
    const double sn = std::sin(omega);
    const double cs = std::cos(omega);
    const double alpha = sn / (2.0 * q);
    double b0 = 1.0, b1 = 0.0, b2 = 0.0;
    double a0 = 1.0, a1 = 0.0, a2 = 0.0;
    if (mode == FilterModeLowPass) {
        b0 = (1.0 - cs) * 0.5;
        b1 = 1.0 - cs;
        b2 = (1.0 - cs) * 0.5;
    } else {
        b0 = (1.0 + cs) * 0.5;
        b1 = -(1.0 + cs);
        b2 = (1.0 + cs) * 0.5;
    }
    a0 = 1.0 + alpha;
    a1 = -2.0 * cs;
    a2 = 1.0 - alpha;
    if (!(std::abs(a0) > 0.0)) return coeffs;
    coeffs.b0 = b0 / a0;
    coeffs.b1 = b1 / a0;
    coeffs.b2 = b2 / a0;
    coeffs.a1 = a1 / a0;
    coeffs.a2 = a2 / a0;
    return coeffs;
}

double process_biquad_sample(const BiquadCoefficients& coeffs, BiquadState& state, double input) {
    const double output = coeffs.b0 * input + state.z1;
    state.z1 = coeffs.b1 * input - coeffs.a1 * output + state.z2;
    state.z2 = coeffs.b2 * input - coeffs.a2 * output;
    return output;
}

double transformed_channel_sample(std::size_t channel_index, std::size_t row_index) {
    if (channel_index >= g.ds.channel_count()) return std::numeric_limits<double>::quiet_NaN();
    const auto& column = g.ds.channels[channel_index];
    if (row_index >= column.size()) return std::numeric_limits<double>::quiet_NaN();

    const double raw = column[row_index];
    if (std::isnan(raw)) return raw;
    if (!g.has_non_identity_formula || channel_index >= g.channel_transform_kind.size()) {
        return raw;
    }
    const TransformRuntimeKind kind = g.channel_transform_kind[channel_index];
    if (kind == TransformRuntimeKind::Identity) return raw;
    if (kind == TransformRuntimeKind::Affine) {
        return raw * g.channel_transform_mul[channel_index] + g.channel_transform_add[channel_index];
    }
    ensure_transformed_channel_cache(channel_index);
    if (channel_index < g.transformed_channel_cache.size() &&
        row_index < g.transformed_channel_cache[channel_index].size()) {
        return g.transformed_channel_cache[channel_index][row_index];
    }
    return transform_channel_value(channel_index, raw);
}

double rendered_channel_sample(std::size_t channel_index, std::size_t row_index) {
    if (g.noise_threshold_enabled) {
        ensure_filtered_channel_cache(channel_index);
        if (channel_index < g.filtered_channel_cache.size() &&
            row_index < g.filtered_channel_cache[channel_index].size()) {
            return g.filtered_channel_cache[channel_index][row_index];
        }
    }
    return transformed_channel_sample(channel_index, row_index);
}

void ensure_filtered_channel_cache(std::size_t channel_index) {
    ensure_channel_formula_vectors();
    if (!g.noise_threshold_enabled || channel_index >= g.ds.channel_count()) return;
    if (channel_index >= g.filtered_channel_cache_valid.size()) invalidate_filtered_channel_cache();
    if (g.filtered_channel_cache_valid[channel_index]) return;

    const auto& time = g.ds.time;
    const auto& src = g.ds.channels[channel_index];
    auto& dst = g.filtered_channel_cache[channel_index];
    dst.resize(src.size());
    if (src.empty() || time.empty()) {
        g.filtered_channel_cache_valid[channel_index] = 1;
        return;
    }

    const double step = current_filter_sample_step();
    const double nyquist = (step > 0.0) ? (0.5 / step) : 0.0;
    if (!(nyquist > 0.0)) {
        for (std::size_t i = 0; i < src.size(); ++i) dst[i] = transformed_channel_sample(channel_index, i);
        g.filtered_channel_cache_valid[channel_index] = 1;
        return;
    }

    const double sample_rate = 1.0 / step;
    const double gap_threshold = step * 64.0;
    const int mode = (g.noise_threshold_mode >= FilterModeLowPass && g.noise_threshold_mode <= FilterModeBandStop)
        ? g.noise_threshold_mode
        : FilterModeLowPass;
    const int topology = (g.noise_threshold_topology == FilterTopologyBessel)
        ? FilterTopologyBessel
        : (g.noise_threshold_topology == FilterTopologyChebyshev)
            ? FilterTopologyChebyshev
            : (g.noise_threshold_topology == FilterTopologyLinkwitzRiley)
                ? FilterTopologyLinkwitzRiley
                : FilterTopologyButterworth;
    const double low_cutoff = clamp_filter_cutoff(g.noise_threshold_min, nyquist);
    const double high_cutoff = clamp_filter_cutoff(g.noise_threshold_max, nyquist);
    const BiquadCoefficients lowpass = design_biquad(FilterModeLowPass, topology, high_cutoff, sample_rate);
    const BiquadCoefficients highpass = design_biquad(FilterModeHighPass, topology, low_cutoff, sample_rate);
    const bool linkwitz_riley = topology == FilterTopologyLinkwitzRiley;
    const BiquadCoefficients lowpass_lr = design_biquad(FilterModeLowPass, FilterTopologyButterworth, high_cutoff, sample_rate);
    const BiquadCoefficients highpass_lr = design_biquad(FilterModeHighPass, FilterTopologyButterworth, low_cutoff, sample_rate);

    BiquadState stage1{};
    BiquadState stage2{};
    BiquadState stage3{};
    BiquadState stage4{};
    bool have_segment = false;
    double prev_time = 0.0;
    for (std::size_t i = 0; i < src.size(); ++i) {
        const double input = transformed_channel_sample(channel_index, i);
        const double tt = time[i];
        const bool finite = std::isfinite(input) && std::isfinite(tt);
        if (!finite) {
            dst[i] = input;
            stage1 = {};
            stage2 = {};
            have_segment = false;
            prev_time = tt;
            continue;
        }
        if (have_segment) {
            const double dt = tt - prev_time;
            if (!(dt > 0.0) || !std::isfinite(dt) || dt > gap_threshold) {
                stage1 = {};
                stage2 = {};
                have_segment = false;
            }
        }

        double output = input;
        if (linkwitz_riley) {
            switch (mode) {
                case FilterModeHighPass: {
                    const double hp1 = (low_cutoff > 0.0) ? process_biquad_sample(highpass_lr, stage1, input) : input;
                    output = (low_cutoff > 0.0) ? process_biquad_sample(highpass_lr, stage2, hp1) : hp1;
                    break;
                }
                case FilterModeBandPass: {
                    const double hp1 = (low_cutoff > 0.0) ? process_biquad_sample(highpass_lr, stage1, input) : input;
                    const double hp2 = (low_cutoff > 0.0) ? process_biquad_sample(highpass_lr, stage2, hp1) : hp1;
                    const double bp1 = (high_cutoff < nyquist) ? process_biquad_sample(lowpass_lr, stage3, hp2) : hp2;
                    output = (high_cutoff < nyquist) ? process_biquad_sample(lowpass_lr, stage4, bp1) : bp1;
                    break;
                }
                case FilterModeBandStop: {
                    const double hp1 = (low_cutoff > 0.0) ? process_biquad_sample(highpass_lr, stage1, input) : input;
                    const double hp2 = (low_cutoff > 0.0) ? process_biquad_sample(highpass_lr, stage2, hp1) : hp1;
                    const double bp1 = (high_cutoff < nyquist) ? process_biquad_sample(lowpass_lr, stage3, hp2) : hp2;
                    const double bp2 = (high_cutoff < nyquist) ? process_biquad_sample(lowpass_lr, stage4, bp1) : bp1;
                    output = input - bp2;
                    break;
                }
                case FilterModeLowPass:
                default: {
                    const double lp1 = (high_cutoff < nyquist) ? process_biquad_sample(lowpass_lr, stage1, input) : input;
                    output = (high_cutoff < nyquist) ? process_biquad_sample(lowpass_lr, stage2, lp1) : lp1;
                    break;
                }
            }
        } else {
            switch (mode) {
                case FilterModeHighPass:
                    output = (low_cutoff > 0.0) ? process_biquad_sample(highpass, stage1, input) : input;
                    break;
                case FilterModeBandPass: {
                    const double hp = (low_cutoff > 0.0) ? process_biquad_sample(highpass, stage1, input) : input;
                    output = (high_cutoff < nyquist) ? process_biquad_sample(lowpass, stage2, hp) : hp;
                    break;
                }
                case FilterModeBandStop: {
                    const double hp = (low_cutoff > 0.0) ? process_biquad_sample(highpass, stage1, input) : input;
                    const double bp = (high_cutoff < nyquist) ? process_biquad_sample(lowpass, stage2, hp) : hp;
                    output = input - bp;
                    break;
                }
                case FilterModeLowPass:
                default:
                    output = (high_cutoff < nyquist) ? process_biquad_sample(lowpass, stage1, input) : input;
                    break;
            }
        }
        dst[i] = output;
        have_segment = true;
        prev_time = tt;
    }
    g.filtered_channel_cache_valid[channel_index] = 1;
}

void reset_channel_transform(std::size_t ci) {
    ensure_channel_formula_vectors();
    if (ci < g.channel_formulas.size()) g.channel_formulas[ci] = default_channel_formula_text();
    if (ci < g.channel_formula_rpn.size()) {
        g.channel_formula_rpn[ci].clear();
        std::wstring error;
        compile_formula_rpn(g.channel_formulas[ci], g.channel_formula_rpn[ci], error, g_str == &kEn);
    }
    invalidate_formula_runtime_channel(ci);
    ensure_channel_formula_vectors();
}

void reset_all_channel_transforms() {
    ensure_channel_formula_vectors();
    for (std::size_t i = 0; i < g.channel_formulas.size(); ++i) {
        reset_channel_transform(i);
    }
}

void clear_transform_sensitive_overlays(bool clear_history = true) {
    clear_all_measure_point_groups();
    g.markers.clear();
    g.active_marker = -1;
    g.guides.erase(
        std::remove_if(g.guides.begin(), g.guides.end(), [](const GuideLine& gl) { return !gl.vertical; }),
        g.guides.end());
    g.pending_line = 0;
    g.pending_marker = false;
    g.measure_mode = false;
    if (clear_history) {
        g_undo.clear();
        g_redo.clear();
    }
    if (g.measure) SendMessageW(g.measure, BM_SETCHECK, BST_UNCHECKED, 0);
}

void on_signal_transform_changed(bool preserve_history = false) {
    if (!has_data()) return;
    clear_transform_sensitive_overlays(!preserve_history);
    g.auto_y = true;
    g.auto_y_amp = true;
    if (g.autoy) SendMessageW(g.autoy, BM_SETCHECK, BST_CHECKED, 0);
    invalidate_plot_analysis_cache();
    clear_spectrum_cache_state();
    if (g.freq_mode) compute_spectrum();
    sync_menu();
    set_status();
    InvalidateRect(g.main, nullptr, TRUE);
    save_runtime_settings();
}

double export_time_channel_value(std::size_t channel_index, std::size_t row_index) {
    if (channel_index >= g.ds.channel_count()) return std::numeric_limits<double>::quiet_NaN();
    const auto& column = g.ds.channels[channel_index];
    if (row_index >= column.size()) return std::numeric_limits<double>::quiet_NaN();
    return rendered_channel_sample(channel_index, row_index);
}

double export_channel_sample(std::size_t channel_index, std::size_t row_index, bool apply_processing) {
    if (channel_index >= g.ds.channel_count()) return std::numeric_limits<double>::quiet_NaN();
    const auto& column = g.ds.channels[channel_index];
    if (row_index >= column.size()) return std::numeric_limits<double>::quiet_NaN();
    if (!apply_processing) return column[row_index];
    return rendered_channel_sample(channel_index, row_index);
}

std::wstring export_channel_label_text(std::size_t channel_index, bool include_names) {
    if (!include_names) {
        return L"Channel_" + std::to_wstring(channel_index + 1);
    }
    return channel_display_label(channel_index);
}

std::vector<std::size_t> export_channel_indices(bool include_hidden_channels) {
    std::vector<std::size_t> cols;
    const std::size_t count = g.ds.channel_count();
    if (include_hidden_channels) {
        cols.reserve(count);
        for (std::size_t c = 0; c < count; ++c) cols.push_back(c);
        return cols;
    }
    for (std::size_t c = 0; c < count; ++c) {
        if (c < g.visible.size() && g.visible[c]) cols.push_back(c);
    }
    if (cols.empty()) {
        cols.reserve(count);
        for (std::size_t c = 0; c < count; ++c) cols.push_back(c);
    }
    return cols;
}

std::vector<int> export_spectrum_channel_indices(const lvm::Spectrum& spec) {
    std::vector<int> indices;
    indices.reserve(spec.names.size());
    for (const auto& name : spec.names) {
        indices.push_back(channel_index_by_name(name));
    }
    return indices;
}

bool build_export_spectrum(const ExportOptions& opts, lvm::Spectrum& out_spec,
                           std::vector<int>& out_channel_indices, bool& out_all_channels) {
    if (!has_data() || !g.freq_mode) return false;
    out_all_channels = false;
    const bool have_current_spectrum = ensure_current_spectrum();
    if (!opts.include_hidden_channels && have_current_spectrum) {
        out_spec = g.spec;
        out_channel_indices = g.spec_channel_indices;
        return out_spec.ok;
    }

    double source_start = 0.0;
    double source_end = 0.0;
    bool from_selection = false;
    if (!last_fft_source_window(source_start, source_end, from_selection) &&
        !current_fft_source_window(source_start, source_end, from_selection)) {
        return false;
    }

    std::vector<std::size_t> all_channels;
    all_channels.reserve(g.ds.channel_count());
    for (std::size_t i = 0; i < g.ds.channel_count(); ++i) {
        all_channels.push_back(i);
    }

    lvm::Dataset view;
    build_time_window_dataset(g.ds, source_start, source_end, view, &all_channels);
    out_spec = lvm::compute_spectrum(view, 16384);
    if (!out_spec.ok) return false;
    out_channel_indices = export_spectrum_channel_indices(out_spec);
    out_all_channels = true;
    return true;
}

const wchar_t* export_filter_mode_key(int mode) {
    switch (mode) {
        case FilterModeLowPass: return L"low_pass";
        case FilterModeHighPass: return L"high_pass";
        case FilterModeBandPass: return L"band_pass";
        case FilterModeBandStop: return L"band_stop";
    }
    return L"band_pass";
}

const wchar_t* export_filter_topology_key(int topology) {
    switch (topology) {
        case FilterTopologyButterworth: return L"butterworth";
        case FilterTopologyBessel: return L"bessel";
        case FilterTopologyChebyshev: return L"chebyshev";
        case FilterTopologyLinkwitzRiley: return L"linkwitz_riley";
    }
    return L"butterworth";
}

std::wstring export_color_triplet(COLORREF color) {
    wchar_t buf[64]{};
    swprintf(buf, 64, L"%u,%u,%u", static_cast<unsigned int>(GetRValue(color)),
             static_cast<unsigned int>(GetGValue(color)), static_cast<unsigned int>(GetBValue(color)));
    return buf;
}

std::wstring export_point_display_text(const PointDisplay& display) {
    wchar_t buf[160]{};
    swprintf(buf, 160, L"number=%d,x=%d,y=%d,dx=%d,dy=%d,inv_dt=%d,dist=%d",
             display.number ? 1 : 0, display.x ? 1 : 0, display.y ? 1 : 0,
             display.dx ? 1 : 0, display.dy ? 1 : 0, display.inv_dt ? 1 : 0,
             display.dist ? 1 : 0);
    return buf;
}

void write_export_comment(std::ofstream& out, const std::wstring& text, const char* line_end) {
    out << "# " << to_utf8(text) << line_end;
}

void write_export_key_value(std::ofstream& out, const std::wstring& key, const std::wstring& value, const char* line_end) {
    write_export_comment(out, key + L"=" + value, line_end);
}

const wchar_t* export_range_mode_key(ExportRangeMode range) {
    switch (range) {
        case ExportRangeMode::Selected: return L"selected";
        case ExportRangeMode::Visible: return L"visible";
        case ExportRangeMode::Whole: return L"whole";
    }
    return L"visible";
}

const wchar_t* export_processing_mode_key(bool apply_processing) {
    return apply_processing ? L"applied_to_data" : L"settings_only";
}

bool export_range_bounds_for_mode(ExportRangeMode range, double& start, double& end, bool& actual_selected) {
    if (!has_data()) return false;
    actual_selected = false;
    if (g.freq_mode) {
        if (!ensure_current_spectrum()) return false;
        actual_selected = (range == ExportRangeMode::Selected);
        if (range == ExportRangeMode::Whole) {
            start = g.spec.freqs.empty() ? 0.0 : g.spec.freqs.front();
            end = g.spec.freqs.empty() ? 0.0 : g.spec.freqs.back();
        } else {
            start = g.freq_start;
            end = g.freq_end;
        }
        if (!std::isfinite(start) || !std::isfinite(end)) return false;
        if (end < start) std::swap(start, end);
        return end > start;
    }

    switch (range) {
        case ExportRangeMode::Selected:
            if (has_fft_window()) {
                start = g.fft_window_start;
                end = g.fft_window_end;
                clamp_time_window(start, end);
                actual_selected = true;
                return end > start;
            }
            start = g.win_start;
            end = g.win_end;
            clamp_time_window(start, end);
            return end > start;
        case ExportRangeMode::Visible:
            start = g.win_start;
            end = g.win_end;
            clamp_time_window(start, end);
            return end > start;
        case ExportRangeMode::Whole:
            start = g.ds.time.empty() ? 0.0 : g.ds.time.front();
            end = g.ds.time.empty() ? 0.0 : g.ds.time.back();
            return end > start;
    }
    return false;
}

void write_export_metadata(std::ofstream& out,
