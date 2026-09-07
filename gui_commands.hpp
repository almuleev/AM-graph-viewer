#pragma once
#include "gui_platform.hpp"

namespace gui {

enum class AnalysisMode;

LRESULT handle_commands_message(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

void set_mode(AnalysisMode mode);

} // namespace gui
