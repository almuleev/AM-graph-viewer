#pragma once
#include "gui_platform.hpp"

namespace gui {

void add_guide_line(bool vertical, double value);

bool px_to_data(int px, int py, double& dx, double& dy);

bool snap_to_nearest_target(double& dx, double& dy, int* out_channel = nullptr);

void snap_to_nearest(double& dx, double& dy);

int hit_test_marker(int px, int py);

LRESULT handle_input_message(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

} // namespace gui
