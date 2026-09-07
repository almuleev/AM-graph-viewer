#pragma once
#include "gui_platform.hpp"

namespace gui {

bool marker_status_detail(std::wstring& text, COLORREF& color);

void set_status();

std::wstring toolbar_hover_text(HWND btn);

void status_msg(const std::wstring& m);

} // namespace gui
