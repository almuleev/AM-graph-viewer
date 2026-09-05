// Headless integration tests of the real GUI data/export pipeline. No windows are opened.
#include "../gui_main.cpp"
#include <iostream>
#include <stdexcept>
#include <chrono>
#ifdef near
#undef near
#endif

namespace {
int checks = 0;
void require(bool condition, const char* message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
}
void near(double actual, double expected, const char* message) {
    require(std::isfinite(actual) && std::fabs(actual - expected) < 1e-9, message);
}
const std::filesystem::path test_dir = "tests/_tmp_gui_regression";

void reset_document(const std::vector<std::string>& names, const std::vector<double>& time,
                    const std::vector<std::vector<double>>& channels) {
    g = App{};
    g_undo.clear(); g_redo.clear(); g_filter_slider_before.reset();
    g_str = &kEn;
    g_config_path = (test_dir / "settings.ini").wstring();
    g.ds.ok = true; g.ds.names = names; g.ds.time = time; g.ds.raw_time = time; g.ds.channels = channels;
    g.visible.assign(names.size(), 1);
    g_channel_colors.clear();
    for (const auto& name : names) g.channel_labels.push_back(to_w(name));
    g.data_t0 = g.win_start = time.front(); g.data_t1 = g.win_end = time.back();
    g.channel_formulas.assign(names.size(), L"x");
    ensure_channel_formula_vectors();
}

void reopen(const std::filesystem::path& path, bool dedup = false) {
    auto ds = lvm::read_lvm_file(path);
    require(ds.ok, "export must be readable");
    if (dedup) lvm::drop_duplicate_time_channels(ds, ds.raw_time);
    auto comments = ds.export_comments;
    reset_document(ds.names, ds.time, ds.channels);
    g.ds.frequency_axis = ds.frequency_axis;
    g.ds.export_comments = comments;
    apply_export_metadata_from_comments(comments);
}

void exports() {
    ExportOptions opts;
    opts.selected_range = ExportRangeMode::Whole;
    opts.include_hidden_channels = true;
    const auto path = test_dir / std::filesystem::u8path(u8"измерение_測定.csv");
    reset_document({"A"}, {0,1,2,3}, {{10,11,12,13}});
    g.global_formula = L"2*x"; rebuild_formula_cache_from_state();
    require(save_tabular_export(path.wstring(), opts), "processed export");
    reopen(path);
    near(rendered_channel_sample(0,0), 20, "processed export must not apply formula twice");
    require(save_tabular_export(path.wstring(), opts), "second export");
    reopen(path);
    near(rendered_channel_sample(0,0), 20, "multiple roundtrips remain stable");

    reset_document({"A","B"}, {0,1,2,3}, {{10,11,12,13},{20,21,22,23}});
    g.visible = {0,1}; g.channel_formulas = {L"3*x",L"x+7"}; rebuild_formula_cache_from_state();
    opts.include_hidden_channels = false; opts.apply_processing_to_data = false;
    require(save_tabular_export(path.wstring(), opts), "subset export");
    reopen(path);
    require(g.ds.names == std::vector<std::string>{"B"} && g.visible[0], "subset identity and visibility");
    near(rendered_channel_sample(0,0), 27, "subset local formula mapping");

    reset_document({u8"Датчик, \"A\", visible=0","B"}, {0,1,2,3}, {{10,11,12,13},{20,21,22,23}});
    opts.include_hidden_channels = true;
    require(save_tabular_export(path.wstring(), opts), "quoted name export");
    reopen(path);
    require(g.ds.names[0] == u8"Датчик, \"A\", visible=0" && g.ds.names[1] == "B", "UTF-8 and metadata text escaping");

    reset_document({"A","B"}, {0,1,2,3}, {{10,11,12,13},{20,21,22,23}});
    opts.format = ExportFileFormat::Lvm;
    const auto lvm_path = test_dir / "roundtrip.lvm";
    require(save_lvm_export(lvm_path.wstring(), opts), "LVM export");
    auto parsed = lvm::read_lvm_file(lvm_path);
    require(parsed.ok, "read LVM");
    lvm::drop_duplicate_time_channels(parsed, parsed.raw_time);
    require(parsed.names == std::vector<std::string>{"A","B"}, "LVM labels match data without metadata import");
    near(parsed.channels[0][0], 10, "LVM channel A"); near(parsed.channels[1][3], 23, "LVM channel B");

    opts.format = ExportFileFormat::Csv; opts.selected_range = ExportRangeMode::Visible;
    { std::ofstream out(path); out << "KEEP THIS FILE"; }
    g.win_start = 10; g.win_end = 11;
    require(!save_tabular_export(path.wstring(), opts), "invalid export fails");
    std::ifstream in(path); std::string content; std::getline(in, content);
    require(content == "KEEP THIS FILE", "failed export preserves destination");
}

void processing() {
    reset_document({"same","same"},{0,.1,.2,.3,.4,.5,.6,.7},{{1,0,-1,0,1,0,-1,0},{2,0,-2,0,2,0,-2,0}});
    compute_spectrum_for_window(0,.7,false);
    require(g.spec_channel_indices == std::vector<int>{0,1}, "duplicate names retain independent spectrum identities");
    g.global_formula=L"2*x"; rebuild_formula_cache_from_state();
    compute_spectrum_for_window(0,.7,false); g.freq_mode=true;
    ExportOptions opts; opts.apply_processing_to_data=false; opts.include_hidden_channels=true;
    lvm::Spectrum spec; std::vector<int> ids; bool all=false;
    require(build_export_spectrum(opts,spec,ids,all), "raw spectrum export");
    const auto peaks=lvm::find_peaks(spec.freqs,spec.amp[0],1);
    require(!peaks.empty(), "raw spectrum peak"); near(peaks[0].amp,1,"raw FFT export ignores active formula");

    std::vector<double> time(1000), signal(1000,0);
    for(std::size_t i=0;i<time.size();++i) time[i]=double(i)/1000;
    signal[499]=1; signal[500]=std::nan("");
    reset_document({"signal"},time,{signal});
    g.noise_threshold_enabled=true;g.noise_threshold_mode=FilterModeBandPass;
    g.noise_threshold_topology=FilterTopologyLinkwitzRiley;g.noise_threshold_min=10;g.noise_threshold_max=100;
    ensure_filtered_channel_cache(0);
    near(g.filtered_channel_cache[0][501],0,"all LR stages reset after NaN");
    g.noise_threshold_enabled=false;
    g.global_formula=L"sqrt(x)";rebuild_formula_cache_from_state();
    require(std::isnan(transform_channel_value(0,-1)),"domain error does not silently restore input");
}

void fft_recording_recovery() {
    std::vector<double> time, signal;
    constexpr double pi = 3.14159265358979323846;
    for (int i = 0; i < 1152; ++i) {
        const int local = i < 128 ? i : i - 128;
        const double offset = i >= 128 && local > 0 && local < 1023 ? 0.0001 * std::sin(2 * pi * local / 17) : 0;
        const double t = (i < 128 ? 0 : 10) + local / 1024.0 + offset;
        time.push_back(t); signal.push_back(std::sin(2 * pi * 64 * t));
    }
    reset_document({"signal"}, time, {signal});
    compute_spectrum_for_window(time.front(), time.back(), false);
    require(g.spec_valid && g.spec.gaps_ignored && g.spec.resampled && g.spec.n == 1152, "GUI uses every jittered sample around a gap");
    require(g.spec_channel_indices == std::vector<int>{0}, "recovered FFT preserves GUI channel mapping");
    near(g.spec.source_start, 0, "GUI spectrum records the full selected interval");
    g_str = &kRu;
    const auto status = spectrum_sampling_status(g.spec);
    require(status.find(L"без учёта пропусков") != std::wstring::npos && status.find(L"интерполяция") != std::wstring::npos,
            "Russian status explains ignored gaps and interpolation");
    g_str = &kEn;

    // Exercise the real background worker, then its GUI completion handler.
    lvm::SpectrumWorker worker;
    worker.submit(g.ds, {7}, 1);
    worker.submit(g.ds, {0}, 2);
    std::optional<lvm::SpectrumWorker::Result> result;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (!result && std::chrono::steady_clock::now() < deadline) {
        result = worker.take_result();
        if (!result) std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    require(result && result->generation == 2, "worker publishes the latest FFT request");
    require(result->spectrum.ok && result->spectrum.resampled && result->spectrum.gaps_ignored && result->spectrum.n == 1152,
            "background FFT recovers the same irregular recording");
    g.freq_start = 0; g.freq_end = 1; g.spec_pending = true; g.spec_fit_pending = true;
    apply_spectrum_result(std::move(result->spectrum));
    require(g.spec_valid && !g.spec_pending && !g.spec_fit_pending, "GUI accepts background FFT result");
    near(g.freq_end, g.spec.nyquist, "background FFT restores full frequency axis");

    // Re-entering the same source window must still request a fresh axis fit.
    set_mode(true);
    require(g.spec_valid && g.freq_end > 100, "entering FFT fits the frequency axis");
    set_mode(false); g.freq_end = 1;
    set_mode(true);
    near(g.freq_end, g.spec.nyquist, "re-entering FFT restores full frequency axis");

    ExportOptions opts; opts.selected_range = ExportRangeMode::Whole; opts.include_hidden_channels = true;
    const auto path = test_dir / "recovered_fft.csv";
    require(save_tabular_export(path.wstring(), opts), "recovered spectrum exports");
    const auto exported = lvm::read_lvm_file(path);
    require(exported.ok && exported.rows() == g.spec.freqs.size(), "recovered spectrum export has all frequency bins");
    bool source = false, resampled = false, gaps_ignored = false;
    for (const auto& comment : exported.export_comments) {
        source = source || comment == "source_start=0";
        resampled = resampled || comment == "resampled=1";
        gaps_ignored = gaps_ignored || comment == "gaps_ignored=1";
    }
    require(source && resampled && gaps_ignored, "export records full FFT source, interpolation and ignored gaps");
}

void light_mode_and_history() {
    reset_document({"gap"}, {0, 1, 2, 102, 103, 104}, {{0, 1, 2, 3, 4, 5}});
    g.stitch_time_gaps = true;
    invalidate_stitched_time_cache();
    near(stitched_time_at_index(0), 0, "stitched graph preserves first timestamp");
    near(stitched_time_at_index(3), 3, "stitched graph removes a large time gap");
    near(stitched_time_at_index(5), 5, "stitched graph keeps samples after the gap contiguous");
    near(stitched_time_from_raw(103), 4, "raw time maps to stitched graph axis");
    near(raw_time_from_stitched(4), 103, "stitched graph coordinate maps back to raw time");
    g.stitch_time_gaps = false;
    near(stitched_time_at_index(3), 102, "disabling stitched graph restores the original time axis");

    std::vector<double> time(300001), values(time.size(), 0);
    for (std::size_t i = 0; i < time.size(); ++i) time[i] = i * 0.001;
    values[11] = 100;
    reset_document({"impulse"}, time, {values});
    g.light_mode = true;
    double low = 0, high = 0;
    require(current_time_yrange_window(0, time.size(), low, high) && high >= 100, "Light Mode auto Y includes a single-sample impulse");
    g.global_formula = L"2*x"; rebuild_formula_cache_from_state(); recompute_transforms_from_state();
    require(current_time_yrange_window(0, time.size(), low, high) && high >= 200, "envelope invalidates after formula changes");
    require(current_time_yrange_window(12, 100, low, high) && high < 10, "envelope query excludes an impulse outside the window");

    reset_document({"signal"}, {0,.001,.002,.003}, {{1,0,-1,0}});
    g.noise_threshold_enabled = true; g.noise_threshold_min = 10; g.noise_threshold_max = 400;
    PointGroup group; group.name = L"measurements"; group.points = {{0,1}};
    g.point_groups.push_back(group); g.active_point_group = g.time_active_point_group = 0;
    const auto serial = g.plot_analysis_serial;
    for (int position = 400; position < 600; ++position) apply_filter_slider_change(true, position, true);
    near(g.noise_threshold_min, 10, "drag previews do not change active filter");
    require(g_undo.empty() && g.plot_analysis_serial == serial, "drag previews avoid repeated FFT/filter invalidation and history copies");
    apply_filter_slider_change(true, 599, false);
    const double applied = g.noise_threshold_min;
    require(g_undo.size() == 1 && g.point_groups.empty(), "one filter drag creates one undo action and clears obsolete measurements");
    apply_filter_slider_change(true, 599, false);
    require(g_undo.size() == 1, "track end notification does not duplicate undo");
    pop_undo();
    near(g.noise_threshold_min, 10, "undo restores original cutoff");
    require(g.point_groups.size() == 1 && g.point_groups[0].points.size() == 1, "undo restores measurements removed by the filter change");
    pop_redo();
    near(g.noise_threshold_min, applied, "redo restores applied cutoff");
    require(g.point_groups.empty(), "redo clears obsolete measurements again");
    ensure_filtered_channel_cache(0);
    require(!g.filtered_channel_cache[0].empty(), "enabled filter produces a cache");
    g.noise_threshold_enabled = false; invalidate_filtered_channel_cache();
    require(g.filtered_channel_cache[0].capacity() == 0, "disabling filter releases stored filtered samples");

    g_undo.clear(); g_redo.clear();
    for (std::size_t i = 0; i < kUndoActionLimit + 20; ++i) {
        UndoAction action; action.type = UndoAction::ADD_LINE; action.line.value = static_cast<double>(i);
        push_undo(std::move(action));
    }
    require(g_undo.size() == kUndoActionLimit && g_undo.front().line.value == 20, "history discards oldest actions at the count limit");
    g_undo.clear();
    for (int i = 0; i < 10; ++i) {
        UndoAction action; action.type = UndoAction::CLEAR_POINTS;
        action.saved_point_groups.resize(1);
        action.saved_point_groups[0].points.resize(kUndoByteLimit / 8 / sizeof(std::pair<double,double>));
        push_undo(std::move(action));
    }
    require(history_stack_bytes(g_undo) <= kUndoByteLimit && g_undo.size() < 10, "history obeys memory limit for large measurement groups");
    g_undo.clear();
}

void reopen_spectrum() {
    const auto path = test_dir / "recovered_fft.csv";
    const auto original = lvm::read_lvm_file(path);
    require(original.ok && original.frequency_axis, "Frequency column identifies an exported spectrum");
    reopen(path, true);
    set_mode(true);
    require(g.spec_valid && g.spec.imported && g.spec.freqs == original.time && g.spec.amp == original.channels,
            "opening spectrum preserves all bins and amplitudes without a second FFT");
    near(g.spec.source_start, 0, "reopened spectrum preserves source interval");
    require(g.spec.resampled && g.spec.gaps_ignored, "reopened spectrum preserves sampling provenance");
    set_mode(false);
    require(g.freq_mode && current_filter_nyquist() == 0, "stored spectrum cannot enter time mode or use a time-domain filter");
    ExportOptions opts; opts.include_hidden_channels = true; opts.selected_range = ExportRangeMode::Whole;
    const auto second_path = test_dir / "spectrum_second_roundtrip.csv";
    require(save_tabular_export(second_path.wstring(), opts), "stored spectrum exports again");
    const auto second = lvm::read_lvm_file(second_path);
    require(second.ok && second.frequency_axis && second.time == original.time && second.channels == original.channels,
            "repeated spectrum export preserves exact numeric values");
    reset_document({"signal"},{0,.1,.2,.3,.4,.5,.6,.7},{{1,0,-1,0,1,0,-1,0}});
    g.global_formula = L"3*x"; rebuild_formula_cache_from_state(); set_mode(true);
    opts.apply_processing_to_data = false;
    require(save_tabular_export(second_path.wstring(), opts), "raw spectrum export stores processing recipe");
    reopen(second_path); set_mode(true);
    require(g.spec_valid && g.spec.imported, "raw spectrum reopens directly");
    const auto peaks = lvm::find_peaks(g.spec.freqs, g.spec.amp[0], 1);
    require(!peaks.empty(), "reopened raw spectrum retains peak");
    near(peaks[0].amp, 1, "time-domain recipe is never reapplied to imported FFT amplitudes");
}
}

int main() {
    std::filesystem::create_directories(test_dir);
    try {
        exports(); processing(); fft_recording_recovery();
        light_mode_and_history(); reopen_spectrum();
        std::cout << checks << " GUI integration checks passed\n";
        return 0;
    } catch(const std::exception& ex) {
        std::cerr << "FAIL after " << checks << " checks: " << ex.what() << '\n';
        return 1;
    }
}
