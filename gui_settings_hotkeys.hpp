#pragma once
#include "gui_platform.hpp"
#include "gui_state.hpp"
#include "gui_theme.hpp"

namespace gui {

const HotkeyBinding* find_hotkey_binding(int command);

std::vector<HotkeyBinding> default_hotkeys();

void ensure_hotkeys_initialized();

std::wstring key_name(WORD key);

struct HotkeysDialogState {
    HWND wnd = nullptr;
    HWND list = nullptr;
};

extern HotkeysDialogState g_hotkeys_dialog;

std::wstring hotkey_text(BYTE fvirt, WORD key);

std::wstring hotkey_text_for_command(int command);

std::wstring hotkey_display_text_for_command(int command);

std::wstring menu_text(const wchar_t* base, int command);

std::wstring command_name(int command);

std::vector<int> hotkey_command_order();

std::wstring hotkey_list_item_text(int command);

void append_hotkey_line(std::wstring& out, int command);

std::wstring hotkeys_body_text();

const wchar_t* hotkeys_dialog_close_text();

std::vector<std::wstring> hotkeys_dialog_lines();

SIZE hotkeys_dialog_client_size();

void populate_hotkeys_dialog_list(HWND list);

LRESULT CALLBACK HotkeysDialogProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

std::wstring toolbar_hover_text(HWND btn);

int hit_test_gap_marker(int x, int y);

void hide_gap_details_card();

void show_gap_details_card(double duration, long long estimated_missing_samples);

void draw_gap_details_card(HDC dc, const RECT& plot);

void append_menu_popup_owner_draw(HMENU bar, HMENU popup, const std::wstring& text);

void append_menu_item_owner_draw(HMENU menu, UINT id, const std::wstring& text);

void modify_menu_item_owner_draw(HMENU menu, UINT id, const std::wstring& text);

std::wstring menu_item_left_text(const std::wstring& text);

std::wstring menu_item_right_text(const std::wstring& text);

void measure_owner_draw_menu(MEASUREITEMSTRUCT* mis);

void draw_owner_draw_menu(const DRAWITEMSTRUCT* dis);

void measure_settings_combo_item(MEASUREITEMSTRUCT* mis);

void draw_settings_combo_item(const DRAWITEMSTRUCT* dis);

void measure_settings_list_item(MEASUREITEMSTRUCT* mis);

void draw_settings_list_item(const DRAWITEMSTRUCT* dis);

void draw_settings_group_box(HDC dc, const RECT& r, const wchar_t* text);

const wchar_t* settings_button_text();

const wchar_t* channel_show_all_text();

const wchar_t* channel_hide_all_text();

void set_all_channels_visible(bool visible);

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

void refresh_theme_windows();

void apply_theme_choice(const Theme* theme);

void draw_themed_button(HDC dc, const RECT& r, const wchar_t* txt, bool pressed, bool active, bool hover);

void show_hotkeys();

void show_about();

void sync_menu();

void set_mode(bool freq_mode);

HMENU make_menu();

extern COLORREF g_custom_colors[16];

const wchar_t* settings_window_title();

int hotkey_binding_index(int command);

void set_hotkey_binding(int command, BYTE fvirt, WORD key);

HotkeyBinding default_hotkey_for_command(int command);

int find_conflicting_hotkey(BYTE fvirt, WORD key, int except_command);

void hotkey_combo_add(HWND combo, const wchar_t* text, WORD key);

void hotkey_combo_add_key(HWND combo, WORD key);

void populate_hotkey_key_combo(HWND combo);

int combo_index_by_key(HWND combo, WORD key);

int settings_selected_hotkey_command(HWND hwnd);

void load_selected_hotkey_controls(HWND hwnd);

void reset_all_hotkeys_to_defaults(HWND hwnd);

void populate_hotkey_list(HWND hwnd);

bool read_formula_edit(HWND edit, std::wstring& formula, std::vector<FormulaToken>& compiled, std::wstring& error);

void assign_formula_to_channel(std::size_t channel_index, const std::wstring& formula, const std::vector<FormulaToken>& compiled);

void assign_global_formula(const std::wstring& formula, const std::vector<FormulaToken>& compiled);

void refresh_settings_controls();

void rebuild_menu_bar();

LRESULT CALLBACK SettingsProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

void open_settings();

HACCEL make_accelerators();

void rebuild_accelerators();

bool should_bypass_accelerators();

} // namespace gui
