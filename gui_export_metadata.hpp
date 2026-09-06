#pragma once
#include "gui_platform.hpp"
#include "gui_dialogs.hpp"
#include "gui_state.hpp"

namespace gui {

int png_encoder_clsid(CLSID* clsid);

bool save_png(const std::wstring& path);

double export_channel_sample(std::size_t channel_index, std::size_t row_index, bool apply_processing);

std::wstring export_channel_label_text(std::size_t channel_index, bool include_names);

std::vector<std::size_t> export_channel_indices(bool include_hidden_channels);

std::vector<int> export_spectrum_channel_indices(const lvm::Spectrum& spec);

bool build_export_spectrum(const ExportOptions& opts, lvm::Spectrum& out_spec,
                           std::vector<int>& out_channel_indices, bool& out_all_channels);

const wchar_t* export_filter_mode_key(int mode);

const wchar_t* export_filter_topology_key(int topology);

std::wstring export_color_triplet(COLORREF color);

std::wstring export_point_display_text(const PointDisplay& display);

void write_export_comment(std::ofstream& out, const std::wstring& text, const char* line_end);

void write_export_key_value(std::ofstream& out, const std::wstring& key, const std::wstring& value, const char* line_end);

const wchar_t* export_range_mode_key(ExportRangeMode range);

const wchar_t* export_processing_mode_key(bool apply_processing);

bool export_range_bounds_for_mode(ExportRangeMode range, double& start, double& end, bool& actual_selected);

std::wstring export_metadata_text(const std::wstring& value);

void write_export_metadata(std::ofstream& out,
                           const ExportOptions& opts,
                           bool csv,
                           double range_start,
                           double range_end,
                           bool actual_selected_range,
                           const std::vector<std::size_t>& exported_channels);

void apply_export_metadata_from_comments(const std::vector<std::string>& comments);

bool write_tabular_export(std::ofstream& out, const ExportOptions& opts);

bool save_tabular_export(const std::wstring& path, const ExportOptions& opts);

bool write_lvm_export(std::ofstream& out, const ExportOptions& opts);

bool save_lvm_export(const std::wstring& path, const ExportOptions& opts);

bool save_dialog(std::wstring& out_path, const wchar_t* filter, const wchar_t* defext,
                 const std::wstring& defname);

std::wstring file_stem();

void status_msg(const std::wstring& m);

void save_png_dialog();

const wchar_t* export_file_extension(ExportFileFormat format);

const wchar_t* export_file_filter(ExportFileFormat format);

const wchar_t* export_file_name(ExportFileFormat format);

bool save_export_file(const std::wstring& path, const ExportOptions& opts);

void save_as_dialog();

std::wstring lvm_current_date_text(const SYSTEMTIME& st);

std::wstring lvm_current_time_text(const SYSTEMTIME& st);

double lvm_export_nominal_delta_x();

} // namespace gui
