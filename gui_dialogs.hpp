#pragma once

#include <string>
#include <windows.h>

#include "export_helpers.hpp"

struct ExportOptions;

void show_styled_info_prompt(HWND owner, const wchar_t* title, const wchar_t* message, bool error);
bool prompt_numeric_value(const wchar_t* title, const wchar_t* label,
                          double& out_value, bool allow_negative = true);
bool prompt_light_mode_window(double range_start, double range_end,
                              double& out_start, double& out_end);
bool prompt_export_options(ExportOptions& out_options);
bool prompt_exact_guide_value(bool vertical, double& out_value);
bool prompt_custom_play_speed(double& out_speed);
