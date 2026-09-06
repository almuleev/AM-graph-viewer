#pragma once
#include "gui_platform.hpp"

namespace gui {

void clamp_range(double& lo, double& hi, double minb, double maxb, double minw);

bool active_axis(double*& lo, double*& hi, double& minb, double& maxb, double& minw);

void zoom_at(double center_frac, double factor);

void zoom_y_at(double center_frac, double factor);

void zoom_y_amp_at(double center_frac, double factor);

void pan_by(double frac);

bool prepare_plot_drag(int mx, int my);

void pan_y_by(double frac);

void goto_start();

void goto_end();

void reset_view();

} // namespace gui
