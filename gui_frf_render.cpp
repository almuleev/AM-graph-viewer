#include "gui_frf_render.hpp"
#include "gui_frf.hpp"
#include "gui_state.hpp"
#include "gui_render.hpp"
#include "gui_layout.hpp"
#include "gui_navigation.hpp"
#include "gui_status.hpp"
#include "gui_ids.hpp"
#include "gui_text.hpp"
#include "gui_theme.hpp"

namespace gui {
namespace {
double dynamic_coefficient(const lvm::FrfResult& r, std::size_t k) {
    return lvm::frf_dynamic_coefficient(r, k);
}
void line(HDC dc, int x0, int y0, int x1, int y1) {
    MoveToEx(dc, x0, y0, nullptr); LineTo(dc, x1, y1);
}
void changed() {
    refresh_frf_controls(); set_status(); invalidate_plot();
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
        for (std::size_t k = 1; k < r.frequencies.size(); ++k) {
            if (r.frequencies[k] < a || r.frequencies[k] > b) continue;
            const double value = dynamic_coefficient(r,k);
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
        HPEN curve = CreatePen(PS_SOLID, 1, channel_color(g.frf.outputs[response]));
        old_pen = SelectObject(dc, curve);
        const std::size_t begin = std::max<std::size_t>(1, static_cast<std::size_t>(
            std::lower_bound(r.frequencies.begin(), r.frequencies.end(), f0) - r.frequencies.begin()));
        const std::size_t end = static_cast<std::size_t>(
            std::upper_bound(r.frequencies.begin(), r.frequencies.end(), f1) - r.frequencies.begin());
        bool started = false;
        // Collapse samples at the same screen x into a min/max segment. Never join
        // across invalid input bins, even when many bins occupy a single pixel.
        int column = -1, ylo = 0, yhi = 0, last_y = 0;
        auto flush = [&] {
            if (column < 0) return;
            if (started) LineTo(dc, column, ylo); else MoveToEx(dc, column, ylo, nullptr);
            LineTo(dc, column, yhi + 1);
            MoveToEx(dc, column, last_y, nullptr);
            started = true;
        };
        for (std::size_t k = begin; k < end; ++k) {
            const double v = dynamic_coefficient(r,k);
            if (!std::isfinite(v)) { flush(); column = -1; started = false; continue; }
            const int x = mapx(r.frequencies[k]), y = mapy(v);
            if (x != column) { flush(); column = x; ylo = yhi = last_y = y; }
            else { ylo = std::min(ylo, y); yhi = std::max(yhi, y); last_y = y; }
        }
        flush();
        SelectObject(dc, old_pen); DeleteObject(curve);
    }
    RestoreDC(dc, saved);
    SetTextColor(dc, g_theme->text_primary);
    // Compact multi-column legend; the cursor exposes the full curve label.
    const int rows=std::max(1,(height-20)/20);
    const int columns=std::max(1,static_cast<int>((g.frf.output_names.size()+rows-1)/rows));
    const int column_width=std::max(1,(width-20)/columns);
    for (std::size_t i=0;i<g.frf.output_names.size();++i) {
        const int left=p.left+10+static_cast<int>(i/rows)*column_width;
        const int top=p.top+6+static_cast<int>(i%rows)*20;
        RECT box{left,top,left+column_width-4,top+20};
        FillRect(dc,&box,g_panel_brush);
        HPEN pen=CreatePen(PS_SOLID,2,channel_color(g.frf.outputs[i]));
        auto previous=SelectObject(dc,pen);
        line(dc,left+3,top+10,left+22,top+10);
        SelectObject(dc,previous); DeleteObject(pen);
        RECT label{left+28,top,box.right-3,top+20};
        std::wstring name=frf_curve_label(i);
        if (i<g.frf.result.responses.size() && !g.frf.result.responses[i].ok)
            name+=L" — "+gui::frf_error_text(g.frf.result.responses[i].error);
        DrawTextW(dc,name.c_str(),-1,&label,DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS);
        g_legend_items.push_back({g.frf.outputs[i],box});
    }
    RECT xlabel{p.left, p.bottom+23, p.right, p.bottom+42};
    DrawTextW(dc, L"Frequency, Hz (log)", -1, &xlabel, DT_CENTER | DT_SINGLELINE);
    RECT ylabel{4, p.top+2, 34, p.top+22};
    DrawTextW(dc, L"КД", -1, &ylabel, DT_LEFT | DT_SINGLELINE);
    SelectObject(dc, font);
    g.vx0 = g.frf.log_start; g.vx1 = g.frf.log_end;
    g.vy0 = low; g.vy1 = high; g.vrect = p; g.vvalid = true;
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
                std::size_t nearest=g.frf.result.responses.size();
                double distance=std::numeric_limits<double>::infinity();
                for (std::size_t i=0;i<g.frf.result.responses.size();++i) {
                    const double kd=dynamic_coefficient(g.frf.result.responses[i],k);
                    if (!std::isfinite(kd)) continue;
                    const double py=p.bottom-(kd-low)/(high-low)*(p.bottom-p.top);
                    const double d=std::fabs(py-GET_Y_LPARAM(lp));
                    if (d<distance) { distance=d; nearest=i; }
                }
                wchar_t text[160]{};
                set_status();
                if (nearest<g.frf.result.responses.size()) {
                    const auto& result=g.frf.result.responses[nearest];
                    swprintf(text,160,L" | f = %.6g Hz | КД = %.6g",fs[k],lvm::frf_dynamic_coefficient(result,k));
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
        case WM_RBUTTONDOWN: return 0;
        case WM_SETCURSOR:
            if (reinterpret_cast<HWND>(wp) == hwnd && LOWORD(lp) == HTCLIENT) {
                SetCursor(LoadCursor(nullptr, IDC_HAND)); return TRUE;
            }
            break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
} // namespace gui
