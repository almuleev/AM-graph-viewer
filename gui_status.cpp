// Status: native viewer implementation.
#include "gui_status.hpp"
#include "gui_ids.hpp"
#include "gui_spectrum.hpp"
#include "gui_state.hpp"
#include "gui_state_history.hpp"
#include "gui_text.hpp"
#include "gui_theme.hpp"

namespace gui {

bool marker_status_detail(std::wstring& text, COLORREF& color) {
    if (g.active_marker < 0 || g.active_marker >= static_cast<int>(g.markers.size())) return false;
    const App::Marker& m = g.markers[static_cast<std::size_t>(g.active_marker)];
    if (m.freq != g.freq_mode || !m.snapped || m.channel < 0) return false;
    color = channel_color(static_cast<std::size_t>(m.channel));
    wchar_t buf[160];
    if (g.freq_mode) {
        if (g_str == &kEn) swprintf(buf, 160, L"   |   %ls: f=%.6g Hz, amp=%.6g", m.label.c_str(), m.x, m.y);
        else swprintf(buf, 160, L"   |   %ls: f=%.6g Гц, amp=%.6g", m.label.c_str(), m.x, m.y);
    } else {
        if (g_str == &kEn) swprintf(buf, 160, L"   |   %ls: t=%.6g s, y=%.6g", m.label.c_str(), m.x, m.y);
        else swprintf(buf, 160, L"   |   %ls: t=%.6g c, y=%.6g", m.label.c_str(), m.x, m.y);
    }
    text = buf;
    return true;
}

void set_status() {
    std::wstring s;
    wchar_t buf[512];
    g.status_detail_text.clear();
    g.status_detail_color = g_theme->accent;
    if (!has_data()) {
        s = g_str->msg_nodata;
    } else if (g.freq_mode) {
        swprintf(buf, 512,
                 g.ds.frequency_axis ? (g_str == &kEn ? L"Stored spectrum: %zu channels, upper frequency %.6g Hz | %.6g..%.6g Hz"
                    : L"Загруженный спектр: %zu каналов, верхняя частота %.6g Гц | %.6g..%.6g Гц") : g_str->st_hz,
                 g.ds.channel_count(), g.spec_valid ? g.spec.nyquist : 0.0,
                 g.freq_start, g.freq_end);
        s = buf;
    } else {
        swprintf(buf, 512,
                 g_str->st_time,
                 g.ds.channel_count(), g.ds.rows(), g.win_start, g.win_end);
        s = buf;
        s += g.auto_y ? g_str->st_yauto : g_str->st_yfix;
        if (g.visual_smooth) s += g_str->st_spline;
        if (g.stitch_time_gaps) s += g_str == &kEn ? L"   |   Time gaps stitched" : L"   |   Пропуски времени склеены";
    }
    if (has_data()) {
        double fft_start, fft_end;
        bool from_selection = false;
        if (g.freq_mode) {
            if (!g.ds.frequency_axis && last_fft_source_window(fft_start, fft_end, from_selection)) {
                s += fft_window_status(fft_start, fft_end, from_selection);
            }
            if (g.spec_valid) s += spectrum_sampling_status(g.spec);
        } else if (has_fft_window()) {
            s += fft_window_status(g.fft_window_start, g.fft_window_end, true);
        }
        if (g.current_file_partial) s += g_str->light_mode_status;
    }
    if (has_data()) {
        std::size_t nlines = 0;
        for (const auto& gl : g.guides)
            if (gl.freq == g.freq_mode) ++nlines;
        if (nlines) { swprintf(buf, 512, g_str->st_lines, nlines); s += buf; }
        std::size_t nmark = 0;
        for (const auto& m : g.markers)
            if (m.freq == g.freq_mode) ++nmark;
        if (nmark) { swprintf(buf, 512, g_str->st_markers, nmark); s += buf; }
    }
    if (g.playing) {
        swprintf(buf, 512, g_str->st_speed, g.play_speed);
        s += buf;
    }
    s += measure_points_status_text();
    const PointGroup* active_group = active_point_group_readonly();
    if (active_group && active_group->points.size() >= 2) {
        const auto& pts = active_group->points;
        const auto& a = pts[pts.size() - 2];
        const auto& b = pts.back();
        const double dx = b.first - a.first, dy = b.second - a.second;
        if (g.freq_mode) {
            swprintf(buf, 512, g_str->msg_delta_f, dx, dy);
        } else {
            const double inv = (dx != 0.0) ? 1.0 / dx : 0.0;
            swprintf(buf, 512, g_str->msg_delta_t, dx, dy, inv);
        }
        s += buf;
    }
    marker_status_detail(g.status_detail_text, g.status_detail_color);
    g.status_text = s;
    if (g.status) SetWindowTextW(g.status, s.c_str());
    if (g.main) {
        RECT rc;
        GetClientRect(g.main, &rc);
        RECT sr = {0, rc.bottom - kBottomBar, rc.right, rc.bottom};
        InvalidateRect(g.main, &sr, FALSE);
    }
}

} // namespace gui
