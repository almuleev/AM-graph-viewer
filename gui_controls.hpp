#pragma once
#include "gui_platform.hpp"

namespace gui {

COLORREF mix_color(COLORREF a, COLORREF b, int weight_b);

void fill_rounded_rect(HDC dc, const RECT& r, COLORREF fill, COLORREF border, int radius);

void draw_button_with_colors(HDC dc, const RECT& r, const wchar_t* txt,
                             COLORREF bg_col, COLORREF border_col, COLORREF text_col,
                             bool pressed);

bool is_channel_checkbox_id(int id);

bool is_side_toggle_id(int id);

bool is_settings_toggle_button_id(int id);

bool is_settings_hotkey_modifier_id(int id);

bool is_settings_checkbox_id(int id);

bool is_welcome_checkbox_id(int id);

bool uses_manual_toggle_state(HWND hwnd);

bool is_toggle_checked(HWND hwnd);

void set_toggle_checked(HWND hwnd, bool checked);

void toggle_checked_state(HWND hwnd);

void draw_themed_check_control(HDC dc, const RECT& r, const wchar_t* txt,
                               bool checked, bool pressed, bool enabled,
                               bool radio, bool compact,
                               COLORREF surface_bg = CLR_INVALID);

void draw_welcome_action_button(HDC dc, const RECT& r, const wchar_t* txt, bool pressed, bool primary, bool outlined);

void redraw_button(HWND btn);

void redraw_window_with_children(HWND hwnd);

void redraw_toolbar_buttons();

void draw_themed_button(HDC dc, const RECT& r, const wchar_t* txt, bool pressed, bool active, bool hover);

} // namespace gui
