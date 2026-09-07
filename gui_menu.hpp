#pragma once
#include "gui_platform.hpp"

namespace gui {

std::wstring menu_text(const wchar_t* base, int command);

void append_menu_popup_owner_draw(HMENU bar, HMENU popup, const std::wstring& text);

void append_menu_item_owner_draw(HMENU menu, UINT id, const std::wstring& text);

void modify_menu_item_owner_draw(HMENU menu, UINT id, const std::wstring& text);

std::wstring menu_item_left_text(const std::wstring& text);

std::wstring menu_item_right_text(const std::wstring& text);

void measure_owner_draw_menu(MEASUREITEMSTRUCT* mis);

void draw_owner_draw_menu(const DRAWITEMSTRUCT* dis);

void sync_menu();

HMENU make_menu();

void rebuild_menu_bar();

} // namespace gui
