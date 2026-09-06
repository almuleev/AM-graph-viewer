#pragma once
#include "gui_platform.hpp"
#include "gui_state_history.hpp"

namespace gui {

std::wstring channel_display_label(std::size_t ci);

void invalidate_formula_runtime();

void invalidate_formula_runtime_channel(std::size_t channel_index);

void invalidate_transformed_channel_cache();

void invalidate_filtered_channel_cache();

void ensure_channel_formula_storage();

void ensure_channel_formulas_loaded();

void ensure_channel_formula_vectors();

void ensure_transformed_channel_cache(std::size_t channel_index);

double transform_channel_value(std::size_t ci, double raw);

std::wstring format_edit_number(double value);

std::wstring format_optional_edit_number(double value);

bool parse_wide_double_text(const wchar_t* text, double& out);

inline constexpr double kFilterSliderRange = 1000.0;

inline constexpr double kFilterSliderGamma = 4.0;

double current_filter_sample_step();

double current_filter_nyquist();

double clamp_filter_cutoff(double hz, double nyquist);

void normalize_filter_bounds();

double filter_slider_to_frequency(int pos, double nyquist);

int frequency_to_filter_slider(double hz, double nyquist);

std::wstring filter_frequency_text(double hz);

double transformed_channel_sample(std::size_t channel_index, std::size_t row_index);

double rendered_channel_sample(std::size_t channel_index, std::size_t row_index);

void ensure_filtered_channel_cache(std::size_t channel_index);

void reset_channel_transform(std::size_t ci);

void reset_all_channel_transforms();

void clear_transform_sensitive_overlays(bool clear_history = true);

void on_signal_transform_changed(bool preserve_history = false);

void commit_filter_settings_change(const SettingsSnapshot& before);

void apply_filter_slider_change(bool low_cutoff, int position, bool preview);

} // namespace gui
