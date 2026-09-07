#pragma once
#include "gui_platform.hpp"

namespace gui {

void measure_settings_combo_item(MEASUREITEMSTRUCT* mis);

void draw_settings_combo_item(const DRAWITEMSTRUCT* dis);

void measure_settings_list_item(MEASUREITEMSTRUCT* mis);

void draw_settings_list_item(const DRAWITEMSTRUCT* dis);

void draw_settings_group_box(HDC dc, const RECT& r, const wchar_t* text);

const wchar_t* settings_button_text();

const wchar_t* settings_window_title();

void refresh_settings_controls();

LRESULT CALLBACK SettingsProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

void open_settings();

} // namespace gui
