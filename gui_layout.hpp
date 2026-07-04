#pragma once

#include <windows.h>

void layout();
void apply_side_panel_visibility();
void set_side_panel_tab(int tab);
int side_panel_width();
void update_side_panel_scrollbar(int viewport_top, int content_height);
bool side_panel_hit_test(const POINT& pt);
void scroll_side_panel(int delta);
void load_side_transform_controls();
