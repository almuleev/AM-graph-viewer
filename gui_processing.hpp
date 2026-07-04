#pragma once

#include <fstream>
#include <string>
#include <vector>

#include "export_helpers.hpp"
#include "formula_engine.hpp"
#include "lvm_parser.hpp"

struct ExportOptions;

void invalidate_formula_runtime();
void invalidate_formula_runtime_channel(std::size_t channel_index);
void invalidate_transformed_channel_cache();
void invalidate_filtered_channel_cache();
void ensure_channel_formula_storage();
void ensure_channel_formulas_loaded();
void ensure_channel_formula_vectors();
void normalize_filter_bounds();
void ensure_transformed_channel_cache(std::size_t channel_index);
void ensure_filtered_channel_cache(std::size_t channel_index);
void reset_channel_transform(std::size_t ci);
void reset_all_channel_transforms();
void clear_transform_sensitive_overlays(bool clear_history = true);
void on_signal_transform_changed(bool preserve_history = false);
bool build_export_spectrum(const ExportOptions& opts, lvm::Spectrum& out_spec,
                           std::wstring& error, bool& has_signal);
void write_export_comment(std::ofstream& out, const std::wstring& text, const char* line_end);
void write_export_key_value(std::ofstream& out, const std::wstring& key,
                            const std::wstring& value, const char* line_end);
bool export_range_bounds_for_mode(ExportRangeMode range, double& start, double& end, bool& actual_selected);
void write_export_metadata(std::ofstream& out, const ExportOptions& opts, const char* line_end,
                           const std::wstring& time_label, const std::wstring& freq_label);
