#pragma once
#include "gui_platform.hpp"

namespace gui {

void invalidate_plot_analysis_cache();

// ---- series ---------------------------------------------------------------

struct ChannelRenderView {
    const std::vector<double>* raw = nullptr;
    const std::vector<double>* cache = nullptr;
    TransformRuntimeKind kind = TransformRuntimeKind::Identity;
    double mul = 1.0;
    double add = 0.0;
};

ChannelRenderView make_channel_render_view(std::size_t channel_index);

inline double channel_render_value(const ChannelRenderView& view, std::size_t sample_index) {
    if (!view.raw || sample_index >= view.raw->size()) return std::nan("");
    double value;
    if (view.cache) {
        value = (*view.cache)[sample_index];
    } else {
        const double raw = (*view.raw)[sample_index];
        value = (view.kind == TransformRuntimeKind::Affine) ? (raw * view.mul + view.add) : raw;
    }
    return value;
}

const MinMaxIndex& channel_envelope(std::size_t channel, const ChannelRenderView& view);

bool any_visible_channel();

bool current_time_yrange_window(std::size_t lo, std::size_t hi, double& ymin, double& ymax);

bool current_time_yrange(double& ymin, double& ymax);

bool current_freq_yrange(double& ymin, double& ymax);

double visible_spectrum_ymax();

} // namespace gui
