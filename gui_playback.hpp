#pragma once
#include "gui_platform.hpp"

namespace gui {

const wchar_t* speed_menu_text();

void set_play_speed(double speed);

void stop_play();

void start_play();

void toggle_play();

LRESULT handle_playback_message(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

} // namespace gui
