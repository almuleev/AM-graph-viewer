#pragma once
#include "gui_platform.hpp"

namespace gui {

extern lvm::SpectrumWorker g_spectrum_worker;

bool has_fft_window();

void clear_fft_window();

void clamp_time_window(double& start, double& end);

void set_fft_window(double start, double end);

bool current_fft_source_window(double& start, double& end, bool& from_selection);

bool fft_window_contains_time(double t);

bool last_fft_source_window(double& start, double& end, bool& from_selection);

std::wstring fft_window_status(double start, double end, bool from_selection);

std::wstring spectrum_sampling_status(const lvm::Spectrum& spec);

void clear_spectrum_cache_state();

void refresh_spec_channel_indices();

void apply_spectrum_result(lvm::Spectrum spectrum);

bool spectrum_needs_visible_channels();

bool build_time_window_dataset(const lvm::Dataset& in, double start, double end, lvm::Dataset& out,
                               const std::vector<std::size_t>* selected_channels = nullptr, bool apply_processing = true);

void compute_spectrum_for_window(double start, double end, bool from_selection);

void compute_spectrum_from_current_source();

void compute_spectrum();

bool ensure_current_spectrum();

} // namespace gui
