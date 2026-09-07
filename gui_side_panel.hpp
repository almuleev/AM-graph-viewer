#pragma once
#include "gui_platform.hpp"

namespace gui {

extern WNDPROC g_channel_edit_proc;

void finish_channel_rename(bool apply);

LRESULT CALLBACK ChannelEditProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

void finish_channel_rename_if_click_outside(HWND hwnd);

void start_channel_rename(int ci);

void destroy_checks();

void rebuild_checks();

void hide_ui_controls();

void show_ui_controls();

bool welcome_visible();

int side_panel_width();

int side_selected_point_group();

void populate_filter_mode_combo(HWND combo);

void populate_filter_topology_combo(HWND combo);

void expand_combo_dropdown(HWND combo);

void sync_filter_controls_from_state();

void load_side_transform_controls();

void load_side_point_group_controls();

void populate_side_point_group_list();

bool side_panel_hit_test(const POINT& pt);

void update_side_panel_scrollbar(int viewport_top, int content_height);

void scroll_side_panel(int delta);

void set_side_panel_tab(int tab);

void apply_side_panel_visibility();

void refresh_side_panel_controls();

const wchar_t* channel_show_all_text();

const wchar_t* channel_hide_all_text();

void set_all_channels_visible(bool visible);

} // namespace gui
