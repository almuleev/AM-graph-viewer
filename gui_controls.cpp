// Shared themed buttons, check controls, and redraw helpers.
#include "gui_controls.hpp"
#include "gui_state.hpp"
#include "gui_theme.hpp"
#include "gui_ids.hpp"

namespace gui {

COLORREF mix_color(COLORREF a, COLORREF b, int weight_b) {
    weight_b = std::clamp(weight_b, 0, 255);
    const int weight_a = 255 - weight_b;
    const int r = (GetRValue(a) * weight_a + GetRValue(b) * weight_b) / 255;
    const int g = (GetGValue(a) * weight_a + GetGValue(b) * weight_b) / 255;
    const int bl = (GetBValue(a) * weight_a + GetBValue(b) * weight_b) / 255;
    return RGB(r, g, bl);
}

void fill_rounded_rect(HDC dc, const RECT& r, COLORREF fill, COLORREF border, int radius) {
    HBRUSH bg = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ old_brush = SelectObject(dc, bg);
    HGDIOBJ old_pen = SelectObject(dc, pen);
    RoundRect(dc, r.left, r.top, r.right, r.bottom, radius, radius);
    SelectObject(dc, old_pen);
    SelectObject(dc, old_brush);
    DeleteObject(pen);
    DeleteObject(bg);
}

void draw_button_with_colors(HDC dc, const RECT& r, const wchar_t* txt,
                             COLORREF bg_col, COLORREF border_col, COLORREF text_col,
                             bool pressed) {
    fill_rounded_rect(dc, r, bg_col, border_col, 6);

    RECT inner = { r.left + 1, r.top + 1, r.right - 1, r.top + 10 };
    HBRUSH gloss = CreateSolidBrush(mix_color(bg_col, RGB(255, 255, 255), pressed ? 8 : 20));
    FillRect(dc, &inner, gloss);
    DeleteObject(gloss);

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, text_col);
    HFONT f = g.ui_font ? g.ui_font : reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    HGDIOBJ old_font = SelectObject(dc, f);
    RECT text_rect = { r.left + 6, r.top, r.right - 6, r.bottom };
    if (pressed) OffsetRect(&text_rect, 0, 1);
    DrawTextW(dc, txt, -1, &text_rect,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    SelectObject(dc, old_font);
}

bool is_channel_checkbox_id(int id) {
    return id >= IDC_CHAN_BASE && id < IDC_CHAN_BASE + static_cast<int>(g.visible.size());
}

bool is_side_toggle_id(int id) {
    return id == IDC_SIDE_PT_NUM ||
           id == IDC_SIDE_PT_X ||
           id == IDC_SIDE_PT_Y ||
           id == IDC_SIDE_PT_DX ||
           id == IDC_SIDE_PT_DY ||
           id == IDC_SIDE_PT_INVDT ||
           id == IDC_SIDE_PT_DIST ||
           id == IDC_SIDE_PT_SNAP ||
           id == IDC_SIDE_FILTER_ENABLE ||
           id == IDC_SIDE_POINT_GROUP_VISIBLE;
}

bool is_settings_toggle_button_id(int id) {
    return id == IDC_SET_LANG_RU ||
           id == IDC_SET_LANG_EN ||
           id == IDC_SET_HOTKEY_CTRL ||
           id == IDC_SET_HOTKEY_SHIFT ||
           id == IDC_SET_HOTKEY_ALT;
}

bool is_settings_hotkey_modifier_id(int id) {
    return id == IDC_SET_HOTKEY_CTRL ||
           id == IDC_SET_HOTKEY_SHIFT ||
           id == IDC_SET_HOTKEY_ALT;
}

bool is_settings_checkbox_id(int id) {
    return id == IDW_LIGHT_MODE ||
           id == IDC_SET_GAP_MARKERS ||
           id == IDC_SET_STITCH_GAPS;
}

bool is_welcome_checkbox_id(int id) {
    return id == IDW_LIGHT_MODE;
}

bool uses_manual_toggle_state(HWND hwnd) {
    if (!hwnd) return false;
    const int id = GetDlgCtrlID(hwnd);
    return is_channel_checkbox_id(id) ||
           is_side_toggle_id(id) ||
           id == IDC_SET_GAP_MARKERS ||
           id == IDC_SET_STITCH_GAPS ||
           id == IDC_EXPORT_APPLY_SETTINGS ||
           id == IDC_EXPORT_APPLY_DATA ||
           id == IDC_EXPORT_INCLUDE_CHANNEL_NAMES ||
           id == IDC_EXPORT_INCLUDE_HIDDEN_CHANNELS ||
           id == IDC_EXPORT_INCLUDE_POINTS ||
           id == IDC_EXPORT_INCLUDE_MARKERS ||
           id == IDC_EXPORT_INCLUDE_GUIDES ||
           id == IDC_EXPORT_INCLUDE_FORMULAS ||
           id == IDC_EXPORT_INCLUDE_FILTER ||
           id == IDC_EXPORT_INCLUDE_GRAPH_SETTINGS ||
           is_welcome_checkbox_id(id) ||
           id == IDC_SET_HOTKEY_CTRL ||
           id == IDC_SET_HOTKEY_SHIFT ||
           id == IDC_SET_HOTKEY_ALT;
}

bool is_toggle_checked(HWND hwnd) {
    if (!hwnd) return false;
    if (uses_manual_toggle_state(hwnd)) {
        return GetPropW(hwnd, L"LvmToggleChecked") != nullptr;
    }
    return SendMessageW(hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

void set_toggle_checked(HWND hwnd, bool checked) {
    if (!hwnd) return;
    if (uses_manual_toggle_state(hwnd)) {
        if (checked) SetPropW(hwnd, L"LvmToggleChecked", reinterpret_cast<HANDLE>(1));
        else RemovePropW(hwnd, L"LvmToggleChecked");
    }
    SendMessageW(hwnd, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
    InvalidateRect(hwnd, nullptr, FALSE);
}

void toggle_checked_state(HWND hwnd) {
    set_toggle_checked(hwnd, !is_toggle_checked(hwnd));
}

void draw_themed_check_control(HDC dc, const RECT& r, const wchar_t* txt,
                               bool checked, bool pressed, bool enabled,
                               bool radio, bool compact,
                               COLORREF surface_bg) {
    const COLORREF base_bg = (surface_bg == CLR_INVALID) ? g_theme->bg_panel : surface_bg;
    HBRUSH bg = CreateSolidBrush(base_bg);
    FillRect(dc, &r, bg);
    DeleteObject(bg);

    const int box_size = compact ? 14 : 16;
    const int box_left = compact ? (r.left + ((r.right - r.left - box_size) / 2)) : (r.left + 4);
    const int box_top = r.top + ((r.bottom - r.top - box_size) / 2);
    RECT box = { box_left, box_top, box_left + box_size, box_top + box_size };

    COLORREF fill = checked ? (pressed ? g_theme->accent_hover : g_theme->accent)
                            : (pressed ? g_theme->btn_hover : g_theme->bg_plot);
    COLORREF border = checked ? g_theme->accent : g_theme->btn_border;
    COLORREF text = enabled ? g_theme->text_primary : g_theme->text_secondary;
    if (!enabled) {
        fill = mix_color(fill, base_bg, 96);
        border = mix_color(border, base_bg, 96);
    }

    if (radio) {
        HBRUSH outer = CreateSolidBrush(fill);
        HPEN pen = CreatePen(PS_SOLID, 1, border);
        HGDIOBJ old_brush = SelectObject(dc, outer);
        HGDIOBJ old_pen = SelectObject(dc, pen);
        Ellipse(dc, box.left, box.top, box.right, box.bottom);
        SelectObject(dc, old_pen);
        SelectObject(dc, old_brush);
        DeleteObject(pen);
        DeleteObject(outer);
        if (checked) {
            RECT dot = { box.left + 4, box.top + 4, box.right - 4, box.bottom - 4 };
            HBRUSH dot_brush = CreateSolidBrush(RGB(255, 255, 255));
            HGDIOBJ old_dot = SelectObject(dc, dot_brush);
            HPEN dot_pen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
            HGDIOBJ old_dot_pen = SelectObject(dc, dot_pen);
            Ellipse(dc, dot.left, dot.top, dot.right, dot.bottom);
            SelectObject(dc, old_dot_pen);
            SelectObject(dc, old_dot);
            DeleteObject(dot_pen);
            DeleteObject(dot_brush);
        }
    } else {
        fill_rounded_rect(dc, box, fill, border, 4);
        if (checked) {
            HPEN check_pen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
            HGDIOBJ old_pen = SelectObject(dc, check_pen);
            MoveToEx(dc, box.left + 3, box.top + 8, nullptr);
            LineTo(dc, box.left + 6, box.bottom - 4);
            LineTo(dc, box.right - 3, box.top + 4);
            SelectObject(dc, old_pen);
            DeleteObject(check_pen);
        }
    }

    if (!compact && txt && txt[0]) {
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, text);
        HFONT font = g.ui_font ? g.ui_font : reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        HGDIOBJ old_font = SelectObject(dc, font);
        RECT text_rect = { box.right + 10, r.top, r.right - 4, r.bottom };
        if (pressed) OffsetRect(&text_rect, 0, 1);
        DrawTextW(dc, txt, -1, &text_rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
        SelectObject(dc, old_font);
    }
}

void draw_welcome_action_button(HDC dc, const RECT& r, const wchar_t* txt,
                                bool pressed, bool primary, bool outlined) {
    COLORREF bg_col = g_theme->btn_bg;
    COLORREF border_col = g_theme->btn_border;
    COLORREF text_col = g_theme->text_primary;

    if (primary) {
        bg_col = pressed ? g_theme->accent_hover : g_theme->accent;
        border_col = pressed ? g_theme->accent_hover : g_theme->accent;
        text_col = RGB(255, 255, 255);
    } else if (outlined) {
        if (g_theme == &kDarkTheme) {
            bg_col = pressed ? mix_color(g_theme->btn_hover, g_theme->accent, 26)
                             : mix_color(g_theme->btn_bg, g_theme->accent, 18);
            border_col = mix_color(g_theme->btn_border, g_theme->accent, 72);
            text_col = RGB(235, 240, 248);
        } else {
            bg_col = pressed ? g_theme->btn_hover : g_theme->bg_panel;
            border_col = g_theme->accent;
            text_col = g_theme->text_primary;
        }
    } else {
        if (g_theme == &kDarkTheme) {
            bg_col = pressed ? g_theme->btn_hover : mix_color(g_theme->btn_bg, g_theme->bg_panel, 70);
            border_col = mix_color(g_theme->btn_border, g_theme->separator, 48);
            text_col = g_theme->text_primary;
        } else {
            bg_col = pressed ? g_theme->btn_hover : g_theme->btn_bg;
            border_col = g_theme->btn_border;
            text_col = g_theme->text_primary;
        }
    }

    fill_rounded_rect(dc, r, bg_col, border_col, 6);

    if (g_theme != &kDarkTheme) {
        RECT inner = { r.left + 1, r.top + 1, r.right - 1, r.top + 10 };
        HBRUSH gloss = CreateSolidBrush(mix_color(bg_col, RGB(255, 255, 255), pressed ? 8 : 20));
        FillRect(dc, &inner, gloss);
        DeleteObject(gloss);
    } else if (!primary) {
        RECT inner = { r.left + 1, r.top + 1, r.right - 1, min(r.bottom - 1, r.top + 8) };
        HBRUSH sheen = CreateSolidBrush(mix_color(bg_col, RGB(255, 255, 255), pressed ? 4 : 10));
        FillRect(dc, &inner, sheen);
        DeleteObject(sheen);
    }

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, text_col);
    HFONT f = g.ui_font ? g.ui_font : reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    HGDIOBJ old_font = SelectObject(dc, f);
    RECT text_rect = r;
    if (pressed) OffsetRect(&text_rect, 0, 1);
    DrawTextW(dc, txt, -1, &text_rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(dc, old_font);
}

void redraw_button(HWND btn) {
    if (!btn || !IsWindow(btn)) return;
    RedrawWindow(btn, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
}

void redraw_window_with_children(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) return;
    RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ALLCHILDREN);
}

void redraw_toolbar_buttons() {
    for (HWND btn : g.buttons) redraw_button(btn);
}

void draw_themed_button(HDC dc, const RECT& r, const wchar_t* txt, bool pressed, bool active, bool hover) {
    COLORREF bg_col, border_col, text_col;
    if (active && pressed) {
        bg_col = g_theme->accent_hover;
        border_col = g_theme->accent_hover;
        text_col = (g_theme == &kDarkTheme) ? RGB(255, 255, 255) : RGB(12, 42, 78);
    } else if (active) {
        bg_col = mix_color(g_theme->btn_bg, g_theme->accent, hover ? 64 : 42);
        border_col = g_theme->accent;
        text_col = (g_theme == &kDarkTheme) ? RGB(255, 255, 255) : RGB(12, 42, 78);
    } else if (pressed) {
        bg_col = mix_color(g_theme->btn_hover, g_theme->separator, 68);
        border_col = g_theme->separator;
        text_col = g_theme->text_primary;
    } else if (hover) {
        bg_col = g_theme->btn_hover;
        border_col = g_theme->btn_border;
        text_col = g_theme->text_primary;
    } else {
        bg_col = g_theme->btn_bg;
        border_col = g_theme->btn_border;
        text_col = g_theme->text_primary;
    }

    draw_button_with_colors(dc, r, txt, bg_col, border_col, text_col, pressed);
}

} // namespace gui
