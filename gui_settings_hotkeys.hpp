#pragma once
#include "gui_platform.hpp"

namespace gui {

void hotkey_combo_add(HWND combo, const wchar_t* text, WORD key);

void hotkey_combo_add_key(HWND combo, WORD key);

void populate_hotkey_key_combo(HWND combo);

int combo_index_by_key(HWND combo, WORD key);

int settings_selected_hotkey_command(HWND hwnd);

void load_selected_hotkey_controls(HWND hwnd);

void reset_all_hotkeys_to_defaults(HWND hwnd);

void populate_hotkey_list(HWND hwnd);

} // namespace gui
