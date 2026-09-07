#pragma once
#include "gui_platform.hpp"

namespace gui {

int hit_test_gap_marker(int x, int y);

void hide_gap_details_card();

void show_gap_details_card(double duration, long long estimated_missing_samples);

void draw_gap_details_card(HDC dc, const RECT& plot);

} // namespace gui
