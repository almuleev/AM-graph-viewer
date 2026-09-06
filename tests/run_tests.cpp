// Unit tests for the LVM parser and spectrum analysis.
//
// A tiny assertion harness (no external framework). Each test writes a small
// temporary .lvm/.txt file, parses it, and checks the result. Exit code is
// non-zero if any check fails, so it works as a `make test` gate.
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "analysis.hpp"
#include "export_helpers.hpp"
#include "gap_details.hpp"
#include "formula_engine.hpp"
#include "lvm_parser.hpp"
#include "data_io.hpp"
#include "minmax_index.hpp"
#include <limits>

namespace {

int g_failures = 0;
int g_checks = 0;

void check(bool cond, const std::string& what) {
    ++g_checks;
    if (!cond) {
        ++g_failures;
        std::printf("  FAIL: %s\n", what.c_str());
    }
}

void check_near(double a, double b, double tol, const std::string& what) {
    ++g_checks;
    if (!(std::fabs(a - b) <= tol)) {
        ++g_failures;
        std::printf("  FAIL: %s (got %.9g, expected %.9g)\n", what.c_str(), a, b);
    }
}

struct TempFile {
    std::string path;

    TempFile(const std::string& name, const std::string& content) : path("tests/_tmp_" + name) {
        std::ofstream out(path, std::ios::binary);
        out << content;
    }

    ~TempFile() {
        std::remove(path.c_str());
    }

    TempFile(const TempFile&) = delete;
    TempFile& operator=(const TempFile&) = delete;
};

void test_basic_parse() {
    std::printf("test_basic_parse\n");
    const std::string content =
        "LabVIEW Measurement\n"
        "Separator\tTab\n"
        "Multi_Headings\tNo\n"
        "***End_of_Header***\n"
        "Time\tChannel_1[V]\tChannel_2[V]\n"
        "0.0\t1.0\t10.0\n"
        "0.1\t2.0\t20.0\n"
        "0.2\t3.0\t30.0\n";
    const TempFile temp("basic.lvm", content);
    lvm::Dataset ds = lvm::read_lvm_file(temp.path);

    check(ds.ok, "parse ok");
    check(ds.stats.header_markers == 1, "one header marker");
    check(ds.rows() == 3, "three rows kept");
    check(ds.channel_count() == 2, "two channels");
    check(ds.names.size() == 2 && ds.names[0] == "Channel_1" && ds.names[1] == "Channel_2",
          "channel names Channel_1/Channel_2");
    check_near(ds.time[0], 0.0, 1e-12, "time[0]");
    check_near(ds.time[2], 0.2, 1e-12, "time[2]");
    check_near(ds.channels[1][2], 30.0, 1e-12, "channel_2 last value");
}

void test_metadata_and_nan() {
    std::printf("test_metadata_and_nan\n");
    const std::string content =
        "LabVIEW Measurement\n"
        "Date\t11/12/2025\n"
        "Time\t12:00:00.0000000\n"     // metadata timestamp -> skipped
        "***End_of_Header***\n"
        "Time\tCh1\tCh2\n"             // first cell "Time" is metadata -> skipped
        "0.0\t1.0\t2.0\n"
        "0.1\t\t4.0\n"                 // empty cell -> NaN, two numeric? only 0.1,4.0 -> kept
        "0.2\tbad\t6.0\n"              // non-numeric -> NaN
        "junk line with one 5\n";      // < 2 numeric values -> skipped
    const TempFile temp("nan.lvm", content);
    lvm::Dataset ds = lvm::read_lvm_file(temp.path);

    check(ds.ok, "parse ok");
    check(ds.rows() == 3, "three data rows (junk skipped)");
    check(ds.channel_count() == 2, "two channels");
    check(std::isnan(ds.channels[0][1]), "empty cell -> NaN");
    check(std::isnan(ds.channels[0][2]), "non-numeric cell -> NaN");
    check_near(ds.channels[1][1], 4.0, 1e-12, "ch2 row 1 value");
}

void test_decimal_comma() {
    std::printf("test_decimal_comma\n");
    const std::string content =
        "***End_of_Header***\n"
        "0,0\t1,5\n"
        "0,1\t2,5\n"
        "0,2\t3,5\n"
        "0,3\t4,5\n";
    const TempFile temp("comma.txt", content);
    lvm::Dataset ds = lvm::read_lvm_file(temp.path);

    check(ds.ok, "parse ok");
    check(ds.rows() == 4, "four rows");
    check_near(ds.time[1], 0.1, 1e-12, "comma decimal in time");
    check_near(ds.channels[0][0], 1.5, 1e-12, "comma decimal in channel");
}

void test_multi_header() {
    std::printf("test_multi_header\n");
    const std::string content =
        "***End_of_Header***\n"
        "0.0\t1.0\n"
        "0.1\t2.0\n"
        "***End_of_Header***\n"        // second section
        "0.0\t3.0\n"                   // local time resets
        "0.1\t4.0\n";
    const TempFile temp("multi.lvm", content);
    lvm::Dataset ds = lvm::read_lvm_file(temp.path);

    check(ds.stats.header_markers == 2, "two header markers");
    check(ds.stats.data_sections == 2, "two data sections");
    check(ds.rows() == 4, "four combined rows");

    // raw time is non-monotonic (0,0.1,0,0.1) -> make_monotonic fixes it.
    std::vector<double> t = ds.time;
    lvm::make_monotonic(t);
    bool increasing = true;
    for (std::size_t i = 1; i < t.size(); ++i) increasing = increasing && (t[i] > t[i - 1]);
    check(increasing, "make_monotonic yields strictly increasing time");
}

void test_make_monotonic_equal_times() {
    std::printf("test_make_monotonic_equal_times\n");
    std::vector<double> t = {0.0, 0.1, 0.1, 0.2};
    lvm::make_monotonic(t);

    bool increasing = true;
    for (std::size_t i = 1; i < t.size(); ++i) increasing = increasing && (t[i] > t[i - 1]);
    check(increasing, "equal timestamps are pushed forward to stay strictly increasing");
    check_near(t[2], 0.2, 1e-12, "duplicate timestamp advanced by the fallback step");
}

void test_make_monotonic_backward_jump() {
    std::printf("test_make_monotonic_backward_jump\n");
    std::vector<double> t = {0.0, 0.1, 0.05, 0.15};
    lvm::make_monotonic(t);

    bool increasing = true;
    for (std::size_t i = 1; i < t.size(); ++i) increasing = increasing && (t[i] > t[i - 1]);
    check(increasing, "backward jumps are pushed forward to stay strictly increasing");
}

void test_drop_duplicate_time() {
    std::printf("test_drop_duplicate_time\n");
    // Channel_1 duplicates time exactly; Channel_2 is real data.
    const std::string content =
        "***End_of_Header***\n"
        "0.0\t0.0\t5.0\n"
        "1.0\t1.0\t6.0\n"
        "2.0\t2.0\t7.0\n"
        "3.0\t3.0\t8.0\n";
    const TempFile temp("dup.lvm", content);
    lvm::Dataset ds = lvm::read_lvm_file(temp.path);
    check(ds.channel_count() == 2, "two channels before drop");

    const std::vector<double> raw_time = ds.time;
    const auto dropped = lvm::drop_duplicate_time_channels(ds, raw_time);
    check(dropped.size() == 1 && dropped[0] == "Channel_1", "Channel_1 dropped as time dup");
    check(ds.channel_count() == 1 && ds.names[0] == "Channel_2", "Channel_2 remains");
}

void test_interleaved_channel_names() {
    std::printf("test_interleaved_channel_names\n");
    const std::string content =
        "LabVIEW Measurement\n"
        "Writer_Version\t0.92\n"
        "Reader_Version\t1\n"
        "Separator\tTab\n"
        "Multi_Headings\tYes\n"
        "X_Columns\tMulti\n"
        "***End_of_Header***\n"
        "Channels\t8\t\t\t\t\t\t\t\n"
        "Samples\t2\t2\t2\t2\t2\t2\t2\t2\n"
        "Date\t2009/05/15\t2009/05/15\t2009/05/15\t2009/05/15\t2009/05/15\t2009/05/15\t2009/05/15\t2009/05/15\n"
        "Time\t00:00:00,000\t00:00:00,000\t00:00:00,000\t00:00:00,000\t00:00:00,000\t00:00:00,000\t00:00:00,000\t00:00:00,000\n"
        "X0\t0\t0\t0\t0\t0\t0\t0\t0\n"
        "Delta_X\t0.1\t0.1\t0.1\t0.1\t0.1\t0.1\t0.1\t0.1\n"
        "***End_of_Header***\n"
        "X_Value\tg1\tX_Value\tg2\tX_Value\tg3\tX_Value\tg4\tX_Value\tg5\tX_Value\tg6\tX_Value\tg7\tX_Value\tg8\tComment\n"
        "0.0\t1.0\t0.0\t2.0\t0.0\t3.0\t0.0\t4.0\t0.0\t5.0\t0.0\t6.0\t0.0\t7.0\t0.0\t8.0\tok\n"
        "0.1\t1.1\t0.1\t2.1\t0.1\t3.1\t0.1\t4.1\t0.1\t5.1\t0.1\t6.1\t0.1\t7.1\t0.1\t8.1\tok\n";
    const TempFile temp("interleaved_names.lvm", content);
    lvm::Dataset ds = lvm::read_lvm_file(temp.path);
    check(ds.ok, "interleaved file parses");
    check(ds.channel_count() == 15, "interleaved file initially has 15 channels before de-dup");

    const std::vector<double> raw_time = ds.raw_time.empty() ? ds.time : ds.raw_time;
    const auto dropped = lvm::drop_duplicate_time_channels(ds, raw_time);
    check(dropped.size() == 7, "seven duplicate time columns dropped");
    check(ds.channel_count() == 8, "eight data channels remain");
    const std::vector<std::string> expected = {"g1", "g2", "g3", "g4", "g5", "g6", "g7", "g8"};
    check(ds.names == expected, "channel names preserved as g1..g8");
}

void test_reference_test_lvm() {
    std::printf("test_reference_test_lvm\n");
    lvm::Dataset ds = lvm::read_lvm_file("lvm_files_for_tests/test.lvm");
    check(ds.ok, "reference test.lvm parses");
    check(ds.rows() == 10000, "reference test.lvm row count");
    check(ds.channel_count() == 3, "reference test.lvm channel count");
    check(ds.names.size() == 3 && ds.names[0] == "Channel_1" && ds.names[1] == "Channel_2" &&
              ds.names[2] == "Channel_3",
          "reference test.lvm channel names");
    if (ds.ok && ds.rows() == 10000 && ds.channel_count() == 3) {
        check_near(ds.time.front(), 0.0, 1e-12, "reference time starts at zero");
        check_near(ds.time.back(), 9.999, 1e-12, "reference time ends at 9.999");
    }
}

void test_fft_peak() {
    std::printf("test_fft_peak\n");
    // 50 Hz sine sampled at 1000 Hz for 1 s -> peak near 50 Hz.
    lvm::Dataset ds;
    ds.names.push_back("Channel_1");
    ds.channels.resize(1);
    const int n = 1000;
    const double fs = 1000.0, freq = 50.0;
    for (int i = 0; i < n; ++i) {
        const double t = static_cast<double>(i) / fs;
        ds.time.push_back(t);
        ds.channels[0].push_back(std::sin(2.0 * 3.14159265358979323846 * freq * t));
    }
    ds.ok = true;

    lvm::Spectrum spec = lvm::compute_spectrum(ds, 0);  // no cap
    check(spec.ok, "spectrum ok");
    check_near(spec.sample_dt, 1.0 / fs, 1e-9, "sample_dt");
    check_near(spec.nyquist, fs / 2.0, 1e-6, "nyquist");

    const auto peaks = lvm::find_peaks(spec.freqs, spec.amp[0], 1);
    check(!peaks.empty(), "found a peak");
    if (!peaks.empty()) {
        check_near(peaks[0].freq, freq, 1.0, "peak at ~50 Hz");
        check_near(peaks[0].amp, 1.0, 0.05, "peak amplitude ~1.0");
    }
}

void test_fft_nyquist_amplitude() {
    std::printf("test_fft_nyquist_amplitude\n");
    lvm::Dataset ds;
    ds.names.push_back("Channel_1");
    ds.channels.resize(1);
    const int n = 8;
    const double fs = 8.0;
    for (int i = 0; i < n; ++i) {
        const double t = static_cast<double>(i) / fs;
        ds.time.push_back(t);
        ds.channels[0].push_back((i % 2 == 0) ? 1.0 : -1.0);
    }
    ds.ok = true;

    const lvm::Spectrum spec = lvm::compute_spectrum(ds, 0);
    check(spec.ok, "nyquist spectrum ok");
    check(spec.amp.size() == 1, "one channel in nyquist spectrum");
    if (spec.ok && spec.amp.size() == 1) {
        check_near(spec.freqs.back(), fs / 2.0, 1e-12, "nyquist frequency bin");
        check_near(spec.amp[0].back(), 1.0, 1e-12, "nyquist amplitude is not doubled");
    }
}

void test_fft_sample_cap_too_small() {
    std::printf("test_fft_sample_cap_too_small\n");
    lvm::Dataset ds;
    ds.names.push_back("Channel_1");
    ds.channels.resize(1);
    for (int i = 0; i < 16; ++i) {
        ds.time.push_back(static_cast<double>(i));
        ds.channels[0].push_back(static_cast<double>(i));
    }
    ds.ok = true;

    const lvm::Spectrum spec = lvm::compute_spectrum(ds, 3);
    check(!spec.ok, "tiny fft sample cap is rejected");
    check(!spec.error.empty(), "tiny fft sample cap reports an error");
}

void test_missing_file() {
    std::printf("test_missing_file\n");
    lvm::Dataset ds = lvm::read_lvm_file("tests/_does_not_exist.lvm");
    check(!ds.ok, "missing file -> not ok");
    check(!ds.error.empty(), "error message set");
}

void test_formula_engine() {
    std::printf("test_formula_engine\n");
    std::vector<FormulaToken> rpn;
    std::wstring error;
    check(compile_formula_rpn(L"2*x + 3", rpn, error, true), "formula compiles");
    check(!formula_rpn_is_identity(rpn), "non-identity formula detected");

    const AffineFormulaInfo affine = analyze_formula_rpn_affine(rpn);
    check(affine.valid, "affine formula detected");
    check_near(affine.mul, 2.0, 1e-12, "affine mul");
    check_near(affine.add, 3.0, 1e-12, "affine add");
    check_near(eval_formula_rpn(rpn, 4.0), 11.0, 1e-12, "formula evaluation");

    std::vector<FormulaToken> identity;
    error.clear();
    check(compile_formula_rpn(L"x", identity, error, true), "identity formula compiles");
    check(formula_rpn_is_identity(identity), "identity formula detected");
}

void test_export_helpers() {
    std::printf("test_export_helpers\n");
    const std::vector<double> values = {0.0, 0.1, 0.2, 0.3, 0.4};
    const auto bounds = export_range_bounds(values, 0.15, 0.35);
    check(bounds.first == 2 && bounds.second == 4, "export range bounds");
}

void test_gap_details() {
    std::printf("test_gap_details\n");
    check(gap_estimated_missing_samples(0.131, 0.03275) == 3, "gap estimate formula");
    check_near(gap_reference_step_from_estimate(0.131, 3), 0.03275, 1e-12, "gap reference step");

    const std::wstring en = gap_details_body_text(true, 0.131, 3, 0.03275);
    check(en.find(L"Gap duration: 0.131 s") != std::wstring::npos, "gap body EN duration");
    check(en.find(L"Approx. missing samples: ~3") != std::wstring::npos, "gap body EN samples");
    check(en.find(L"Reference step: 0.03275 s") != std::wstring::npos, "gap body EN step");

    const std::wstring ru = gap_details_body_text(false, 0.131, 3, 0.03275);
    check(ru.find(L"Длительность разрыва: 0.131 c") != std::wstring::npos, "gap body RU duration");
    check(ru.find(L"Пропущено примерно: ~3 отсч.") != std::wstring::npos, "gap body RU samples");
    check(ru.find(L"Типичный шаг: 0.03275 c") != std::wstring::npos, "gap body RU step");
}

void test_scan_and_window_load() {
    std::printf("test_scan_and_window_load\n");
    const std::string content =
        "***End_of_Header***\n"
        "0.0\t1.0\t10.0\n"
        "0.1\t2.0\t20.0\n"
        "0.2\t3.0\t30.0\n"
        "0.3\t4.0\t40.0\n"
        "0.4\t5.0\t50.0\n";
    const TempFile temp("window.lvm", content);

    double start = 0.0;
    double end = 0.0;
    std::string error;
    check(lvm::scan_time_bounds(temp.path, start, end, error), "scan_time_bounds ok");
    check(error.empty(), "scan_time_bounds error empty");
    check_near(start, 0.0, 1e-12, "scan start");
    check_near(end, 0.4, 1e-12, "scan end");

    lvm::LoadOptions options;
    options.use_time_window = true;
    options.time_start = 0.15;
    options.time_end = 0.35;
    lvm::Dataset ds = lvm::read_lvm_file(temp.path, options);
    check(ds.ok, "windowed load ok");
    check(ds.partial, "windowed load marked partial");
    check(ds.rows() == 2, "windowed load rows");
    check_near(ds.time[0], 0.2, 1e-12, "windowed load first time");
    check_near(ds.time[1], 0.3, 1e-12, "windowed load second time");
    check(ds.channel_count() == 2, "windowed load channel count");
}

void test_data_integrity_regressions() {
    std::printf("test_data_integrity_regressions\n");
    const TempFile blank("missing_time.txt", "0\t10\t20\n\t11\t21\n2\t12\t22\n");
    auto ds = lvm::read_lvm_file(blank.path);
    check(ds.ok && ds.rows() == 2, "missing timestamp is not replaced by channel value");
    if(ds.rows() == 2) check_near(ds.channels[0][1],12,1e-12,"columns remain aligned");
    const TempFile csv("quoted.csv", "\xEF\xBB\xBFTime,\"A, volts\",B\n\"0\",\"10\",\"20\"\n\"1\",\"11\",\"21\"\n");
    ds = lvm::read_lvm_file(csv.path);
    check(ds.ok && ds.rows() == 2 && ds.names == std::vector<std::string>{"A, volts","B"}, "BOM and quoted CSV");
    const TempFile multiline("multiline.csv", "Time,\"line one\nline two\"\n0,10\n1,20\n");
    ds=lvm::read_lvm_file(multiline.path);
    check(ds.ok && ds.rows()==2 && ds.names[0]=="line one\nline two","multiline CSV label");
    const TempFile times("same_times.txt", "0\t10\n1\t11\n11\t12\n0\t13\n1\t14\n");
    ds = lvm::read_lvm_file(times.path); lvm::make_monotonic(ds.time);
    lvm::LoadOptions options; options.use_time_window=true; options.time_start=0; options.time_end=100;
    auto partial=lvm::read_lvm_file(times.path,options);
    check(ds.time == partial.time,"full and window timeline normalization agree");
    double start=0,end=0;std::string error;
    auto index=std::make_shared<lvm::ScanIndex>();
    check(lvm::scan_time_bounds(times.path,start,end,error,nullptr,index.get()),"indexed scan");
    check_near(end,ds.time.back(),1e-12,"scan timeline matches full load");
    options.scan_index=index;options.time_start=1;
    partial=lvm::read_lvm_file(times.path,options);
    check(partial.time==std::vector<double>(ds.time.begin()+1,ds.time.end()),"indexed window agrees");
    auto original=ds.channels;
    lvm::drop_duplicate_time_channels(ds,ds.raw_time);
    check(ds.channels==original,"nonduplicate data survives channel compaction");
    for(const auto* formula:{L"2x",L"x 2",L"x+()",L"1e999",L"sin x",L"x(2)"}) {
        std::vector<FormulaToken> rpn;std::wstring message;
        check(!compile_formula_rpn(formula,rpn,message,true) && rpn.empty() && !message.empty(),"invalid formula rejected atomically");
    }
    const TempFile destination("atomic.txt","previous data");
    check(!lvm::atomic_write_file(destination.path,[](std::ofstream& out){out << "new";return false;}),"failed writer returns failure");
    std::ifstream in(destination.path);std::string text;std::getline(in,text);
    check(text=="previous data","failed writer preserves original");
}

void test_spectrum_integrity_regressions() {
    std::printf("test_spectrum_integrity_regressions\n");
    lvm::Dataset ds;ds.names={"signal"};ds.channels.resize(1);
    for(int i=0;i<32768;++i){double t=double(i)/1024;ds.time.push_back(t);ds.channels[0].push_back(std::sin(2*3.14159265358979323846*400*t));}
    auto spectrum=lvm::compute_spectrum(ds,16384);
    check(spectrum.ok,"capped spectrum works");
    if(spectrum.ok){const auto peaks=lvm::find_peaks(spectrum.freqs,spectrum.amp[0],1);check(!peaks.empty(),"capped peak found");if(!peaks.empty())check_near(peaks[0].freq,400,1e-9,"cap cannot alias 400Hz to 112Hz");}
    ds.channels[0][8]=std::numeric_limits<double>::infinity();
    check(!lvm::compute_spectrum(ds,0).ok,"infinite input cannot produce successful FFT");
    ds.channels[0][8]=0;ds.time[10]+=1;
    check(!lvm::compute_spectrum(ds,0).ok,"invalid time cannot produce successful FFT");
    ds.time[10]-=1;ds.channels[0].pop_back();
    check(!lvm::compute_spectrum(ds,0).ok,"misaligned dataset rejected");
}

void test_fft_irregular_timestamps() {
    std::printf("test_fft_irregular_timestamps\n");
    constexpr double pi = 3.14159265358979323846;
    const auto append_tone = [pi](lvm::Dataset& ds, int count, double start, double dt, double frequency, bool jitter) {
        for (int i = 0; i < count; ++i) {
            const double offset = jitter && i > 0 && i < count - 1 ? 0.12 * dt * std::sin(2 * pi * i / 17) : 0;
            const double t = start + i * dt + offset;
            ds.time.push_back(t);
            ds.channels[0].push_back(std::sin(2 * pi * frequency * (t - start)));
        }
    };
    const auto check_tone = [](const lvm::Spectrum& spec, double frequency, double frequency_tolerance) {
        check(spec.ok, "irregular recording produces FFT");
        if (!spec.ok) return;
        const auto peaks = lvm::find_peaks(spec.freqs, spec.amp[0], 1);
        check(!peaks.empty(), "irregular recording has a peak");
        if (peaks.empty()) return;
        check_near(peaks[0].freq, frequency, frequency_tolerance, "irregular timestamps preserve tone frequency");
        check_near(peaks[0].amp, 1, 0.06, "interpolation preserves low-frequency tone amplitude");
    };

    lvm::Dataset jittered;
    jittered.names = {"signal"}; jittered.channels.resize(1);
    append_tone(jittered, 1024, 0, 1.0 / 1024, 64, true);
    auto spec = lvm::compute_spectrum(jittered, 0);
    check_tone(spec, 64, 1e-9);
    check(spec.resampled && !spec.gaps_ignored && spec.n == 1024, "jitter resamples the whole recording");

    lvm::Dataset rounded;
    rounded.names = {"signal"}; rounded.channels.resize(1);
    const double frequency = 32.0 / (512 * 0.03275);
    append_tone(rounded, 512, 0, 0.03275, frequency, false);
    for (auto& t : rounded.time) t = std::round(t * 1000) / 1000;
    spec = lvm::compute_spectrum(rounded, 0);
    check_tone(spec, frequency, 0.001);
    check(spec.resampled && !spec.gaps_ignored, "millisecond timestamp rounding does not reject FFT");

    lvm::Dataset gapped;
    gapped.names = {"signal"}; gapped.channels.resize(1);
    append_tone(gapped, 128, 0, 1.0 / 1024, 32, false);
    append_tone(gapped, 1024, 10, 1.0 / 1024, 128, false);
    spec = lvm::compute_spectrum(gapped, 0);
    check(spec.ok && spec.gaps_ignored && !spec.resampled && spec.n == 1152, "gap does not discard samples from either side");
    if (spec.ok) {
        const auto peaks = lvm::find_peaks(spec.freqs, spec.amp[0], 2);
        check(peaks.size() == 2, "gap-compressed spectrum retains both signal fragments");
        if (peaks.size() == 2) {
            check_near(peaks[0].freq, 128, 1e-9, "longer fragment is present after a gap");
            check_near(peaks[0].amp, 1024.0 / 1152.0, 0.02, "fragment amplitude reflects all selected samples");
            check_near(peaks[1].freq, 32, 1e-9, "shorter fragment is present after a gap");
            check_near(peaks[1].amp, 128.0 / 1152.0, 0.02, "short fragment is not discarded after a gap");
        }
    }
    check_near(spec.source_start, 0, 1e-12, "full selected range start reported");
    check_near(spec.source_end, gapped.time.back(), 1e-12, "actual FFT source end reported");
    spec = lvm::compute_spectrum(gapped, 256);
    check(spec.ok && spec.n == 256 && spec.gaps_ignored, "sample cap applies to selected samples without discarding the gap rule");
    if (spec.ok) {
        const auto peaks = lvm::find_peaks(spec.freqs, spec.amp[0], 2);
        check(peaks.size() == 2, "capped FFT retains data on both sides of a gap");
        if (peaks.size() == 2) {
            check_near(peaks[0].amp, 0.5, 0.02, "capped FFT includes the first signal fragment");
            check_near(peaks[1].amp, 0.5, 0.02, "capped FFT includes the second signal fragment");
            check(std::abs(peaks[0].freq - 32) < 1e-9 || std::abs(peaks[0].freq - 128) < 1e-9,
                  "first capped peak has a source frequency");
            check(std::abs(peaks[1].freq - 32) < 1e-9 || std::abs(peaks[1].freq - 128) < 1e-9,
                  "second capped peak has a source frequency");
        }
    }
    check_near(spec.source_end, 10 + 127.0 / 1024, 1e-12, "capped FFT reports the selected source window");

    // Large gaps must not allocate a uniform grid spanning the entire gap.
    gapped.time.push_back(1e100); gapped.channels[0].push_back(0);
    spec = lvm::compute_spectrum(gapped, 0);
    check(spec.ok && spec.n == 1153 && spec.gaps_ignored, "huge gap keeps all values and FFT memory bounded");

    lvm::Dataset offset;
    offset.names = {"signal"}; offset.channels.resize(1);
    append_tone(offset, 1024, 1e9, 1.0 / 1024, 64, false);
    spec = lvm::compute_spectrum(offset, 0);
    check_tone(spec, 64, 1e-9);
    check(!spec.resampled && !spec.gaps_ignored, "uniform absolute timestamps do not alter samples");

    // A straight line has an exact interpolation oracle. Simply removing the
    // uniformity check and running FFT on uneven samples fails this comparison.
    lvm::Dataset uneven, uniform;
    uneven.names = uniform.names = {"ramp"}; uneven.channels.resize(1); uniform.channels.resize(1);
    for (int i = 0; i < 16; ++i) {
        const double t = i + (i > 0 && i < 15 ? (i % 2 ? 0.2 : -0.2) : 0);
        uneven.time.push_back(t); uneven.channels[0].push_back(2 * t + 3);
        uniform.time.push_back(i); uniform.channels[0].push_back(2 * i + 3);
    }
    const auto interpolated = lvm::compute_spectrum(uneven, 0);
    const auto reference = lvm::compute_spectrum(uniform, 0);
    check(interpolated.ok && interpolated.resampled && reference.ok, "linear interpolation oracle setup");
    if (interpolated.ok && reference.ok) {
        for (std::size_t k = 0; k < reference.freqs.size(); ++k) {
            check_near(interpolated.amp[0][k], reference.amp[0][k], 1e-10, "resampling matches the exact uniform signal");
        }
    }
    std::atomic<bool> cancelled{true};
    bool cancelled_cleanly = false;
    try { lvm::compute_spectrum(jittered, 0, &cancelled); }
    catch (const std::runtime_error&) { cancelled_cleanly = true; }
    check(cancelled_cleanly, "resampling respects FFT cancellation");
}

void test_fft_gap_regressions() {
    std::printf("test_fft_gap_regressions\n");
    // Compare every bin against the same available values on a compact axis.
    // Non-periodic values make losing or interpolating a fragment observable.
    for (int gap_count : {10, 10000}) {
        for (int pattern = 0; pattern < 4; ++pattern) {
            lvm::Dataset compact, recorded;
            compact.names = recorded.names = {"A", "B"};
            compact.channels.resize(2); recorded.channels.resize(2);
            const int rows = pattern == 0 ? gap_count * 3 + 1 : gap_count + 1001;
            double time = 0;
            int gaps = 0;
            for (int i = 0; i < rows; ++i) {
                const bool gap = i && (pattern == 0 ? i % 3 == 0 : i > 1000);
                if (i) time += (1.0 + (gap ? (pattern == 0 || pattern == 3 ? 1 + gaps % 3 : 1024.0 * (1 + gaps % 7)) : 0.0)) / 1024;
                if (gap) ++gaps;
                compact.time.push_back(i / 1024.0);
                recorded.time.push_back(time);
                for (int c = 0; c < 2; ++c) {
                    const double value = std::sin(i * (0.19 + c * 0.27)) + (i % 31 == 0 ? 3.0 : 0.0);
                    compact.channels[c].push_back(value);
                    recorded.channels[c].push_back(value);
                }
            }
            // Explicit cap is the only permitted source of truncation.
            const int cap = pattern == 2 ? rows - 3 : 0;
            const auto expected = lvm::compute_spectrum(compact, cap);
            const auto actual = lvm::compute_spectrum(recorded, cap);
            check(gaps == gap_count && actual.ok && expected.ok, "gap oracle setup");
            check(actual.gaps_ignored && !actual.resampled, "short and dominant large gaps are compressed without interpolation");
            check(actual.n == expected.n && actual.source_end == recorded.time[expected.n - 1], "all selected values and correct physical bounds retained");
            check(actual.freqs == expected.freqs, "gap-compressed frequency bins match compact reference");
            check(actual.amp == expected.amp, "every amplitude in both channels matches compact reference");
        }
    }

    lvm::Dataset jittered;
    jittered.names = {"A"}; jittered.channels.resize(1);
    for (int i = 0; i < 256; ++i) {
        const double t = (i + 0.1 * std::sin(i * 0.3)) / 1024.0;
        jittered.time.push_back(t); jittered.channels[0].push_back(std::sin(t * 200));
    }
    jittered.time.push_back(1000); jittered.channels[0].push_back(0.4);
    const auto moderate = lvm::compute_spectrum(jittered, 0);
    jittered.time.back() = 1e100;
    const auto huge = lvm::compute_spectrum(jittered, 0);
    check(moderate.ok && huge.ok && moderate.resampled && huge.resampled, "huge gap does not suppress jitter correction");
    check(huge.freqs == moderate.freqs && huge.amp == moderate.amp, "gap duration cannot change the compacted spectrum");

    // Rows beyond an explicit cap must not change sampling inference.
    lvm::Dataset capped;
    capped.names = {"A"}; capped.channels.resize(1);
    for (int i = 0; i < 16; ++i) { capped.time.push_back(i / 1024.0); capped.channels[0].push_back(std::sin(i)); }
    const auto before = lvm::compute_spectrum(capped, 16);
    for (int i = 0; i < 256; ++i) { capped.time.push_back(capped.time.back() + 1e-6); capped.channels[0].push_back(i); }
    const auto after = lvm::compute_spectrum(capped, 16);
    check(after.ok && after.freqs == before.freqs && after.amp == before.amp, "unselected tail cannot affect capped FFT");
}

void test_minmax_and_spectrum_import() {
    std::printf("test_minmax_and_spectrum_import\n");
    std::vector<double> values(1027);
    for (std::size_t i = 0; i < values.size(); ++i) values[i] = std::sin(i * 0.7);
    values[64] = 1e35; values[1026] = -1e35; values[130] = std::nan("");
    const auto sample = [&](std::size_t i) { return values[i]; };
    MinMaxIndex index; index.build(values.size(), sample);
    bool exact = true;
    for (std::size_t lo = 0; lo < values.size(); lo += 13) {
        for (std::size_t hi = lo; hi <= values.size(); hi += 17) {
            double low = std::numeric_limits<double>::infinity(), high = -low;
            for (std::size_t i = lo; i < hi; ++i) if (std::isfinite(values[i])) {
                low = std::min(low, values[i]); high = std::max(high, values[i]);
            }
            const auto range = index.query(lo, hi, sample);
            exact = exact && range.first == low && range.second == high;
        }
    }
    check(exact, "minmax pyramid matches exhaustive finite sample ranges");
    check(index.query(1024, 2000, sample).first == -1e35, "minmax includes last partial block without float overflow");
    const TempFile spectrum("stored.csv", "Frequency,A,B\n0,0,3\n1,1,8\n2,2,5\n");
    auto ds = lvm::read_lvm_file(spectrum.path);
    check(ds.ok && ds.frequency_axis, "stored frequency header recognized");
    lvm::drop_duplicate_time_channels(ds, ds.raw_time);
    check(ds.channels.size() == 2, "amplitude equal to frequency is not discarded as duplicate time");
    const auto spec = lvm::compute_spectrum(ds, 0);
    check(spec.ok && spec.imported && spec.freqs == ds.time && spec.amp == ds.channels, "three-bin spectrum loads without FFT or minimum-four-sample restriction");
    lvm::LoadOptions options; options.use_time_window = true; options.time_start = 1; options.time_end = 2;
    const auto window = lvm::read_lvm_file(spectrum.path, options);
    check(window.ok && window.frequency_axis && window.time == std::vector<double>{1,2}, "spectrum window preserves frequencies");
    const TempFile invalid("invalid_spectrum.csv", "Frequency,A\n0,3\n2,4\n1,5\n");
    check(!lvm::read_lvm_file(invalid.path).ok, "invalid spectrum frequency ordering rejected");
    const TempFile text_spectrum("stored_spectrum.txt", "Frequency\tSensor\n0\t3\n1\t4\n2\t5\n");
    const auto text_ds = lvm::read_lvm_file(text_spectrum.path);
    check(text_ds.ok && text_ds.frequency_axis && text_ds.names == std::vector<std::string>{"Sensor"}, "plain TXT spectrum header recognized without metadata");
    check(lvm::tsv_header_field("A\tB\nC\rD") == "A B C D", "TXT header cannot introduce extra columns or rows");
}

}  // namespace

int main() {
    test_data_integrity_regressions();
    test_spectrum_integrity_regressions();
    test_fft_irregular_timestamps();
    test_fft_gap_regressions();
    test_minmax_and_spectrum_import();
    test_basic_parse();
    test_metadata_and_nan();
    test_decimal_comma();
    test_multi_header();
    test_make_monotonic_equal_times();
    test_make_monotonic_backward_jump();
    test_drop_duplicate_time();
    test_interleaved_channel_names();
    test_reference_test_lvm();
    test_fft_peak();
    test_fft_nyquist_amplitude();
    test_fft_sample_cap_too_small();
    test_missing_file();
    test_formula_engine();
    test_export_helpers();
    test_gap_details();
    test_scan_and_window_load();

    std::printf("\n%d checks, %d failure(s)\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
