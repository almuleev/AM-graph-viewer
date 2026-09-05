#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

namespace lvm {

// Both FFT and the stitched graph use this rule. A single missing sample
// gives a two-step interval; retain some margin for clock jitter/rounding.
inline constexpr double sampling_gap_factor = 1.5;

// Input contains finite positive intervals and is reordered in place.
// Use the short-interval cluster when separated from outages by a factor > 4.
// Shorter gaps can dominate too: accept the first separated lower cluster if
// all intervals are within 5% of integer multiples of its median cadence.
// This works even if outages are the majority. Require two intervals in the
// lower cluster so one near-duplicate timestamp cannot determine the cadence.
// Otherwise retain the median's resistance to clock jitter. Constant spacing
// cannot reveal missing samples without external sample-rate information.
inline double typical_sample_spacing(std::vector<double>& intervals) {
    if (intervals.empty()) return 0.0;
    std::sort(intervals.begin(), intervals.end());
    std::size_t count = intervals.size();
    bool tested_multiples = false;
    for (std::size_t i = 2; i < intervals.size(); ++i) {
        const double separation = intervals[i] / intervals[i - 1];
        if (separation > 4.0) { count = i; break; }
        if (separation > sampling_gap_factor && !tested_multiples) {
            tested_multiples = true;
            const double candidate = i % 2 ? intervals[i / 2]
                : 0.5 * intervals[i / 2] + 0.5 * intervals[i / 2 - 1];
            const bool multiples = std::all_of(intervals.begin(), intervals.end(), [candidate](double interval) {
                const double ratio = interval / candidate;
                return std::isfinite(ratio) && ratio >= 0.95 && std::fabs(ratio - std::round(ratio)) <= 0.05;
            });
            if (multiples) { count = i; break; }
        }
    }
    const std::size_t mid = count / 2;
    return count % 2 ? intervals[mid] : 0.5 * intervals[mid] + 0.5 * intervals[mid - 1];
}

} // namespace lvm
