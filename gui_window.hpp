#pragma once
#include "gui_platform.hpp"

namespace gui {

void rebuild_ui();

LRESULT handle_window_message(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

} // namespace gui
