#include "gui_frf_render.hpp"
#include "gui_frf.hpp"
#include "gui_input.hpp"
#include "gui_side_panel.hpp"
#include "gui_state_history.hpp"
#include "gui_state.hpp"
#include "gui_render.hpp"
#include "gui_layout.hpp"
#include "gui_navigation.hpp"
#include "gui_status.hpp"
#include "gui_menu.hpp"
#include "gui_ids.hpp"
#include "gui_text.hpp"
#include "gui_theme.hpp"

namespace gui {
namespace {
double dynamic_coefficient(const lvm::FrfResult& r, std::size_t k) {
    return lvm::frf_dynamic_coefficient(r, k);
}
std::vector<double> display_coefficients(const lvm::FrfResult& r) {
    const std::size_t n=r.frequencies.size();
    std::vector<double> values(n,std::numeric_limits<double>::quiet_NaN());
    if (n<2) return values;
    if (g.frf.display_smoothing_octaves<=0) {
        for (std::size_t k=1;k<n;++k) values[k]=dynamic_coefficient(r,k);
        return values;
    }
    std::vector<double> sums(n+1,0); std::vector<std::size_t> counts(n+1,0);
    for (std::size_t k=1;k<n;++k) {
        sums[k+1]=sums[k]; counts[k+1]=counts[k];
        const double value=dynamic_coefficient(r,k);
        if (std::isfinite(value)) { sums[k+1]+=value; ++counts[k+1]; }
    }
    const double span=std::pow(2.0,g.frf.display_smoothing_octaves/2);
    std::size_t lo=1,hi=1;
    for (std::size_t k=1;k<n;++k) {
        if (!std::isfinite(dynamic_coefficient(r,k))) continue;
        const double low=r.frequencies[k]/span, high=r.frequencies[k]*span;
        while (lo<n && r.frequencies[lo]<low) ++lo;
        while (hi<n && r.frequencies[hi]<=high) ++hi;
        const auto count=counts[hi]-counts[lo];
        if (count) values[k]=(sums[hi]-sums[lo])/count;
    }
    return values;
}
void line(HDC dc, int x0, int y0, int x1, int y1) {
    MoveToEx(dc, x0, y0, nullptr); LineTo(dc, x1, y1);
}
void changed() {
    // Pan and zoom only change the graph and its axis labels. Do not invalidate
    // the bottom status bar: it contains text that does not change on zoom.
    RECT dirty=plot_rect();
    dirty.bottom+=kAxisBottom;
    InvalidateRect(g.main,&dirty,FALSE);
}
}

double frf_frequency_at_fraction(double t) {
    return std::pow(10.0, g.frf.log_start + t * (g.frf.log_end - g.frf.log_start));
}
double frf_frequency_fraction(double f) {
    if (!(f > 0) || !(g.frf.log_end > g.frf.log_start)) return std::numeric_limits<double>::quiet_NaN();
    return (std::log10(f) - g.frf.log_start) / (g.frf.log_end - g.frf.log_start);
}
void frf_y_range(double& low, double& high) {
    low = g.frf.y_min; high = g.frf.y_max;
    if (!g.frf.auto_y) return;
    low = std::numeric_limits<double>::infinity(); high = -low;
    const double a = frf_frequency_at_fraction(0), b = frf_frequency_at_fraction(1);
    for (const auto& r:g.frf.result.responses) {
        if (!r.ok) continue;
        const auto values=display_coefficients(r);
        for (std::size_t k = 1; k < r.frequencies.size(); ++k) {
            if (r.frequencies[k] < a || r.frequencies[k] > b) continue;
            const double value = values[k];
            if (!std::isfinite(value)) continue;
            low = std::min(low, value); high = std::max(high, value);
        }
    }
    if (!std::isfinite(low)) { low = 0; high = 1; return; }
    const double pad = std::max(0.05, (high - low) * 0.08);
    low = std::max(0.0, low - pad); high += pad;
}

void draw_frf(HDC dc, const RECT& p) {
    g_legend_items.clear(); g_legend_box = {};
    g.visible_gap_markers.clear();
    HBRUSH bg = CreateSolidBrush(g_theme->bg_plot);
    FillRect(dc, &p, bg); DeleteObject(bg);
    SetTextAlign(dc, TA_LEFT | TA_TOP);
    SetBkMode(dc, TRANSPARENT); SetTextColor(dc, g_theme->axis_text);
    HGDIOBJ font = SelectObject(dc, g.ui_font ? g.ui_font : GetStockObject(DEFAULT_GUI_FONT));
    if (!g.frf.result.ok || g.frf.pending) {
        RECT r = p; InflateRect(&r, -24, -24);
        std::wstring text = frf_status_text();
        if (text.empty()) text = g_str == &kEn ? L"Select Supports and Responses, then Calculate." : L"Выберите опоры и отклики и нажмите «Рассчитать».";
        DrawTextW(dc, text.c_str(), -1, &r, DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
        SelectObject(dc, font); g.vvalid = false; return;
    }
    double low, high; frf_y_range(low, high);
    const int width = p.right - p.left, height = p.bottom - p.top;
    if (width <= 0 || height <= 0 || !(high > low)) { SelectObject(dc, font); return; }
    const auto mapx = [&](double f) { return p.left + static_cast<int>(std::clamp(frf_frequency_fraction(f), -1.0, 2.0) * width); };
    const auto mapy = [&](double v) { return p.bottom - static_cast<int>(std::clamp((v - low) / (high - low), -1.0, 2.0) * height); };
    HPEN grid = CreatePen(PS_SOLID, 1, g_theme->grid);
    HGDIOBJ old_pen = SelectObject(dc, grid);
    const double f0 = frf_frequency_at_fraction(0), f1 = frf_frequency_at_fraction(1);
    // Decades with 2/5 subdivisions; labels adapt to the visible span.
    int previous_label_x = -10000;
    for (int decade = static_cast<int>(std::floor(g.frf.log_start)); decade <= static_cast<int>(std::ceil(g.frf.log_end)); ++decade) {
        const double base = std::pow(10.0, decade);
        for (double multiple : {1.0, 2.0, 5.0}) {
            const double f = base * multiple;
            if (f < f0 || f > f1) continue;
            const int x = mapx(f); line(dc, x, p.top, x, p.bottom);
            if (x - previous_label_x >= 54) {
                wchar_t text[48]; swprintf(text, 48, L"%.5g", f);
                RECT label{x-36, p.bottom+4, x+36, p.bottom+23};
                DrawTextW(dc, text, -1, &label, DT_CENTER | DT_SINGLELINE | DT_NOPREFIX);
                previous_label_x = x;
            }
        }
    }
    // A narrow zoom may contain no decade ticks: label the visible bounds.
    if (previous_label_x == -10000) {
        for (int i = 0; i <= 4; ++i) {
            const double f = frf_frequency_at_fraction(i / 4.0);
            const int x = mapx(f); line(dc, x, p.top, x, p.bottom);
            wchar_t text[48]; swprintf(text, 48, L"%.5g", f);
            RECT label{x-36, p.bottom+4, x+36, p.bottom+23};
            DrawTextW(dc, text, -1, &label, DT_CENTER | DT_SINGLELINE | DT_NOPREFIX);
        }
    }
    const double raw_step = (high - low) / 6;
    const double base = std::pow(10.0, std::floor(std::log10(raw_step)));
    const double ratio = raw_step / base;
    const double step = base * (ratio <= 1 ? 1 : ratio <= 2 ? 2 : ratio <= 5 ? 5 : 10);
    for (double v = std::ceil(low / step) * step; v <= high; v += step) {
        const int y = mapy(v); line(dc, p.left, y, p.right, y);
        wchar_t text[48]; swprintf(text, 48, L"%.5g", v);
        RECT label{0, y-10, p.left-8, y+10};
        DrawTextW(dc, text, -1, &label, DT_RIGHT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
    }
    SelectObject(dc, old_pen); DeleteObject(grid);
    HPEN frame = CreatePen(PS_SOLID, 1, g_theme->frame);
    old_pen = SelectObject(dc, frame);
    line(dc, p.left, p.top, p.left, p.bottom);
    line(dc, p.left, p.bottom, p.right, p.bottom);
    SelectObject(dc, old_pen); DeleteObject(frame);

    const int saved = SaveDC(dc);
    IntersectClipRect(dc, p.left+1, p.top+1, p.right, p.bottom);
    for (std::size_t response=0;response<g.frf.result.responses.size();++response) {
        const auto& r=g.frf.result.responses[response];
        if (!r.ok) continue;
        const COLORREF color=channel_color(g.frf.outputs[response]);
        HPEN curve = CreatePen(PS_SOLID, 1, color);
        old_pen = SelectObject(dc, curve);
        const std::size_t begin = std::max<std::size_t>(1, static_cast<std::size_t>(
            std::lower_bound(r.frequencies.begin(), r.frequencies.end(), f0) - r.frequencies.begin()));
        const std::size_t end = static_cast<std::size_t>(
            std::upper_bound(r.frequencies.begin(), r.frequencies.end(), f1) - r.frequencies.begin());
        const auto values=display_coefficients(r);
        bool started = false;
        // Use one mean value per screen column instead of a min/max whisker.
        // Invalid bins still break the curve rather than being joined across.
        int column = -1; double sum_y=0; std::size_t count_y=0;
        auto flush = [&] {
            if (column < 0) return;
            const int y=static_cast<int>(std::lround(sum_y/count_y));
            if (started) LineTo(dc,column,y); else MoveToEx(dc,column,y,nullptr);
            started = true;
        };
        for (std::size_t k = begin; k < end; ++k) {
            const double v = values[k];
            if (!std::isfinite(v)) { flush(); column = -1; started = false; continue; }
            const int x = mapx(r.frequencies[k]), y = mapy(v);
            if (x != column) { flush(); column=x; sum_y=y; count_y=1; }
            else { sum_y+=y; ++count_y; }
        }
        flush();
        SelectObject(dc, old_pen); DeleteObject(curve);
    }
    RestoreDC(dc, saved);
    RECT xlabel{p.left, p.bottom+23, p.right, p.bottom+42};
    DrawTextW(dc, L"Frequency, Hz (log)", -1, &xlabel, DT_CENTER | DT_SINGLELINE);
    RECT ylabel{4, p.top+2, 34, p.top+22};
    DrawTextW(dc, L"КД", -1, &ylabel, DT_LEFT | DT_SINGLELINE);
    SelectObject(dc, font);
    g.vx0 = g.frf.log_start; g.vx1 = g.frf.log_end;
    g.vy0 = low; g.vy1 = high; g.vrect = p; g.vvalid = true;
    draw_guides(dc);
    draw_markers(dc);
    draw_measure(dc);
}

LRESULT handle_frf_input(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    const RECT p = plot_rect();
    const auto inside = [&](int x, int y) { return x >= p.left && x <= p.right && y >= p.top && y <= p.bottom; };
    switch (msg) {
        case WM_MOUSEWHEEL: {
            POINT pt{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)}; ScreenToClient(hwnd, &pt);
            if (!inside(pt.x, pt.y) || !g.frf.result.ok) return 0;
            const bool up = GET_WHEEL_DELTA_WPARAM(wp) > 0;
            if (GetKeyState(VK_SHIFT) & 0x8000) pan_by(up ? -.1 : .1);
            else if (GetKeyState(VK_CONTROL) & 0x8000) zoom_y_at(
                static_cast<double>(p.bottom-pt.y)/(p.bottom-p.top), up ? .85 : 1/.85);
            else if (GetKeyState(VK_MENU) & 0x8000) pan_y_by(up ? -.1 : .1);
            else zoom_at(static_cast<double>(pt.x-p.left)/(p.right-p.left), up ? .8 : 1.25);
            changed(); return 0;
        }
        case WM_LBUTTONDOWN:
            if (inside(GET_X_LPARAM(lp),GET_Y_LPARAM(lp)) && (g.pending_line || g.pending_marker || g.measure_mode) && g.vvalid) {
                double frequency=0, coefficient=0;
                if (!px_to_data(GET_X_LPARAM(lp),GET_Y_LPARAM(lp),frequency,coefficient)) return 0;
                if (g.pending_line) {
                    GuideLine line;
                    line.vertical=g.pending_line==1; line.value=line.vertical ? frequency : coefficient; line.mode=AnalysisMode::FRF;
                    g.guides.push_back(line);
                    UndoAction action; action.type=UndoAction::ADD_LINE; action.line=line; push_undo(action);
                    g.pending_line=0;
                } else if (g.pending_marker) {
                    App::Marker marker;
                    int channel=-1;
                    const bool snapped=g.snap_to_data && snap_to_nearest_target(frequency,coefficient,&channel);
                    marker.x=frequency; marker.y=coefficient; marker.freq=true;
                    marker.mode=AnalysisMode::FRF; marker.snapped=snapped; marker.channel=snapped ? channel : -1;
                    wchar_t label[16]{}; swprintf(label,16,L"M%zu",g.markers.size()+1);
                    marker.label=label;
                    g.markers.push_back(marker);
                    g.active_marker=static_cast<int>(g.markers.size())-1;
                    UndoAction action; action.type=UndoAction::ADD_MARKER; action.marker=marker; push_undo(action);
                    g.pending_marker=false;
                } else if (g.measure_mode) {
                    if (g.snap_to_data) snap_to_nearest(frequency,coefficient);
                    bool created=false;
                    const int group=ensure_point_group_for_measurement((GetKeyState(VK_CONTROL)&0x8000)!=0,&created);
                    if (group>=0) {
                        g.point_groups[static_cast<std::size_t>(group)].points.push_back({frequency,coefficient});
                        UndoAction action; action.type=UndoAction::ADD_POINT; action.point={frequency,coefficient};
                        action.point_group_index=group; action.point_group_created=created;
                        action.point_group_state=g.point_groups[static_cast<std::size_t>(group)]; action.point_group_state.points.clear();
                        push_undo(action); refresh_side_panel_controls();
                    }
                }
                set_status(); sync_menu(); invalidate_plot(); return 0;
            }
            if (inside(GET_X_LPARAM(lp), GET_Y_LPARAM(lp)) && g.frf.result.ok &&
                prepare_plot_drag(GET_X_LPARAM(lp), GET_Y_LPARAM(lp))) { g.dragging = true; SetCapture(hwnd); }
            return 0;
        case WM_MOUSEMOVE: {
            if (g.dragging) {
                double *lo, *hi, minb, maxb, minw;
                if (!active_axis(lo, hi, minb, maxb, minw)) return 0;
                const double shift = static_cast<double>(GET_X_LPARAM(lp)-g.drag_x)/(p.right-p.left)*(g.drag_hi-g.drag_lo);
                *lo = g.drag_lo-shift; *hi = g.drag_hi-shift;
                clamp_range(*lo, *hi, minb, maxb, minw);
                if (g.vertical_pan) {
                    const double dy = static_cast<double>(GET_Y_LPARAM(lp)-g.drag_y)/(p.bottom-p.top)*(g.drag_y_hi-g.drag_y_lo);
                    g.frf.y_min = g.drag_y_lo+dy; g.frf.y_max = g.drag_y_hi+dy; g.frf.auto_y = false;
                }
                changed();
            } else if (g.frf.result.ok && inside(GET_X_LPARAM(lp), GET_Y_LPARAM(lp))) {
                const double f = frf_frequency_at_fraction(static_cast<double>(GET_X_LPARAM(lp)-p.left)/(p.right-p.left));
                const auto& fs = g.frf.result.common().frequencies;
                auto it = std::lower_bound(fs.begin(), fs.end(), f);
                std::size_t k = it == fs.end() ? fs.size()-1 : static_cast<std::size_t>(it-fs.begin());
                if (k > 1 && f-fs[k-1] < fs[k]-f) --k;
                double low,high; frf_y_range(low,high);
                std::vector<std::vector<double>> values;
                for (const auto& result:g.frf.result.responses) values.push_back(display_coefficients(result));
                std::size_t nearest=g.frf.result.responses.size();
                double distance=std::numeric_limits<double>::infinity();
                for (std::size_t i=0;i<g.frf.result.responses.size();++i) {
                    const double kd=k<values[i].size() ? values[i][k] : std::numeric_limits<double>::quiet_NaN();
                    if (!std::isfinite(kd)) continue;
                    const double py=p.bottom-(kd-low)/(high-low)*(p.bottom-p.top);
                    const double d=std::fabs(py-GET_Y_LPARAM(lp));
                    if (d<distance) { distance=d; nearest=i; }
                }
                wchar_t text[160]{};
                if (nearest<g.frf.result.responses.size()) {
                    const auto& result=g.frf.result.responses[nearest];
                    const wchar_t* suffix=g.frf.display_smoothing_octaves>0 ? L" (сгл.)" : L"";
                    swprintf(text,160,L" | f = %.6g Hz | КД%s = %.6g",fs[k],suffix,values[nearest][k]);
                    g.status_detail_text=frf_curve_label(nearest)+text;
                    if (k<result.coherence_valid.size() && result.coherence_valid[k]) {
                        swprintf(text,160,L" | Coherence = %.4f",result.coherence[k]);
                        g.status_detail_text+=text;
                    } else g.status_detail_text+=L" | Coherence: —";
                } else {
                    swprintf(text,160,L"f = %.6g Hz | КД: —",fs[k]);
                    g.status_detail_text=text;
                }
                RECT status{0, p.bottom+kAxisBottom, 0, 0};
                RECT client; GetClientRect(hwnd, &client); status.right=client.right; status.bottom=client.bottom;
                InvalidateRect(hwnd, &status, FALSE);
            }
            return 0;
        }
        case WM_LBUTTONUP:
        case WM_CANCELMODE:
            g.dragging = false; if (GetCapture() == hwnd) ReleaseCapture(); return 0;
        case WM_KEYDOWN:
            if (wp == VK_ESCAPE) { g.dragging = false; if (GetCapture() == hwnd) ReleaseCapture(); }
            return 0;
        case WM_RBUTTONDOWN:
            if (has_measure_points()) {
                UndoAction action; action.type=UndoAction::CLEAR_POINTS; action.saved_point_groups=g.point_groups;
                action.saved_active_point_group=g.active_point_group;
                action.saved_time_active_point_group=g.time_active_point_group;
                action.saved_freq_active_point_group=g.freq_active_point_group;
                action.saved_frf_active_point_group=g.frf_active_point_group;
                action.cleared_mode=current_point_group_mode();
                push_undo(action); clear_measure_point_groups(); refresh_side_panel_controls(); invalidate_plot(); return 0;
            }
            return 0;
        case WM_SETCURSOR:
            if (reinterpret_cast<HWND>(wp) == hwnd && LOWORD(lp) == HTCLIENT) {
                SetCursor(LoadCursor(nullptr, (g.pending_line || g.pending_marker || g.measure_mode) ? IDC_CROSS : IDC_HAND)); return TRUE;
            }
            break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
} // namespace gui
