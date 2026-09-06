#pragma once
#include "gui_platform.hpp"

namespace gui {

double precompute_global_gap_step(const std::vector<double>& time);

void ensure_global_gap_step_ready();

void invalidate_stitched_time_cache();

void ensure_stitched_time_cache();

double stitched_time_at_index(std::size_t index);

double stitched_time_from_raw(double time);

double raw_time_from_stitched(double time);

} // namespace gui
