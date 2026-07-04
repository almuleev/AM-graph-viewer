#pragma once

#include <string>
#include <vector>

#include "gui_processing.hpp"

struct ExportOptions;

void apply_export_metadata_from_comments(const std::vector<std::string>& comments);
bool save_tabular_export(const std::wstring& path, const ExportOptions& opts);
bool save_lvm_export(const std::wstring& path, const ExportOptions& opts);
void save_png_dialog();
bool save_export_file(const std::wstring& path, const ExportOptions& opts);
void save_as_dialog();
