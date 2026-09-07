#pragma once
#include "gui_platform.hpp"
#include "gui_state.hpp"

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

void show_hotkeys();

int hotkey_binding_index(int command);

void set_hotkey_binding(int command, BYTE fvirt, WORD key);

HotkeyBinding default_hotkey_for_command(int command);

int find_conflicting_hotkey(BYTE fvirt, WORD key, int except_command);

HACCEL make_accelerators();

void rebuild_accelerators();

bool should_bypass_accelerators();

} // namespace gui
