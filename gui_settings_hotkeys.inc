// Hotkey, settings, and dialog helpers extracted from gui_main.cpp.
std::wstring hotkey_text(BYTE fvirt, WORD key) {
    if (key == 0) return g_str == &kEn ? L"Not assigned" : L"Не назначено";
    std::wstring out;
    if (fvirt & FCONTROL) out += L"Ctrl+";
    if (fvirt & FSHIFT) out += L"Shift+";
    if (fvirt & FALT) out += L"Alt+";
    out += key_name(key);
    return out;
}

std::wstring hotkey_text_for_command(int command) {
    const HotkeyBinding* hk = find_hotkey_binding(command);
    return hk ? hotkey_text(hk->fvirt, hk->key) : std::wstring();
}

std::wstring hotkey_display_text_for_command(int command) {
    std::wstring text = hotkey_text_for_command(command);
    if (!text.empty()) return text;
    return (g_str == &kEn) ? L"Not assigned" : L"Не назначено";
}

std::wstring menu_text(const wchar_t* base, int command) {
    const HotkeyBinding* binding = find_hotkey_binding(command);
    if (!binding || binding->key == 0) return base;
    std::wstring hk = hotkey_text(binding->fvirt, binding->key);
    if (hk.empty()) return base;
    return std::wstring(base) + L"\t" + hk;
}

std::wstring command_name(int command) {
    const bool en = (g_str == &kEn);
    switch (command) {
        case IDC_OPEN: return en ? L"Open file" : L"Открыть файл";
        case IDC_SAVEPNG: return en ? L"Save PNG" : L"Сохранить PNG";
        case IDC_SAVECSV: return en ? L"Save as…" : L"Сохранить как…";
        case IDM_UNDO: return en ? L"Undo" : L"Отменить";
        case IDM_REDO: return en ? L"Redo" : L"Повторить";
        case IDM_MODE_TIME: return en ? L"Time view" : L"Режим времени";
        case IDM_MODE_FREQ: return en ? L"Hz / FFT view" : L"Режим Гц / БПФ";
        case IDC_MEASURE: return en ? L"Measurement points" : L"Точки измерения";
        case IDM_ADD_MARKER: return en ? L"Marker" : L"Маркер";
        case IDM_ADD_VLINE: return en ? L"Vertical line" : L"Вертикальная линия";
        case IDM_ADD_HLINE: return en ? L"Horizontal line" : L"Горизонтальная линия";
        case IDM_ADD_VLINE_EXACT: return en ? L"Vertical line (exact)" : L"Вертикальная линия (точно)";
        case IDM_ADD_HLINE_EXACT: return en ? L"Horizontal line (exact)" : L"Горизонтальная линия (точно)";
        case IDC_AUTOY: return en ? L"Auto zoom" : L"Автомасштабирование";
        case IDM_VISMOOTH: return en ? L"Smoothing" : L"Сглаживание";
        case IDM_VPAN: return en ? L"Vertical pan" : L"Вертикальное панорамирование";
        case IDM_THEME: return en ? L"Dark theme" : L"Тёмная тема";
        case IDC_PLAY: return en ? L"Play / Pause" : L"Старт/стоп";
        case IDC_ZOOMIN: return en ? L"Zoom in" : L"Увеличить";
        case IDC_ZOOMOUT: return en ? L"Zoom out" : L"Уменьшить";
        case IDC_PANLEFT: return en ? L"Pan left" : L"Сдвиг влево";
        case IDC_PANRIGHT: return en ? L"Pan right" : L"Сдвиг вправо";
        case IDC_RESET: return en ? L"Reset view" : L"Сброс вида";
        case IDC_GOTO_START: return en ? L"Go to start" : L"В начало";
        case IDC_GOTO_END: return en ? L"Go to end" : L"В конец";
        case IDM_CLEAR_POINTS: return en ? L"Clear points" : L"Очистить точки";
        case IDM_HOTKEYS: return en ? L"Hotkeys help" : L"Справка по клавишам";
        case IDM_SPEED_CUSTOM: return en ? L"Playback speed…" : L"Скорость воспроизведения…";
        default: return L"?";
    }
}

std::vector<int> hotkey_command_order() {
    return {
        IDC_OPEN, IDC_SAVEPNG, IDC_SAVECSV, IDM_UNDO, IDM_REDO,
        IDM_MODE_TIME, IDM_MODE_FREQ, IDC_MEASURE, IDM_ADD_MARKER,
        IDM_ADD_VLINE, IDM_ADD_HLINE, IDC_AUTOY, IDM_VISMOOTH,
        IDM_VPAN, IDM_THEME, IDC_PLAY, IDC_ZOOMIN, IDC_ZOOMOUT,
        IDC_PANLEFT, IDC_PANRIGHT, IDC_RESET, IDC_GOTO_START,
        IDC_GOTO_END, IDM_CLEAR_POINTS, IDM_HOTKEYS
    };
}

std::wstring hotkey_list_item_text(int command) {
    return command_name(command) + L"  [" + hotkey_display_text_for_command(command) + L"]";
}

std::wstring welcome_version_text() {
    return (g_str == &kEn)
        ? (std::wstring(L"Build: ") + APP_VERSION_W)
        : (std::wstring(L"Версия сборки: ") + APP_VERSION_W);
}

std::wstring welcome_intro_text() {
    return (g_str == &kEn)
        ? L"Open a file, choose how to work with it, and continue right away."
        : L"Откройте файл, выберите удобный режим работы и сразу продолжайте.";
}

std::wstring welcome_features_text() {
    return (g_str == &kEn)
        ? L"Light mode opens only the required fragment.\r\nThe side panel keeps channels, points, formulas, and filters together.\r\nThemes, hotkeys, and export options are available from the interface."
        : L"Лёгкий режим открывает только нужный фрагмент.\r\nБоковая панель объединяет каналы, точки, формулы и фильтры.\r\nТемы, горячие клавиши и экспорт доступны прямо в интерфейсе.";
}

const wchar_t* welcome_actions_title_text() {
    return (g_str == &kEn) ? L"What to do next" : L"Что дальше";
}


const wchar_t* welcome_author_credit_text() {
    return (g_str == &kEn)
        ? L"Application developed by Alexander Muleev  |  al.muleev@gmail.com"
        : L"Приложение разработал Александр Мулеев  |  al.muleev@gmail.com";
}


struct WelcomeLayout {
    RECT bounds{};
    RECT hero{};
    RECT action{};
    bool stacked = false;
    bool compact = false;
};

int rect_width(const RECT& r) {
    return r.right - r.left;
}

int rect_height(const RECT& r) {
    return r.bottom - r.top;
}

WelcomeLayout compute_welcome_layout(HWND hwnd) {
    RECT rc;
    GetClientRect(hwnd, &rc);
    WelcomeLayout layout{};
    const int client_w = max(0, static_cast<int>(rc.right - rc.left));
    const int client_h = max(0, static_cast<int>(rc.bottom - rc.top));
    const int outer = (client_w >= 1200) ? 36 : ((client_w >= 900) ? 28 : 20);
    const int gap = (client_w >= 900) ? 24 : 16;
    const int content_w = max(320, min(client_w - outer * 2, 1180));
    const int content_h = max(260, client_h - outer * 2);
    const int x0 = max(0, (client_w - content_w) / 2);
    const int y0 = max(0, (client_h - content_h) / 2);

    layout.bounds = { x0, y0, x0 + content_w, y0 + content_h };
    layout.stacked = (content_w < 760);

    if (layout.stacked) {
        int hero_h = std::clamp(content_h * 48 / 100, 220, 340);
        const int min_action_h = 320;
        hero_h = min(hero_h, max(180, content_h - gap - min_action_h));
        layout.hero = { x0, y0, x0 + content_w, y0 + hero_h };
        layout.action = { x0, layout.hero.bottom + gap, x0 + content_w, y0 + content_h };
    } else {
        const int action_w = std::clamp(content_w / 3, 320, 380);
        layout.hero = { x0, y0, x0 + content_w - action_w - gap, y0 + content_h };
        layout.action = { layout.hero.right + gap, y0, x0 + content_w, y0 + content_h };
    }

    layout.compact = rect_height(layout.action) < 560 || rect_width(layout.action) < 340;

    return layout;
}

void layout_welcome_controls(HWND hwnd) {
    WelcomeLayout layout = compute_welcome_layout(hwnd);
    auto place = [&](int id, int x, int y, int w, int h) {
        HWND ctl = GetDlgItem(hwnd, id);
        if (ctl) MoveWindow(ctl, x, y, max(1, w), max(1, h), TRUE);
    };

    const int hero_pad = layout.compact ? 20 : (layout.stacked ? 22 : 30);
    const int hx = layout.hero.left + hero_pad;
    int hy = layout.hero.top + hero_pad;
    const int hw = max(180, rect_width(layout.hero) - hero_pad * 2);
    const int title_h = layout.compact ? 40 : (layout.stacked ? 46 : 52);
    const int subtitle_h = layout.compact ? 22 : 24;
    const int version_h = layout.compact ? 18 : 20;
    place(IDW_TITLE, hx, hy, hw, title_h);
    hy += title_h + (layout.compact ? 6 : 8);
    place(IDW_SUBTITLE, hx, hy, hw, subtitle_h);
    hy += subtitle_h + (layout.compact ? 4 : 6);
    place(IDW_VERSION, hx, hy, hw, version_h);

    const int action_pad = layout.compact ? 16 : (layout.stacked ? 20 : 24);
    const int ax = layout.action.left + action_pad;
    int ay = layout.action.top + action_pad;
    const int aw = max(180, rect_width(layout.action) - action_pad * 2);
    const int segment_gap = 10;
    const int segment_button_h = layout.compact ? 28 : 32;
    const int action_button_h = layout.compact ? 34 : 40;
    const int button_gap = layout.compact ? 8 : (layout.stacked ? 10 : 12);
    place(IDW_LANG_LABEL, ax, ay, aw, 20);
    ay += layout.compact ? 22 : 24;
    const int lang_w = max(80, (aw - segment_gap) / 2);
    place(IDM_LANG_RU, ax, ay, lang_w, segment_button_h);
    place(IDM_LANG_EN, ax + aw - lang_w, ay, lang_w, segment_button_h);
    ay += segment_button_h + (layout.compact ? 8 : 12);
    place(IDW_THEME_LABEL, ax, ay, aw, 20);
    ay += layout.compact ? 22 : 24;
    const int theme_w = max(80, (aw - segment_gap) / 2);
    place(IDW_THEME_LIGHT, ax, ay, theme_w, segment_button_h);
    place(IDW_THEME_DARK, ax + aw - theme_w, ay, theme_w, segment_button_h);
    ay += segment_button_h + (layout.compact ? 12 : 16);
    place(IDW_PERF_LABEL, ax, ay, aw, 20);
    ay += layout.compact ? 22 : 24;
    place(IDW_PERF_FAST_OPEN, ax, ay, aw, segment_button_h);
    ay += segment_button_h + (layout.compact ? 6 : 8);
    place(IDW_PERF_FAST_WORK, ax, ay, aw, segment_button_h);
    ay += segment_button_h + (layout.compact ? 6 : 8);
    place(IDW_PERF_COMFORT, ax, ay, aw, segment_button_h);
    ay += segment_button_h + (layout.compact ? 12 : 16);
    place(IDC_PTSETTINGS, ax, ay, aw, action_button_h);
    ay += action_button_h + button_gap;
    place(IDM_HOTKEYS, ax, ay, aw, action_button_h);
    ay += action_button_h + button_gap;
    place(IDW_START, ax, ay, aw, action_button_h);
}

void append_hotkey_line(std::wstring& out, int command) {
    out += L"  " + hotkey_display_text_for_command(command) + L"\t: " + command_name(command) + L"\n";
}

std::wstring hotkeys_body_text() {
    ensure_hotkeys_initialized();
    const bool en = (g_str == &kEn);
    std::wstring out = en ? L"Files\n" : L"Файлы\n";
    append_hotkey_line(out, IDC_OPEN);
    append_hotkey_line(out, IDC_SAVEPNG);
    append_hotkey_line(out, IDC_SAVECSV);
    append_hotkey_line(out, IDM_UNDO);
    append_hotkey_line(out, IDM_REDO);
    out += L"\n";

    out += en ? L"Modes and tools\n" : L"Режимы и инструменты\n";
    append_hotkey_line(out, IDM_MODE_TIME);
    append_hotkey_line(out, IDM_MODE_FREQ);
    append_hotkey_line(out, IDC_MEASURE);
    append_hotkey_line(out, IDM_ADD_MARKER);
    append_hotkey_line(out, IDM_ADD_VLINE);
    append_hotkey_line(out, IDM_ADD_HLINE);
    out += en ? L"  Esc\t: Cancel current add mode\n\n" : L"  Esc\t: Отменить текущий режим добавления\n\n";

    out += en ? L"View\n" : L"Вид\n";
    append_hotkey_line(out, IDC_AUTOY);
    append_hotkey_line(out, IDM_VISMOOTH);
    append_hotkey_line(out, IDM_VPAN);
    append_hotkey_line(out, IDM_THEME);
    append_hotkey_line(out, IDC_PLAY);
    append_hotkey_line(out, IDC_ZOOMIN);
    append_hotkey_line(out, IDC_ZOOMOUT);
    append_hotkey_line(out, IDC_PANLEFT);
    append_hotkey_line(out, IDC_PANRIGHT);
    append_hotkey_line(out, IDC_RESET);
    append_hotkey_line(out, IDC_GOTO_START);
    append_hotkey_line(out, IDC_GOTO_END);
    append_hotkey_line(out, IDM_CLEAR_POINTS);
    out += L"\n";

    out += en ? L"Mouse\n" : L"Мышь\n";
    out += en ? L"  Wheel\t: Zoom under cursor\n" : L"  Колесо\t: Масштаб под курсором\n";
    out += en ? L"  Shift+Wheel\t: Pan left / right\n" : L"  Shift+колесо\t: Прокрутка влево / вправо\n";
    out += en ? L"  Ctrl+Wheel\t: Zoom Y\n" : L"  Ctrl+колесо\t: Масштаб по Y\n";
    out += en ? L"  Alt+Wheel\t: Pan up / down (Y)\n" : L"  Alt+колесо\t: Сдвиг вверх / вниз по Y\n";
    out += en ? L"  Left-drag\t: Pan view\n" : L"  ЛКМ + тяга\t: Панорамирование\n";
    out += en ? L"  Left-click\t: Place point / line / marker in active mode\n" : L"  ЛКМ\t: Поставить точку / линию / маркер в активном режиме\n";
    out += en ? L"  Right-click\t: Clear points\n\n" : L"  ПКМ\t: Очистить точки\n\n";
    append_hotkey_line(out, IDM_HOTKEYS);
    return out;
}

const wchar_t* hotkeys_dialog_close_text() {
    return (g_str == &kEn) ? L"Close" : L"Закрыть";
}

std::vector<std::wstring> hotkeys_dialog_lines() {
    std::vector<std::wstring> lines;
    std::wstring body = hotkeys_body_text();
    std::size_t start = 0;
    while (start <= body.size()) {
        std::size_t end = body.find(L'\n', start);
        std::wstring line = body.substr(start, end == std::wstring::npos ? std::wstring::npos : end - start);
        if (!line.empty() && line.back() == L'\r') line.pop_back();
        lines.push_back(std::move(line));
        if (end == std::wstring::npos) break;
        start = end + 1;
    }
    return lines;
}

SIZE hotkeys_dialog_client_size() {
    const auto lines = hotkeys_dialog_lines();
    HFONT font = g.ui_font ? g.ui_font : reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    HWND measure_wnd = g.main ? g.main : GetDesktopWindow();
    HDC dc = GetDC(measure_wnd);
    HFONT old_font = dc ? reinterpret_cast<HFONT>(SelectObject(dc, font)) : nullptr;
    TEXTMETRICW tm{};
    if (dc) GetTextMetricsW(dc, &tm);

    int max_line_width = 0;
    for (const std::wstring& line : lines) {
        SIZE text_size{};
        const wchar_t* text = line.empty() ? L" " : line.c_str();
        int length = line.empty() ? 1 : static_cast<int>(line.size());
        if (dc && GetTextExtentPoint32W(dc, text, length, &text_size)) {
            max_line_width = max(max_line_width, static_cast<int>(text_size.cx));
        }
    }

    if (dc) {
        if (old_font) SelectObject(dc, old_font);
        ReleaseDC(measure_wnd, dc);
    }

    const int title_h = max(20, static_cast<int>(tm.tmHeight + tm.tmExternalLeading));
    const int line_h = max(18, static_cast<int>(tm.tmHeight + tm.tmExternalLeading + 2));
    const int button_h = 30;
    const int client_w = max(620, max_line_width + 56);
    const int client_h = 16 + title_h + 8 + static_cast<int>(lines.size()) * line_h + 8 + 12 + button_h + 18;
    return { client_w, max(client_h, 300) };
}

void populate_hotkeys_dialog_list(HWND list) {
    if (!list) return;
    SendMessageW(list, LB_RESETCONTENT, 0, 0);
    const auto lines = hotkeys_dialog_lines();
    for (const std::wstring& line : lines) {
        SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(line.c_str()));
    }
    SendMessageW(list, LB_SETCURSEL, static_cast<WPARAM>(-1), 0);
}

LRESULT CALLBACK HotkeysDialogProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CREATE: {
            HFONT font = g.ui_font ? g.ui_font : reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
            RECT rc{};
            GetClientRect(hwnd, &rc);
            const int pad = 18;
            const int title_y = 16;
            const int title_h = 20;
            const int list_y = 44;
            const int button_h = 30;
            const int button_w = 122;
            const int button_y = rc.bottom - pad - button_h;
            const int list_w = max(200, static_cast<int>(rc.right - pad * 2));
            const int list_h = max(120, button_y - 12 - list_y);
            HWND label = CreateWindowExW(
                0, L"STATIC",
                (g_str == &kEn) ? L"Configured keyboard shortcuts" : L"Настроенные горячие клавиши",
                WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX,
                pad, title_y, list_w, title_h, hwnd, nullptr,
                reinterpret_cast<LPCREATESTRUCT>(lp)->hInstance, nullptr);
            g_hotkeys_dialog.list = CreateWindowExW(
                0, L"LISTBOX", L"",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | WS_BORDER | LBS_NOINTEGRALHEIGHT,
                pad, list_y, list_w, list_h, hwnd,
                reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_HOTKEYS_DIALOG_LIST)),
                reinterpret_cast<LPCREATESTRUCT>(lp)->hInstance, nullptr);
            HWND close = CreateWindowExW(
                0, L"BUTTON", hotkeys_dialog_close_text(),
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                rc.right - pad - button_w, button_y, button_w, button_h, hwnd,
                reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_HOTKEYS_DIALOG_CLOSE)),
                reinterpret_cast<LPCREATESTRUCT>(lp)->hInstance, nullptr);
            if (label) SendMessageW(label, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
            if (g_hotkeys_dialog.list) SendMessageW(g_hotkeys_dialog.list, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
            if (close) SendMessageW(close, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
            populate_hotkeys_dialog_list(g_hotkeys_dialog.list);
            enable_file_drop_support(hwnd);
            return 0;
        }
        case WM_COMMAND:
            if (LOWORD(wp) == IDC_HOTKEYS_DIALOG_CLOSE) {
                DestroyWindow(hwnd);
                return 0;
            }
            break;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DROPFILES:
            if (g.main && IsWindow(g.main)) {
                SendMessageW(g.main, WM_DROPFILES, wp, lp);
                return 0;
            }
            return 0;
        case WM_ERASEBKGND: {
            HDC dc = reinterpret_cast<HDC>(wp);
            draw_prompt_surface(hwnd, dc);
            return 1;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps{};
            HDC dc = BeginPaint(hwnd, &ps);
            draw_prompt_surface(hwnd, dc);
            EndPaint(hwnd, &ps);
            return 1;
        }
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN: {
            HDC dc = reinterpret_cast<HDC>(wp);
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, g_theme->text_primary);
            return reinterpret_cast<LRESULT>(g_panel_brush);
        }
        case WM_CTLCOLORLISTBOX: {
            HDC dc = reinterpret_cast<HDC>(wp);
            SetBkColor(dc, g_theme->bg_plot);
            SetTextColor(dc, g_theme->text_primary);
            return reinterpret_cast<LRESULT>(g_input_brush ? g_input_brush : g_panel_brush);
        }
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* dis = reinterpret_cast<DRAWITEMSTRUCT*>(lp);
            if (!dis || !dis->hwndItem) break;
            if (GetDlgCtrlID(dis->hwndItem) == IDC_HOTKEYS_DIALOG_CLOSE) {
                wchar_t txt[64]{};
                GetWindowTextW(dis->hwndItem, txt, 64);
                const bool pressed = (dis->itemState & ODS_SELECTED) != 0;
                draw_welcome_action_button(dis->hDC, dis->rcItem, txt, pressed, false, false);
                return TRUE;
            }
            break;
        }
        case WM_DESTROY:
            g_hotkeys_dialog.done = true;
            g_hotkeys_dialog.wnd = nullptr;
            g_hotkeys_dialog.list = nullptr;
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

std::wstring toolbar_hover_text(HWND btn) {
    const bool en = (g_str == &kEn);
    if (btn == g.open) return g_str->hover_open;
    if (btn == g.play) return g.playing ? g_str->hover_pause : g_str->hover_play;
    if (btn == g.measure) return g_str->hover_measure;
    if (btn == g.reset) return g_str->hover_reset;
    if (btn == g.autoy) return g_str->hover_autoy;
    if (btn == g.ptsettings) return en ? L"Open general settings" : L"Открыть общие настройки";
    if (btn == g.sidepanel_btn) return en ? L"Show or hide the right-side work panel" : L"Показать или скрыть рабочую панель справа";
    if (btn == g.mode_time) return en ? L"Switch to Time view" : L"Переключить в режим времени";
    if (btn == g.mode_freq) return en ? L"Switch to FFT spectrum" : L"Переключить в режим спектра БПФ";
    if (btn == g.marker_btn) return en ? L"Place a marker on the plot" : L"Поставить маркер на график";
    if (btn == g.vline_btn) return en ? L"Place a vertical guide line" : L"Поставить вертикальную линию";
    if (btn == g.hline_btn) return en ? L"Place a horizontal guide line" : L"Поставить горизонтальную линию";
    if (btn == g.show_all_btn) return en ? L"Show all channels" : L"Показать все каналы";
    if (btn == g.hide_all_btn) return en ? L"Hide all channels" : L"Скрыть все каналы";
    if (btn == g.savepng) return g_str->hover_png;
    if (btn == g.savecsv) return en ? L"Save as" : L"Сохранить как";
    return L"";
}

int hit_test_gap_marker(int x, int y) {
    POINT pt{ x, y };
    for (int i = static_cast<int>(g.visible_gap_markers.size()) - 1; i >= 0; --i) {
        if (PtInRect(&g.visible_gap_markers[static_cast<std::size_t>(i)].rect, pt)) return i;
    }
    return -1;
}

void hide_gap_details_card() {
    if (!g.gap_details_visible &&
        g.gap_details_duration == 0.0 &&
        g.gap_details_missing_samples == 0 &&
        g.gap_details_reference_step == 0.0) {
        return;
    }
    g.gap_details_visible = false;
    g.gap_details_duration = 0.0;
    g.gap_details_missing_samples = 0;
    g.gap_details_reference_step = 0.0;
    if (g.main && IsWindow(g.main)) InvalidateRect(g.main, nullptr, FALSE);
}

void show_gap_details_card(double duration, long long estimated_missing_samples) {
    g.gap_details_visible = true;
    g.gap_details_duration = duration;
    g.gap_details_missing_samples = estimated_missing_samples;
    g.gap_details_reference_step = gap_reference_step_from_estimate(duration, estimated_missing_samples);
    if (g.main && IsWindow(g.main)) InvalidateRect(g.main, nullptr, FALSE);
}

void draw_gap_details_card(HDC dc, const RECT& plot) {
    if (!g.gap_details_visible || !has_data() || !g.show_gap_markers || g.freq_mode) return;

    const int margin = 12;
    const int pad = 14;
    const int available_w = plot.right - plot.left - margin * 2;
    const int available_h = plot.bottom - plot.top - margin * 2;
    if (available_w < 220 || available_h < 120) return;

    int card_w = available_w > 420 ? 420 : available_w;
    if (card_w < 260) card_w = available_w;
    if (card_w < 220) card_w = 220;

    const std::wstring title = gap_details_title_text(g_str == &kEn);
    const std::wstring body = gap_details_body_text(
        g_str == &kEn,
        g.gap_details_duration,
        g.gap_details_missing_samples,
        g.gap_details_reference_step);

    RECT title_rc = {0, 0, card_w - pad * 2, 0};
    RECT body_rc = {0, 0, card_w - pad * 2, 0};
    HFONT body_font = g.ui_font ? g.ui_font : reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    HFONT title_font = g.bold_font ? g.bold_font : body_font;
    HGDIOBJ old_font = SelectObject(dc, title_font);
    DrawTextW(dc, title.c_str(), -1, &title_rc, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(dc, body_font);
    DrawTextW(dc, body.c_str(), -1, &body_rc, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
    SelectObject(dc, old_font);

    const int title_h = title_rc.bottom - title_rc.top;
    const int body_h = body_rc.bottom - body_rc.top;
    const int card_h = pad * 2 + title_h + 10 + body_h;
    if (card_h >= available_h) return;

    RECT card = {
        plot.right - margin - card_w,
        plot.top + margin,
        plot.right - margin,
        plot.top + margin + card_h
    };
    if (card.left < plot.left + margin) {
        card.left = plot.left + margin;
        card.right = card.left + card_w;
    }
    if (card.bottom > plot.bottom - margin) {
        card.bottom = plot.bottom - margin;
        card.top = card.bottom - card_h;
    }

    const COLORREF gap_orange = RGB(245, 140, 32);
    const COLORREF gap_fill = mix_color(g_theme->bg_panel, gap_orange, (g_theme == &kDarkTheme) ? 24 : 18);
    const COLORREF gap_border = mix_color(g_theme->separator, gap_orange, (g_theme == &kDarkTheme) ? 110 : 96);
    fill_rounded_rect(dc, card, gap_fill, gap_border, 14);

    RECT text = card;
    InflateRect(&text, -pad, -pad);
    SetBkMode(dc, TRANSPARENT);

    RECT title_rect = text;
    title_rect.bottom = title_rect.top + title_h;
    SelectObject(dc, title_font);
    SetTextColor(dc, gap_orange);
    DrawTextW(dc, title.c_str(), -1, &title_rect, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);

    HPEN sep = CreatePen(PS_SOLID, 1, mix_color(g_theme->separator, gap_orange, 72));
    HGDIOBJ old_pen = SelectObject(dc, sep);
    const int line_y = title_rect.bottom + 5;
    MoveToEx(dc, text.left, line_y, nullptr);
    LineTo(dc, text.right, line_y);
    SelectObject(dc, old_pen);
    DeleteObject(sep);

    RECT body_rect = text;
    body_rect.top = line_y + 7;
    SelectObject(dc, body_font);
    SetTextColor(dc, g_theme->text_primary);
    DrawTextW(dc, body.c_str(), -1, &body_rect, DT_LEFT | DT_TOP | DT_WORDBREAK | DT_NOPREFIX);

    SelectObject(dc, old_font);
}

void append_menu_popup_owner_draw(HMENU bar, HMENU popup, const std::wstring& text) {
    AppendMenuW(
        bar,
        MF_OWNERDRAW | MF_POPUP,
        reinterpret_cast<UINT_PTR>(popup),
        reinterpret_cast<LPCWSTR>(stash_menu_entry(text, true, true)));
}

void append_menu_item_owner_draw(HMENU menu, UINT id, const std::wstring& text) {
    AppendMenuW(
        menu,
        MF_OWNERDRAW | MF_STRING,
        id,
        reinterpret_cast<LPCWSTR>(stash_menu_entry(text, false, false)));
}

void modify_menu_item_owner_draw(HMENU menu, UINT id, const std::wstring& text) {
    ModifyMenuW(
        menu,
        id,
        MF_BYCOMMAND | MF_OWNERDRAW,
        id,
        reinterpret_cast<LPCWSTR>(stash_menu_entry(text, false, false)));
}

std::wstring menu_item_left_text(const std::wstring& text) {
    std::size_t tab = text.find(L'\t');
    return (tab == std::wstring::npos) ? text : text.substr(0, tab);
}

std::wstring menu_item_right_text(const std::wstring& text) {
    std::size_t tab = text.find(L'\t');
    return (tab == std::wstring::npos) ? L"" : text.substr(tab + 1);
}

void measure_owner_draw_menu(MEASUREITEMSTRUCT* mis) {
    if (!mis || mis->CtlType != ODT_MENU) return;
    const OwnerDrawMenuEntry* entry = reinterpret_cast<const OwnerDrawMenuEntry*>(mis->itemData);
    const std::wstring text = entry ? entry->text : L"";
    const std::wstring left = menu_item_left_text(text);
    const std::wstring right = menu_item_right_text(text);
    HDC dc = GetDC(g.main ? g.main : nullptr);
    HFONT font = (entry && entry->top_level && g.menu_font)
        ? g.menu_font
        : (g.ui_font ? g.ui_font : reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT)));
    HGDIOBJ old_font = SelectObject(dc, font);
    SIZE left_sz{};
    SIZE right_sz{};
    GetTextExtentPoint32W(dc, left.c_str(), static_cast<int>(left.size()), &left_sz);
    GetTextExtentPoint32W(dc, right.c_str(), static_cast<int>(right.size()), &right_sz);
    SelectObject(dc, old_font);
    ReleaseDC(g.main ? g.main : nullptr, dc);
    if (entry && entry->top_level) {
        mis->itemWidth = left_sz.cx + 24;
        mis->itemHeight = max(34u, static_cast<UINT>(left_sz.cy + 18));
        return;
    }

    const UINT check_col = 28;
    const UINT left_pad = 10;
    const UINT right_pad = 12;
    const UINT gap = right.empty() ? 0 : 24;
    const UINT arrow_space = (entry && entry->popup) ? 18 : 0;
    mis->itemWidth = check_col + left_pad + left_sz.cx + gap + right_sz.cx + arrow_space + right_pad;
    mis->itemHeight = max(24u, static_cast<UINT>(max(left_sz.cy, right_sz.cy) + 10));
}

void draw_owner_draw_menu(const DRAWITEMSTRUCT* dis) {
    if (!dis || dis->CtlType != ODT_MENU) return;
    auto blend = [](COLORREF a, COLORREF b, int weight_b) {
        weight_b = std::clamp(weight_b, 0, 255);
        const int weight_a = 255 - weight_b;
        return RGB(
            (GetRValue(a) * weight_a + GetRValue(b) * weight_b) / 255,
            (GetGValue(a) * weight_a + GetGValue(b) * weight_b) / 255,
            (GetBValue(a) * weight_a + GetBValue(b) * weight_b) / 255);
    };
    const OwnerDrawMenuEntry* entry = reinterpret_cast<const OwnerDrawMenuEntry*>(dis->itemData);
    const std::wstring text = entry ? entry->text : L"";
    const std::wstring left = menu_item_left_text(text);
    const std::wstring right = menu_item_right_text(text);
    RECT r = dis->rcItem;
    const bool selected = (dis->itemState & ODS_SELECTED) != 0;
    const bool hot = selected || (dis->itemState & ODS_HOTLIGHT) != 0;
    const bool disabled = (dis->itemState & ODS_DISABLED) != 0;
    const bool checked = (dis->itemState & ODS_CHECKED) != 0;
    COLORREF bg = hot ? g_theme->btn_hover : g_theme->bg_toolbar;
    COLORREF text_col = disabled ? g_theme->text_secondary : g_theme->text_primary;
    HBRUSH bg_brush = CreateSolidBrush(bg);
    FillRect(dis->hDC, &r, bg_brush);
    DeleteObject(bg_brush);
    SetBkMode(dis->hDC, TRANSPARENT);
    SetTextColor(dis->hDC, text_col);
    HFONT font = (entry && entry->top_level && g.menu_font)
        ? g.menu_font
        : (g.ui_font ? g.ui_font : reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT)));
    HGDIOBJ old_font = SelectObject(dis->hDC, font);
    if (entry && entry->top_level) {
        RECT text_rect = r;
        text_rect.bottom -= 1;
        if (hot && !disabled) {
            RECT hi = r;
            hi.left += 1;
            hi.right -= 1;
            hi.top += 1;
            hi.bottom -= 1;
            const int accent_weight = (g_theme == &kDarkTheme) ? 42 : 58;
            const COLORREF hi_fill = blend(g_theme->bg_toolbar, g_theme->accent, accent_weight);
            const COLORREF hi_border = blend(g_theme->btn_border, g_theme->accent, (g_theme == &kDarkTheme) ? 88 : 120);
            fill_rounded_rect(dis->hDC, hi, hi_fill, hi_border, 4);
            text_col = (g_theme == &kDarkTheme) ? RGB(255, 255, 255) : g_theme->accent_hover;
            SetTextColor(dis->hDC, text_col);

            RECT underline = { hi.left + 5, hi.bottom - 1, hi.right - 5, hi.bottom };
            HBRUSH underline_brush = CreateSolidBrush(g_theme->accent);
            FillRect(dis->hDC, &underline, underline_brush);
            DeleteObject(underline_brush);
        }
        DrawTextW(
            dis->hDC, left.c_str(), -1, &text_rect,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        SelectObject(dis->hDC, old_font);
        return;
    }

    RECT check_rect = r;
    check_rect.right = check_rect.left + 28;
    RECT left_rect = r;
    left_rect.left = check_rect.right + 10;
    left_rect.right = r.right - 12;
    if (!right.empty()) left_rect.right -= 80;
    if (entry && entry->popup) left_rect.right -= 18;
    RECT right_rect = r;
    right_rect.left = max(left_rect.right + 8, r.right - 92);
    right_rect.right = r.right - ((entry && entry->popup) ? 24 : 12);
    DrawTextW(
        dis->hDC, left.c_str(), -1, &left_rect,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    if (!right.empty()) {
        DrawTextW(
            dis->hDC, right.c_str(), -1, &right_rect,
            DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }

    if (checked) {
        RECT box = check_rect;
        box.left += 7;
        box.right = box.left + 14;
        box.top = r.top + max(0L, ((r.bottom - r.top) - 14) / 2);
        box.bottom = box.top + 14;
        HBRUSH accent_brush = CreateSolidBrush(g_theme->btn_active);
        FillRect(dis->hDC, &box, accent_brush);
        DeleteObject(accent_brush);

        HPEN pen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
        HGDIOBJ old_pen = SelectObject(dis->hDC, pen);
        MoveToEx(dis->hDC, box.left + 3, box.top + 7, nullptr);
        LineTo(dis->hDC, box.left + 6, box.top + 10);
        LineTo(dis->hDC, box.right - 3, box.top + 4);
        SelectObject(dis->hDC, old_pen);
        DeleteObject(pen);
    }

    if (entry && entry->popup) {
        POINT pts[3] = {
            { r.right - 12, r.top + (r.bottom - r.top) / 2 - 4 },
            { r.right - 12, r.top + (r.bottom - r.top) / 2 + 4 },
            { r.right - 7,  r.top + (r.bottom - r.top) / 2 }
        };
        HBRUSH arrow_brush = CreateSolidBrush(text_col);
        HPEN arrow_pen = CreatePen(PS_SOLID, 1, text_col);
        HGDIOBJ old_brush = SelectObject(dis->hDC, arrow_brush);
        HGDIOBJ old_pen = SelectObject(dis->hDC, arrow_pen);
        Polygon(dis->hDC, pts, 3);
        SelectObject(dis->hDC, old_pen);
        SelectObject(dis->hDC, old_brush);
        DeleteObject(arrow_pen);
        DeleteObject(arrow_brush);
    }
    SelectObject(dis->hDC, old_font);
}

void measure_settings_combo_item(MEASUREITEMSTRUCT* mis) {
    if (!mis || mis->CtlType != ODT_COMBOBOX) return;
    mis->itemHeight = 22;
}

void draw_settings_combo_item(const DRAWITEMSTRUCT* dis) {
    if (!dis || dis->CtlType != ODT_COMBOBOX) return;
    HWND combo = dis->hwndItem;
    RECT r = dis->rcItem;
    const bool disabled = (dis->itemState & ODS_DISABLED) != 0;
    const bool selected = (dis->itemState & ODS_SELECTED) != 0;
    const bool combo_edit = (dis->itemState & ODS_COMBOBOXEDIT) != 0;
    COLORREF bg = combo_edit ? g_theme->bg_plot : (selected ? g_theme->btn_hover : g_theme->bg_plot);
    COLORREF text_col = disabled ? g_theme->text_secondary : g_theme->text_primary;

    HBRUSH bg_brush = CreateSolidBrush(bg);
    FillRect(dis->hDC, &r, bg_brush);
    DeleteObject(bg_brush);

    if (combo_edit) {
        HPEN border = CreatePen(PS_SOLID, 1, g_theme->btn_border);
        HGDIOBJ old_pen = SelectObject(dis->hDC, border);
        HGDIOBJ old_brush = SelectObject(dis->hDC, GetStockObject(NULL_BRUSH));
        Rectangle(dis->hDC, r.left, r.top, r.right, r.bottom);
        SelectObject(dis->hDC, old_brush);
        SelectObject(dis->hDC, old_pen);
        DeleteObject(border);
    }

    UINT item = dis->itemID;
    if (item == static_cast<UINT>(-1) && combo) {
        LRESULT sel = SendMessageW(combo, CB_GETCURSEL, 0, 0);
        if (sel != CB_ERR) item = static_cast<UINT>(sel);
    }

    std::wstring text;
    if (combo && item != static_cast<UINT>(-1)) {
        LRESULT len = SendMessageW(combo, CB_GETLBTEXTLEN, item, 0);
        if (len >= 0) {
            text.resize(static_cast<std::size_t>(len));
            SendMessageW(combo, CB_GETLBTEXT, item, reinterpret_cast<LPARAM>(text.data()));
        }
    }

    RECT text_rect = r;
    text_rect.left += 8;
    if (combo_edit) text_rect.right -= 8;
    SetBkMode(dis->hDC, TRANSPARENT);
    SetTextColor(dis->hDC, text_col);
    HFONT font = g.ui_font ? g.ui_font : reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    HGDIOBJ old_font = SelectObject(dis->hDC, font);
    DrawTextW(dis->hDC, text.c_str(), -1, &text_rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(dis->hDC, old_font);
}

void measure_settings_list_item(MEASUREITEMSTRUCT* mis) {
    if (!mis || mis->CtlType != ODT_LISTBOX) return;
    if (mis->CtlID != IDC_SET_HOTKEY_LIST) return;
    mis->itemHeight = 24;
}

void draw_settings_list_item(const DRAWITEMSTRUCT* dis) {
    if (!dis || dis->CtlType != ODT_LISTBOX || dis->CtlID != IDC_SET_HOTKEY_LIST) return;

    RECT r = dis->rcItem;
    const bool disabled = (dis->itemState & ODS_DISABLED) != 0;
    const bool selected = (dis->itemState & ODS_SELECTED) != 0;
    const bool focused = (dis->itemState & ODS_FOCUS) != 0;
    const COLORREF base_bg = g_theme->bg_plot;
    const COLORREF row_bg = selected ? mix_color(base_bg, g_theme->accent, 24) : base_bg;
    const COLORREF row_border = selected ? mix_color(g_theme->accent, g_theme->btn_border, 78) : base_bg;
    const COLORREF text_col = disabled ? g_theme->text_secondary
        : (selected ? ((g_theme == &kDarkTheme) ? RGB(255, 255, 255) : RGB(12, 42, 78))
                    : g_theme->text_primary);

    HBRUSH bg = CreateSolidBrush(base_bg);
    FillRect(dis->hDC, &r, bg);
    DeleteObject(bg);

    if (dis->itemID != static_cast<UINT>(-1)) {
        std::wstring text;
        LRESULT len = SendMessageW(dis->hwndItem, LB_GETTEXTLEN, dis->itemID, 0);
        if (len >= 0) {
            text.resize(static_cast<std::size_t>(len));
            SendMessageW(dis->hwndItem, LB_GETTEXT, dis->itemID, reinterpret_cast<LPARAM>(text.data()));
        }

        if (selected) {
            RECT fill = r;
            InflateRect(&fill, -2, -2);
            fill_rounded_rect(dis->hDC, fill, row_bg, row_border, 5);
        }

        RECT text_rect = r;
        text_rect.left += 10;
        text_rect.right -= 10;
        SetBkMode(dis->hDC, TRANSPARENT);
        SetTextColor(dis->hDC, text_col);
        HFONT font = g.ui_font ? g.ui_font : reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        HGDIOBJ old_font = SelectObject(dis->hDC, font);
        DrawTextW(dis->hDC, text.c_str(), -1, &text_rect,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
        SelectObject(dis->hDC, old_font);
    }

    if (focused && dis->itemID != static_cast<UINT>(-1)) {
        RECT focus = r;
        InflateRect(&focus, -4, -4);
        DrawFocusRect(dis->hDC, &focus);
    }
}

void draw_settings_group_box(HDC dc, const RECT& r, const wchar_t* text) {
    RECT text_rect = r;
    text_rect.left += 10;
    text_rect.right -= 10;
    text_rect.top += 1;

    HFONT font = g.ui_font ? g.ui_font : reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    HGDIOBJ old_font = SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, g_theme->text_primary);

    SIZE sz{};
    const int text_len = lstrlenW(text);
    GetTextExtentPoint32W(dc, text, text_len, &sz);

    const int text_x = r.left + 12;
    const int text_y = r.top + 1;
    const int gap_left = text_x - 6;
    const int gap_right = text_x + sz.cx + 6;
    const int line_y = r.top + 10;

    HPEN pen = CreatePen(PS_SOLID, 1, g_theme->btn_border);
    HGDIOBJ old_pen = SelectObject(dc, pen);
    MoveToEx(dc, r.left, line_y, nullptr); LineTo(dc, static_cast<int>(max<LONG>(r.left, gap_left)), line_y);
    MoveToEx(dc, static_cast<int>(min<LONG>(r.right, gap_right)), line_y, nullptr); LineTo(dc, r.right, line_y);
    MoveToEx(dc, r.left, line_y, nullptr); LineTo(dc, r.left, r.bottom);
    MoveToEx(dc, r.right - 1, line_y, nullptr); LineTo(dc, r.right - 1, r.bottom);
    MoveToEx(dc, r.left, r.bottom - 1, nullptr); LineTo(dc, r.right, r.bottom - 1);
    SelectObject(dc, old_pen);
    DeleteObject(pen);

    SetBkColor(dc, g_theme->bg_panel);
    ExtTextOutW(dc, text_x, text_y, ETO_OPAQUE, nullptr, text, text_len, nullptr);
    SelectObject(dc, old_font);
}

const wchar_t* settings_button_text() {
    return (g_str == &kEn) ? L"Settings" : L"Настройки";
}

const wchar_t* channel_show_all_text() {
    return (g_str == &kEn) ? L"All" : L"Все";
}

const wchar_t* channel_hide_all_text() {
    return (g_str == &kEn) ? L"None" : L"Скрыть";
}

void set_all_channels_visible(bool visible) {
    for (std::size_t i = 0; i < g.visible.size(); ++i) {
        g.visible[i] = visible ? 1 : 0;
        if (i < g.checks.size()) {
            set_toggle_checked(g.checks[i], visible);
        }
    }
    invalidate_plot_analysis_cache();
}

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
    return id == IDC_SET_PERF_FAST_OPEN ||
           id == IDC_SET_PERF_FAST_WORK ||
           id == IDC_SET_PERF_COMFORT ||
           id == IDC_SET_GAP_MARKERS;
}

bool is_welcome_checkbox_id(int id) {
    return id == IDW_PERF_FAST_OPEN ||
           id == IDW_PERF_FAST_WORK ||
           id == IDW_PERF_COMFORT;
}

bool uses_manual_toggle_state(HWND hwnd) {
    if (!hwnd) return false;
    const int id = GetDlgCtrlID(hwnd);
    return is_channel_checkbox_id(id) ||
           is_side_toggle_id(id) ||
           id == IDC_SET_PERF_FAST_OPEN ||
           id == IDC_SET_PERF_FAST_WORK ||
           id == IDC_SET_PERF_COMFORT ||
           id == IDC_SET_GAP_MARKERS ||
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
                               COLORREF surface_bg = CLR_INVALID) {
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

void refresh_theme_windows() {
    auto redraw = [](HWND wnd) {
        if (!wnd || !IsWindow(wnd)) return;
        RedrawWindow(
            wnd, nullptr, nullptr,
            RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN | RDW_UPDATENOW);
    };
    redraw(g.main);
    redraw(g.settings_wnd);
    redraw(g.welcome_wnd);
    redraw(g.channel_edit);
    for (HWND h : g.checks) redraw(h);
    for (HWND h : g.check_labels) redraw(h);
}

void apply_theme_choice(const Theme* theme) {
    if (!theme || g_theme == theme) return;
    g_theme = theme;
    save_app_settings();
    update_theme_brushes();
    if (g.menu) {
        MENUINFO mi{};
        mi.cbSize = sizeof(mi);
        mi.fMask = MIM_BACKGROUND | MIM_APPLYTOSUBMENUS;
        mi.hbrBack = CreateSolidBrush(g_theme->bg_toolbar);
        SetMenuInfo(g.menu, &mi);
    }
    sync_menu();
    if (g.settings_wnd) refresh_settings_controls();
    refresh_theme_windows();
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

void show_hotkeys() {
    static ATOM atom = 0;
    if (!atom) {
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = HotkeysDialogProc;
        wc.hInstance = reinterpret_cast<HINSTANCE>(GetWindowLongPtr(g.main, GWLP_HINSTANCE));
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;
        wc.lpszClassName = L"LvmHotkeysDialog";
        atom = RegisterClassExW(&wc);
    }

    if (g_hotkeys_dialog.wnd && IsWindow(g_hotkeys_dialog.wnd)) {
        ShowWindow(g_hotkeys_dialog.wnd, SW_RESTORE);
        SetForegroundWindow(g_hotkeys_dialog.wnd);
        if (g_hotkeys_dialog.list) SetFocus(g_hotkeys_dialog.list);
        return;
    }

    RECT work_area{};
    if (HMONITOR monitor = MonitorFromWindow(g.main ? g.main : GetDesktopWindow(), MONITOR_DEFAULTTONEAREST)) {
        MONITORINFO mi{};
        mi.cbSize = sizeof(mi);
        if (GetMonitorInfoW(monitor, &mi)) work_area = mi.rcWork;
    }
    if (work_area.right <= work_area.left || work_area.bottom <= work_area.top) {
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &work_area, 0);
    }

    const SIZE desired_client_size = hotkeys_dialog_client_size();
    const int max_client_w = max(420, static_cast<int>((work_area.right - work_area.left) - 24));
    const int max_client_h = max(300, static_cast<int>((work_area.bottom - work_area.top) - 24));
    RECT dlg_rc{
        0, 0,
        min(static_cast<int>(desired_client_size.cx), max_client_w),
        min(static_cast<int>(desired_client_size.cy), max_client_h)
    };
    AdjustWindowRectEx(&dlg_rc, WS_CAPTION | WS_SYSMENU | WS_POPUP, FALSE, WS_EX_DLGMODALFRAME | WS_EX_TOOLWINDOW);

    g_hotkeys_dialog.done = false;
    g_hotkeys_dialog.wnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOOLWINDOW,
        L"LvmHotkeysDialog",
        g_str->dlg_hotkeys_title,
        WS_CAPTION | WS_SYSMENU | WS_POPUP | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, dlg_rc.right - dlg_rc.left, dlg_rc.bottom - dlg_rc.top,
        g.main, nullptr,
        reinterpret_cast<HINSTANCE>(GetWindowLongPtr(g.main, GWLP_HINSTANCE)),
        nullptr);
    if (!g_hotkeys_dialog.wnd) return;

    RECT mr{}, wr{};
    GetWindowRect(g.main, &mr);
    GetWindowRect(g_hotkeys_dialog.wnd, &wr);
    const int window_w = wr.right - wr.left;
    const int window_h = wr.bottom - wr.top;
    int x = mr.left + ((mr.right - mr.left) - window_w) / 2;
    int y = mr.top + ((mr.bottom - mr.top) - window_h) / 2;
    x = std::clamp(x,
        static_cast<int>(work_area.left),
        max(static_cast<int>(work_area.left), static_cast<int>(work_area.right - window_w)));
    y = std::clamp(y,
        static_cast<int>(work_area.top),
        max(static_cast<int>(work_area.top), static_cast<int>(work_area.bottom - window_h)));
    SetWindowPos(
        g_hotkeys_dialog.wnd, HWND_TOP,
        x, y,
        0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
    ShowWindow(g_hotkeys_dialog.wnd, SW_SHOWNORMAL);
    UpdateWindow(g_hotkeys_dialog.wnd);
    if (g_hotkeys_dialog.list) SetFocus(g_hotkeys_dialog.list);
}

void show_about() {
    show_welcome(GetModuleHandleW(nullptr));
}

// Refresh every checkable menu item from the current app state. Cheap, so we
// just call it whenever a toggle changes (menu, toolbar, or accelerator).
void sync_menu() {
    if (!g.menu) return;
    auto chk = [&](UINT id, bool on) {
        CheckMenuItem(g.menu, id, MF_BYCOMMAND | (on ? MF_CHECKED : MF_UNCHECKED));
    };
    chk(IDM_MODE_TIME, !g.freq_mode);
    chk(IDM_MODE_FREQ, g.freq_mode);
    chk(IDM_VISMOOTH, g.visual_smooth);
    chk(IDM_VPAN, g.vertical_pan);
    chk(IDC_MEASURE, g.measure_mode);
    chk(IDM_ADD_MARKER, g.pending_marker);
    chk(IDM_ADD_VLINE, g.pending_line == 1);
    chk(IDM_ADD_HLINE, g.pending_line == 2);
    chk(IDC_AUTOY, g.auto_y);
    chk(IDM_THEME, g_theme == &kDarkTheme);
    chk(IDM_LANG_RU, g_str == &kRu);
    chk(IDM_LANG_EN, g_str == &kEn);
    std::wstring theme_label = menu_text(g_theme == &kDarkTheme ? g_str->theme_light : g_str->theme_dark, IDM_THEME);
    modify_menu_item_owner_draw(g.menu, IDM_THEME, theme_label);
    redraw_toolbar_buttons();
}

void set_mode(bool freq_mode) {
    if (g.freq_mode == freq_mode) return;
    g.freq_mode = freq_mode;
    if (g.freq_mode) {
        stop_play();
        hide_gap_details_card();
    }
    if (g.freq_mode) {
        compute_spectrum_from_current_source();
        g.freq_start = 0.0;
        g.freq_end = g.spec_valid ? g.spec.nyquist : 1.0;
    }
    sync_menu();
    set_status();
    InvalidateRect(g.main, nullptr, TRUE);
}

HMENU make_menu() {
    HMENU bar = CreateMenu();
    const wchar_t* saveas_menu = (g_str == &kEn) ? L"Save as…" : L"Сохранить как…";

    {
        const bool en = (g_str == &kEn);

        HMENU file = CreatePopupMenu();
        std::wstring open_text = menu_text(en ? L"Open file…" : L"Открыть файл…", IDC_OPEN);
        std::wstring save_png_text = menu_text(en ? L"Save PNG…" : L"Сохранить PNG…", IDC_SAVEPNG);
        std::wstring save_as_text = menu_text(saveas_menu, IDC_SAVECSV);
        std::wstring undo_text = menu_text(en ? L"Undo" : L"Отменить", IDM_UNDO);
        std::wstring redo_text = menu_text(en ? L"Redo" : L"Повторить", IDM_REDO);
        append_menu_item_owner_draw(file, IDC_OPEN, open_text);
        append_menu_item_owner_draw(file, IDC_SAVEPNG, save_png_text);
        append_menu_item_owner_draw(file, IDC_SAVECSV, save_as_text);
        AppendMenuW(file, MF_SEPARATOR, 0, nullptr);
        append_menu_item_owner_draw(file, IDM_UNDO, undo_text);
        append_menu_item_owner_draw(file, IDM_REDO, redo_text);
        AppendMenuW(file, MF_SEPARATOR, 0, nullptr);
        append_menu_item_owner_draw(file, IDM_EXIT, en ? L"Exit\tAlt+F4" : L"Выход\tAlt+F4");
        append_menu_popup_owner_draw(bar, file, en ? L"File" : L"Файл");

        HMENU view = CreatePopupMenu();
        std::wstring mode_time_text = menu_text(en ? L"Time" : L"Время", IDM_MODE_TIME);
        std::wstring mode_freq_text = menu_text(en ? L"Hz (FFT)" : L"Гц (FFT)", IDM_MODE_FREQ);
        std::wstring zoom_in_text = menu_text(en ? L"Zoom in" : L"Увеличить", IDC_ZOOMIN);
        std::wstring zoom_out_text = menu_text(en ? L"Zoom out" : L"Уменьшить", IDC_ZOOMOUT);
        std::wstring reset_text = menu_text(en ? L"Reset view" : L"Сбросить вид", IDC_RESET);
        std::wstring start_text = menu_text(en ? L"Go to start" : L"В начало", IDC_GOTO_START);
        std::wstring end_text = menu_text(en ? L"Go to end" : L"В конец", IDC_GOTO_END);
        std::wstring autoy_text = menu_text(en ? L"Auto zoom" : L"Автомасштабирование", IDC_AUTOY);
        std::wstring smooth_text = menu_text(en ? L"Smoothing" : L"Сглаживание", IDM_VISMOOTH);
        std::wstring vpan_text = menu_text(en ? L"Vertical pan" : L"Вертикальное панорамирование", IDM_VPAN);
        std::wstring play_text = menu_text(en ? L"Play / Pause" : L"Старт/стоп", IDC_PLAY);
        std::wstring theme_text = menu_text(en ? L"Dark theme" : L"Тёмная тема", IDM_THEME);
        append_menu_item_owner_draw(view, IDM_MODE_TIME, mode_time_text);
        append_menu_item_owner_draw(view, IDM_MODE_FREQ, mode_freq_text);
        AppendMenuW(view, MF_SEPARATOR, 0, nullptr);
        append_menu_item_owner_draw(view, IDC_ZOOMIN, zoom_in_text);
        append_menu_item_owner_draw(view, IDC_ZOOMOUT, zoom_out_text);
        append_menu_item_owner_draw(view, IDC_RESET, reset_text);
        append_menu_item_owner_draw(view, IDC_GOTO_START, start_text);
        append_menu_item_owner_draw(view, IDC_GOTO_END, end_text);
        AppendMenuW(view, MF_SEPARATOR, 0, nullptr);
        append_menu_item_owner_draw(view, IDC_AUTOY, autoy_text);
        append_menu_item_owner_draw(view, IDM_VISMOOTH, smooth_text);
        append_menu_item_owner_draw(view, IDM_VPAN, vpan_text);
        append_menu_item_owner_draw(view, IDC_PLAY, play_text);
        append_menu_item_owner_draw(view, IDM_SPEED_CUSTOM, speed_menu_text());
        append_menu_item_owner_draw(view, IDM_THEME, theme_text);
        append_menu_popup_owner_draw(bar, view, en ? L"View" : L"Вид");

        HMENU tools = CreatePopupMenu();
        std::wstring measure_text = menu_text(en ? L"Points" : L"Точки", IDC_MEASURE);
        std::wstring marker_text = menu_text(en ? L"Marker" : L"Маркер", IDM_ADD_MARKER);
        std::wstring vline_text = menu_text(en ? L"Vertical line" : L"Вертикальная линия", IDM_ADD_VLINE);
        std::wstring vline_exact_text = menu_text(en ? L"Vertical line (exact)" : L"Вертикальная линия (точно)", IDM_ADD_VLINE_EXACT);
        std::wstring hline_text = menu_text(en ? L"Horizontal line" : L"Горизонтальная линия", IDM_ADD_HLINE);
        std::wstring hline_exact_text = menu_text(en ? L"Horizontal line (exact)" : L"Горизонтальная линия (точно)", IDM_ADD_HLINE_EXACT);
        append_menu_item_owner_draw(tools, IDC_MEASURE, measure_text);
        append_menu_item_owner_draw(tools, IDM_ADD_MARKER, marker_text);
        append_menu_item_owner_draw(tools, IDM_ADD_VLINE, vline_text);
        append_menu_item_owner_draw(tools, IDM_ADD_VLINE_EXACT, vline_exact_text);
        append_menu_item_owner_draw(tools, IDM_ADD_HLINE, hline_text);
        append_menu_item_owner_draw(tools, IDM_ADD_HLINE_EXACT, hline_exact_text);
        AppendMenuW(tools, MF_SEPARATOR, 0, nullptr);
        append_menu_item_owner_draw(tools, IDM_CLEAR_POINTS, en ? L"Clear points" : L"Очистить точки");
        append_menu_item_owner_draw(tools, IDM_CLEAR_MARKERS, en ? L"Clear markers" : L"Очистить маркеры");
        append_menu_item_owner_draw(tools, IDM_CLEAR_LINES, en ? L"Clear lines" : L"Очистить линии");
        append_menu_popup_owner_draw(bar, tools, en ? L"Tools" : L"Инструменты");

        HMENU settings = CreatePopupMenu();
        append_menu_item_owner_draw(settings, IDC_PTSETTINGS, en ? L"General settings" : L"Общие настройки");
        append_menu_popup_owner_draw(bar, settings, en ? L"Settings" : L"Настройки");

        HMENU help = CreatePopupMenu();
        std::wstring hotkeys_text = menu_text(en ? L"Keyboard shortcuts" : L"Горячие клавиши", IDM_HOTKEYS);
        append_menu_item_owner_draw(help, IDM_HOTKEYS, hotkeys_text);
        append_menu_item_owner_draw(help, IDM_ABOUT, en ? L"About" : L"О программе");
        append_menu_popup_owner_draw(bar, help, en ? L"Help" : L"Справка");
        return bar;
    }

    {
        const bool en = (g_str == &kEn);
        const wchar_t* menu_file = en ? L"File" : L"Файл";
        const wchar_t* menu_view = en ? L"View" : L"Вид";
        const wchar_t* menu_points = en ? L"Points" : L"Точки";
        const wchar_t* menu_lines = en ? L"Lines" : L"Линии";
        const wchar_t* menu_markers = en ? L"Markers" : L"Маркеры";
        const wchar_t* menu_help = en ? L"Help" : L"Справка";

        HMENU file = CreatePopupMenu();
        std::wstring open_text = menu_text(en ? L"Open file…" : L"Открыть файл…", IDC_OPEN);
        std::wstring save_png_text = menu_text(en ? L"Save PNG…" : L"Сохранить PNG…", IDC_SAVEPNG);
        std::wstring save_as_text = menu_text(saveas_menu, IDC_SAVECSV);
        std::wstring undo_text = menu_text(en ? L"Undo" : L"Отменить", IDM_UNDO);
        std::wstring redo_text = menu_text(en ? L"Redo" : L"Повторить", IDM_REDO);
        append_menu_item_owner_draw(file, IDC_OPEN, open_text);
        append_menu_item_owner_draw(file, IDC_SAVEPNG, save_png_text);
        append_menu_item_owner_draw(file, IDC_SAVECSV, save_as_text);
        AppendMenuW(file, MF_SEPARATOR, 0, nullptr);
        append_menu_item_owner_draw(file, IDM_UNDO, undo_text);
        append_menu_item_owner_draw(file, IDM_REDO, redo_text);
        AppendMenuW(file, MF_SEPARATOR, 0, nullptr);
        append_menu_item_owner_draw(file, IDM_EXIT, en ? L"Exit\tAlt+F4" : L"Выход\tAlt+F4");
        append_menu_popup_owner_draw(bar, file, menu_file);

        HMENU view = CreatePopupMenu();
        std::wstring mode_time_text = menu_text(en ? L"Time" : L"Время", IDM_MODE_TIME);
        std::wstring mode_freq_text = menu_text(en ? L"Hz (FFT)" : L"Гц (FFT)", IDM_MODE_FREQ);
        std::wstring zoom_in_text = menu_text(en ? L"Zoom in" : L"Увеличить", IDC_ZOOMIN);
        std::wstring zoom_out_text = menu_text(en ? L"Zoom out" : L"Уменьшить", IDC_ZOOMOUT);
        std::wstring reset_text = menu_text(en ? L"Reset view" : L"Сбросить вид", IDC_RESET);
        std::wstring start_text = menu_text(en ? L"Go to start" : L"В начало", IDC_GOTO_START);
        std::wstring end_text = menu_text(en ? L"Go to end" : L"В конец", IDC_GOTO_END);
        std::wstring autoy_text = menu_text(en ? L"Auto zoom" : L"Автомасштабирование", IDC_AUTOY);
        std::wstring smooth_text = menu_text(en ? L"Smoothing" : L"Сглаживание", IDM_VISMOOTH);
        std::wstring vpan_text = menu_text(en ? L"Vertical pan" : L"Вертикальное панорамирование", IDM_VPAN);
        std::wstring play_text = menu_text(en ? L"Play / Pause" : L"Старт/стоп", IDC_PLAY);
        std::wstring theme_text = menu_text(en ? L"Dark theme" : L"Тёмная тема", IDM_THEME);
        append_menu_item_owner_draw(view, IDM_MODE_TIME, mode_time_text);
        append_menu_item_owner_draw(view, IDM_MODE_FREQ, mode_freq_text);
        AppendMenuW(view, MF_SEPARATOR, 0, nullptr);
        append_menu_item_owner_draw(view, IDC_ZOOMIN, zoom_in_text);
        append_menu_item_owner_draw(view, IDC_ZOOMOUT, zoom_out_text);
        append_menu_item_owner_draw(view, IDC_RESET, reset_text);
        append_menu_item_owner_draw(view, IDC_GOTO_START, start_text);
        append_menu_item_owner_draw(view, IDC_GOTO_END, end_text);
        AppendMenuW(view, MF_SEPARATOR, 0, nullptr);
        append_menu_item_owner_draw(view, IDC_AUTOY, autoy_text);
        append_menu_item_owner_draw(view, IDM_VISMOOTH, smooth_text);
        append_menu_item_owner_draw(view, IDM_VPAN, vpan_text);
        append_menu_item_owner_draw(view, IDC_PLAY, play_text);
        append_menu_item_owner_draw(view, IDM_SPEED_CUSTOM, speed_menu_text());
        append_menu_item_owner_draw(view, IDM_THEME, theme_text);
        append_menu_popup_owner_draw(bar, view, menu_view);

        HMENU meas = CreatePopupMenu();
        std::wstring measure_text = menu_text(en ? L"Points" : L"Точки", IDC_MEASURE);
        append_menu_item_owner_draw(meas, IDC_MEASURE, measure_text);
        append_menu_item_owner_draw(meas, IDC_PTSETTINGS, en ? L"Settings" : L"Настройки");
        AppendMenuW(meas, MF_SEPARATOR, 0, nullptr);
        append_menu_item_owner_draw(meas, IDM_CLEAR_POINTS, en ? L"Clear\tDelete" : L"Очистить\tDelete");
        append_menu_popup_owner_draw(bar, meas, menu_points);

        HMENU lines = CreatePopupMenu();
        std::wstring vline_text = menu_text(en ? L"Vertical (click)" : L"Вертикальная (клик)", IDM_ADD_VLINE);
        std::wstring vline_exact_text = menu_text(en ? L"Vertical (exact)" : L"Вертикальная (точно)", IDM_ADD_VLINE_EXACT);
        std::wstring hline_text = menu_text(en ? L"Horizontal (click)" : L"Горизонтальная (клик)", IDM_ADD_HLINE);
        std::wstring hline_exact_text = menu_text(en ? L"Horizontal (exact)" : L"Горизонтальная (точно)", IDM_ADD_HLINE_EXACT);
        append_menu_item_owner_draw(lines, IDM_ADD_VLINE, vline_text);
        append_menu_item_owner_draw(lines, IDM_ADD_VLINE_EXACT, vline_exact_text);
        append_menu_item_owner_draw(lines, IDM_ADD_HLINE, hline_text);
        append_menu_item_owner_draw(lines, IDM_ADD_HLINE_EXACT, hline_exact_text);
        AppendMenuW(lines, MF_SEPARATOR, 0, nullptr);
        append_menu_item_owner_draw(lines, IDM_CLEAR_LINES, en ? L"Clear" : L"Очистить");
        append_menu_popup_owner_draw(bar, lines, menu_lines);

        HMENU markers = CreatePopupMenu();
        std::wstring marker_text = menu_text(en ? L"Add" : L"Добавить", IDM_ADD_MARKER);
        append_menu_item_owner_draw(markers, IDM_ADD_MARKER, marker_text);
        AppendMenuW(markers, MF_SEPARATOR, 0, nullptr);
        append_menu_item_owner_draw(markers, IDM_CLEAR_MARKERS, en ? L"Clear" : L"Очистить");
        append_menu_popup_owner_draw(bar, markers, menu_markers);

        HMENU help = CreatePopupMenu();
        std::wstring hotkeys_text = menu_text(en ? L"Keyboard shortcuts" : L"Горячие клавиши", IDM_HOTKEYS);
        append_menu_item_owner_draw(help, IDM_HOTKEYS, hotkeys_text);
        append_menu_item_owner_draw(help, IDM_ABOUT, en ? L"About" : L"О программе");
        append_menu_popup_owner_draw(bar, help, menu_help);
        return bar;
    }

    HMENU file = CreatePopupMenu();
    append_menu_item_owner_draw(file, IDC_OPEN, L"Открыть файл…\tCtrl+O");
    append_menu_item_owner_draw(file, IDC_SAVEPNG, L"Сохранить PNG…\tCtrl+S");
    append_menu_item_owner_draw(file, IDC_SAVECSV, (g_str == &kEn) ? L"Save as…\tCtrl+Shift+S" : L"Сохранить как…\tCtrl+Shift+S");
    AppendMenuW(file, MF_SEPARATOR, 0, nullptr);
    append_menu_item_owner_draw(file, IDM_UNDO, L"Отменить\tCtrl+Z");
    append_menu_item_owner_draw(file, IDM_REDO, L"Повторить\tCtrl+Shift+Z");
    AppendMenuW(file, MF_SEPARATOR, 0, nullptr);
    append_menu_item_owner_draw(file, IDM_EXIT, L"Выход\tAlt+F4");
    append_menu_popup_owner_draw(bar, file, L"Файл");

    HMENU view = CreatePopupMenu();
    append_menu_item_owner_draw(view, IDC_MODE, L"Время / Гц\tM");
    AppendMenuW(view, MF_SEPARATOR, 0, nullptr);
    append_menu_item_owner_draw(view, IDC_ZOOMIN, L"Увеличить\t+");
    append_menu_item_owner_draw(view, IDC_ZOOMOUT, L"Уменьшить\t−");
    append_menu_item_owner_draw(view, IDC_RESET, L"Сбросить вид\tHome");
    append_menu_item_owner_draw(view, IDC_GOTO_START, L"В начало\tCtrl+Home");
    append_menu_item_owner_draw(view, IDC_GOTO_END, L"В конец\tCtrl+End");
    AppendMenuW(view, MF_SEPARATOR, 0, nullptr);
    append_menu_item_owner_draw(view, IDC_AUTOY, (g_str == &kEn) ? L"Auto zoom" : L"Автомасштабирование");
    append_menu_item_owner_draw(view, IDM_VISMOOTH, L"Сглаживание\tC");
    append_menu_item_owner_draw(view, IDM_VPAN, L"Вертикальное панорамирование\tP");
    append_menu_item_owner_draw(view, IDC_PLAY, L"Старт/стоп\tПробел");
    append_menu_item_owner_draw(view, IDM_SPEED_CUSTOM, speed_menu_text());
    append_menu_item_owner_draw(view, IDM_THEME, L"Тёмная тема\tT");
    append_menu_popup_owner_draw(bar, view, L"Вид");

    HMENU meas = CreatePopupMenu();
    append_menu_item_owner_draw(meas, IDC_MEASURE, L"Точки\tV");
    append_menu_item_owner_draw(meas, IDC_PTSETTINGS, L"Настройки");
    AppendMenuW(meas, MF_SEPARATOR, 0, nullptr);
    append_menu_item_owner_draw(meas, IDM_CLEAR_POINTS, L"Очистить\tDelete");
    append_menu_popup_owner_draw(bar, meas, L"Точки");

    HMENU lines = CreatePopupMenu();
    append_menu_item_owner_draw(lines, IDM_ADD_VLINE, L"Вертикальная\tL");
    append_menu_item_owner_draw(lines, IDM_ADD_VLINE_EXACT, L"Вертикальная (точно)...");
    append_menu_item_owner_draw(lines, IDM_ADD_HLINE, L"Горизонтальная\tH");
    append_menu_item_owner_draw(lines, IDM_ADD_HLINE_EXACT, L"Горизонтальная (точно)...");
    AppendMenuW(lines, MF_SEPARATOR, 0, nullptr);
    append_menu_item_owner_draw(lines, IDM_CLEAR_LINES, L"Очистить");
    append_menu_popup_owner_draw(bar, lines, L"Линии");

    HMENU markers = CreatePopupMenu();
    append_menu_item_owner_draw(markers, IDM_ADD_MARKER, L"Добавить\tK");
    AppendMenuW(markers, MF_SEPARATOR, 0, nullptr);
    append_menu_item_owner_draw(markers, IDM_CLEAR_MARKERS, L"Очистить");
    append_menu_popup_owner_draw(bar, markers, L"Маркеры");

    HMENU help = CreatePopupMenu();
    append_menu_item_owner_draw(help, IDM_HOTKEYS, L"Горячие клавиши\tF1");
    append_menu_item_owner_draw(help, IDM_ABOUT, L"О программе");
    append_menu_popup_owner_draw(bar, help, L"Справка");

    return bar;
}

// ---- measurement-point settings panel (modeless) -------------------------

COLORREF g_custom_colors[16] = {0};

const wchar_t* settings_window_title() {
    return (g_str == &kEn) ? L"Settings" : L"Настройки";
}

int hotkey_binding_index(int command) {
    for (std::size_t i = 0; i < g.hotkeys.size(); ++i)
        if (g.hotkeys[i].command == command) return static_cast<int>(i);
    return -1;
}

void set_hotkey_binding(int command, BYTE fvirt, WORD key) {
    int idx = hotkey_binding_index(command);
    if (idx >= 0) {
        g.hotkeys[static_cast<std::size_t>(idx)].fvirt = fvirt;
        g.hotkeys[static_cast<std::size_t>(idx)].key = key;
    } else {
        g.hotkeys.push_back({command, fvirt, key});
    }
}

HotkeyBinding default_hotkey_for_command(int command) {
    for (const auto& hk : default_hotkeys())
        if (hk.command == command) return hk;
    return {command, FVIRTKEY, 0};
}

int find_conflicting_hotkey(BYTE fvirt, WORD key, int except_command) {
    if (key == 0) return 0;
    for (const auto& hk : g.hotkeys) {
        if (hk.command == except_command) continue;
        if (hk.key == key && hk.fvirt == fvirt) return hk.command;
    }
    return 0;
}

void hotkey_combo_add(HWND combo, const wchar_t* text, WORD key) {
    int idx = static_cast<int>(SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text)));
    SendMessageW(combo, CB_SETITEMDATA, idx, key);
}

void populate_hotkey_key_combo(HWND combo) {
    SendMessageW(combo, CB_RESETCONTENT, 0, 0);
    hotkey_combo_add(combo, g_str == &kEn ? L"None" : L"Нет", 0);
    for (wchar_t ch = L'A'; ch <= L'Z'; ++ch) {
        wchar_t txt[2] = {ch, 0};
        hotkey_combo_add(combo, txt, static_cast<WORD>(ch));
    }
    for (wchar_t ch = L'0'; ch <= L'9'; ++ch) {
        wchar_t txt[2] = {ch, 0};
        hotkey_combo_add(combo, txt, static_cast<WORD>(ch));
    }
    for (int i = 1; i <= 12; ++i) {
        std::wstring txt = L"F" + std::to_wstring(i);
        hotkey_combo_add(combo, txt.c_str(), static_cast<WORD>(VK_F1 + i - 1));
    }
    hotkey_combo_add(combo, g_str == &kEn ? L"Space" : L"Пробел", VK_SPACE);
    hotkey_combo_add(combo, L"Home", VK_HOME);
    hotkey_combo_add(combo, L"End", VK_END);
    hotkey_combo_add(combo, L"Delete", VK_DELETE);
    hotkey_combo_add(combo, L"Esc", VK_ESCAPE);
    hotkey_combo_add(combo, g_str == &kEn ? L"Left" : L"Влево", VK_LEFT);
    hotkey_combo_add(combo, g_str == &kEn ? L"Right" : L"Вправо", VK_RIGHT);
    hotkey_combo_add(combo, g_str == &kEn ? L"Up" : L"Вверх", VK_UP);
    hotkey_combo_add(combo, g_str == &kEn ? L"Down" : L"Вниз", VK_DOWN);
    hotkey_combo_add(combo, L"+", VK_OEM_PLUS);
    hotkey_combo_add(combo, L"-", VK_OEM_MINUS);
    SendMessageW(combo, CB_SETCURSEL, 0, 0);
}

int combo_index_by_key(HWND combo, WORD key) {
    int count = static_cast<int>(SendMessageW(combo, CB_GETCOUNT, 0, 0));
    for (int i = 0; i < count; ++i)
        if (static_cast<WORD>(SendMessageW(combo, CB_GETITEMDATA, i, 0)) == key) return i;
    return 0;
}

int settings_selected_hotkey_command(HWND hwnd) {
    HWND list = GetDlgItem(hwnd, IDC_SET_HOTKEY_LIST);
    if (!list) return 0;
    int sel = static_cast<int>(SendMessageW(list, LB_GETCURSEL, 0, 0));
    if (sel == LB_ERR) return 0;
    return static_cast<int>(SendMessageW(list, LB_GETITEMDATA, sel, 0));
}

void load_selected_hotkey_controls(HWND hwnd) {
    int command = settings_selected_hotkey_command(hwnd);
    const HotkeyBinding* hk = find_hotkey_binding(command);
    BYTE fvirt = hk ? hk->fvirt : FVIRTKEY;
    WORD key = hk ? hk->key : 0;
    set_toggle_checked(GetDlgItem(hwnd, IDC_SET_HOTKEY_CTRL), (fvirt & FCONTROL) != 0);
    set_toggle_checked(GetDlgItem(hwnd, IDC_SET_HOTKEY_SHIFT), (fvirt & FSHIFT) != 0);
    set_toggle_checked(GetDlgItem(hwnd, IDC_SET_HOTKEY_ALT), (fvirt & FALT) != 0);
    HWND combo = GetDlgItem(hwnd, IDC_SET_HOTKEY_KEY);
    if (combo) SendMessageW(combo, CB_SETCURSEL, combo_index_by_key(combo, key), 0);
}

void populate_hotkey_list(HWND hwnd) {
    HWND list = GetDlgItem(hwnd, IDC_SET_HOTKEY_LIST);
    if (!list) return;
    int selected_command = settings_selected_hotkey_command(hwnd);
    SendMessageW(list, LB_RESETCONTENT, 0, 0);
    int selected_index = 0;
    const auto order = hotkey_command_order();
    for (std::size_t i = 0; i < order.size(); ++i) {
        std::wstring item = hotkey_list_item_text(order[i]);
        int idx = static_cast<int>(SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item.c_str())));
        SendMessageW(list, LB_SETITEMDATA, idx, order[i]);
        if (order[i] == selected_command) selected_index = idx;
    }
    SendMessageW(list, LB_SETCURSEL, selected_index, 0);
}

bool read_formula_edit(HWND edit, std::wstring& formula, std::vector<FormulaToken>& compiled, std::wstring& error) {
    if (!edit) {
        error = (g_str == &kEn) ? L"Coefficient field is unavailable." : L"Поле коэффициента недоступно.";
        return false;
    }
    wchar_t buf[512]{};
    GetWindowTextW(edit, buf, 512);
    formula = normalize_formula_text(buf);
    if (formula.empty()) formula = default_channel_formula_text();
    return compile_formula_rpn(formula, compiled, error, g_str == &kEn);
}

void assign_formula_to_channel(std::size_t channel_index, const std::wstring& formula, const std::vector<FormulaToken>& compiled) {
    ensure_channel_formula_vectors();
    if (channel_index >= g.channel_formulas.size() || channel_index >= g.channel_formula_rpn.size()) return;
    g.channel_formulas[channel_index] = formula;
    g.channel_formula_rpn[channel_index] = compiled;
    invalidate_formula_runtime_channel(channel_index);
    ensure_channel_formula_vectors();
}

void assign_global_formula(const std::wstring& formula, const std::vector<FormulaToken>& compiled) {
    g.global_formula = formula;
    g.global_formula_rpn = compiled;
    invalidate_formula_runtime();
    ensure_channel_formula_vectors();
}

int settings_selected_point_group(HWND hwnd) {
    HWND list = GetDlgItem(hwnd, IDC_SET_POINT_GROUP_LIST);
    if (!list) return -1;
    int sel = static_cast<int>(SendMessageW(list, LB_GETCURSEL, 0, 0));
    if (sel == LB_ERR) return -1;
    return static_cast<int>(SendMessageW(list, LB_GETITEMDATA, sel, 0));
}

void load_selected_point_group_controls(HWND hwnd) {
    const int index = settings_selected_point_group(hwnd);
    const bool valid = index >= 0 && index < static_cast<int>(g.point_groups.size());
    HWND visible = GetDlgItem(hwnd, IDC_SET_POINT_GROUP_VISIBLE);
    HWND recolor = GetDlgItem(hwnd, IDC_SET_POINT_GROUP_COLOR);
    if (visible) {
        set_toggle_checked(
            visible,
            valid && g.point_groups[static_cast<std::size_t>(index)].visible);
        EnableWindow(visible, valid);
    }
    if (recolor) EnableWindow(recolor, valid);
    HWND current_color = GetDlgItem(hwnd, IDC_SET_POINT_COLOR_CURRENT);
    if (current_color) SetWindowTextW(current_color, point_current_color_button_text());
    if (recolor) SetWindowTextW(recolor, point_selected_group_color_button_text());
    HWND create_new = GetDlgItem(hwnd, IDC_SET_POINT_GROUP_NEW);
    if (create_new) SetWindowTextW(create_new, point_group_new_button_text());
}

void populate_point_group_list(HWND hwnd) {
    HWND list = GetDlgItem(hwnd, IDC_SET_POINT_GROUP_LIST);
    if (!list) return;
    const int previous = settings_selected_point_group(hwnd);
    SendMessageW(list, LB_RESETCONTENT, 0, 0);
    normalize_active_point_group();
    int selected_index = LB_ERR;
    for (std::size_t i = 0; i < g.point_groups.size(); ++i) {
        std::wstring label = point_group_list_label(i, g.point_groups[i]);
        int idx = static_cast<int>(SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str())));
        SendMessageW(list, LB_SETITEMDATA, idx, static_cast<LPARAM>(i));
        if (static_cast<int>(i) == previous || static_cast<int>(i) == g.active_point_group) selected_index = idx;
    }
    if (selected_index != LB_ERR) SendMessageW(list, LB_SETCURSEL, selected_index, 0);
    load_selected_point_group_controls(hwnd);
    refresh_side_panel_controls();
}

void refresh_settings_controls() {
    if (!g.settings_wnd) return;
    CheckRadioButton(g.settings_wnd, IDC_SET_LANG_RU, IDC_SET_LANG_EN, g_str == &kEn ? IDC_SET_LANG_EN : IDC_SET_LANG_RU);
    sync_performance_mode_controls(
        g.settings_wnd,
        IDC_SET_PERF_LABEL,
        IDC_SET_PERF_FAST_OPEN,
        IDC_SET_PERF_FAST_WORK,
        IDC_SET_PERF_COMFORT);
    set_toggle_checked(GetDlgItem(g.settings_wnd, IDC_SET_GAP_MARKERS), g.show_gap_markers);
    if (HWND gap = GetDlgItem(g.settings_wnd, IDC_SET_GAP_MARKERS)) SetWindowTextW(gap, gap_markers_toggle_text());
    if (HWND xlbl = GetDlgItem(g.settings_wnd, IDC_SET_AXIS_X_LABEL_STATIC)) SetWindowTextW(xlbl, axis_x_label_text());
    if (HWND ylbl = GetDlgItem(g.settings_wnd, IDC_SET_AXIS_Y_LABEL_STATIC)) SetWindowTextW(ylbl, axis_y_label_text());
    if (HWND xedit = GetDlgItem(g.settings_wnd, IDC_SET_AXIS_X_LABEL_EDIT)) {
        g.updating_axis_label_edits = true;
        SetWindowTextW(xedit, g.axis_x_label.c_str());
        g.updating_axis_label_edits = false;
    }
    if (HWND yedit = GetDlgItem(g.settings_wnd, IDC_SET_AXIS_Y_LABEL_EDIT)) {
        g.updating_axis_label_edits = true;
        SetWindowTextW(yedit, g.axis_y_label.c_str());
        g.updating_axis_label_edits = false;
    }
    populate_hotkey_list(g.settings_wnd);
    load_selected_hotkey_controls(g.settings_wnd);
}

void rebuild_menu_bar() {
    if (!g.main) return;
    SetMenu(g.main, nullptr);
    if (g.menu) DestroyMenu(g.menu);
    g_menu_text_storage.clear();
    g.menu = make_menu();
    SetMenu(g.main, welcome_visible() ? nullptr : g.menu);
    MENUINFO mi{};
    mi.cbSize = sizeof(mi);
    mi.fMask = MIM_BACKGROUND | MIM_APPLYTOSUBMENUS;
    mi.hbrBack = CreateSolidBrush(g_theme->bg_toolbar);
    SetMenuInfo(g.menu, &mi);
    sync_menu();
    DrawMenuBar(g.main);
}

LRESULT CALLBACK SettingsProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CREATE: {
            HINSTANCE inst = reinterpret_cast<LPCREATESTRUCT>(lp)->hInstance;
            HFONT font = g.ui_font ? g.ui_font : reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
            auto is_settings_group = [](int id) {
                return id == IDC_SET_GROUP_GENERAL ||
                       id == IDC_SET_GROUP_HOTKEYS;
            };
            auto mk = [&](const wchar_t* cls, const wchar_t* text, DWORD style, int x, int y, int w, int h, int id) {
                HWND c = CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style, x, y, w, h, hwnd,
                    id ? reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)) : nullptr, inst, nullptr);
                if (c) SendMessageW(c, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
                if (c && is_settings_group(id)) EnableWindow(c, FALSE);
                return c;
            };
            auto mkcheck = [&](const wchar_t* text, int x, int y, int w, int h, int id) {
                return mk(L"BUTTON", text, BS_OWNERDRAW | WS_TABSTOP, x, y, w, h, id);
            };
            const bool en = (g_str == &kEn);
            mk(L"BUTTON", en ? L"General" : L"Общие", BS_OWNERDRAW, 12, 10, 510, 300, IDC_SET_GROUP_GENERAL);
            mk(L"BUTTON", g_str->lang_ru, BS_OWNERDRAW, 28, 36, 110, 22, IDC_SET_LANG_RU);
            mk(L"BUTTON", g_str->lang_en, BS_OWNERDRAW, 144, 36, 110, 22, IDC_SET_LANG_EN);
            mk(L"STATIC", performance_mode_label_text(), SS_LEFT, 28, 64, 278, 20, IDC_SET_PERF_LABEL);
            mkcheck(performance_fast_open_text(), 28, 88, 278, 28, IDC_SET_PERF_FAST_OPEN);
            mkcheck(performance_fast_work_text(), 28, 116, 278, 28, IDC_SET_PERF_FAST_WORK);
            mkcheck(performance_comfort_text(), 28, 144, 278, 28, IDC_SET_PERF_COMFORT);
            mkcheck(gap_markers_toggle_text(), 28, 182, 278, 28, IDC_SET_GAP_MARKERS);
            mk(L"STATIC", axis_x_label_text(), SS_LEFT, 28, 240, 72, 20, IDC_SET_AXIS_X_LABEL_STATIC);
            mk(L"EDIT", g.axis_x_label.c_str(), WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 104, 236, 260, 24, IDC_SET_AXIS_X_LABEL_EDIT);
            mk(L"STATIC", axis_y_label_text(), SS_LEFT, 28, 268, 72, 20, IDC_SET_AXIS_Y_LABEL_STATIC);
            mk(L"EDIT", g.axis_y_label.c_str(), WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 104, 264, 260, 24, IDC_SET_AXIS_Y_LABEL_EDIT);

            mk(L"BUTTON", en ? L"Hotkeys" : L"Горячие клавиши", BS_OWNERDRAW, 12, 310, 510, 188, IDC_SET_GROUP_HOTKEYS);
            mk(L"LISTBOX", L"", LBS_NOTIFY | WS_VSCROLL | WS_BORDER | LBS_NOINTEGRALHEIGHT | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS,
                24, 334, 240, 146, IDC_SET_HOTKEY_LIST);
            mk(L"BUTTON", L"Ctrl", BS_OWNERDRAW, 284, 342, 70, 22, IDC_SET_HOTKEY_CTRL);
            mk(L"BUTTON", L"Shift", BS_OWNERDRAW, 356, 342, 70, 22, IDC_SET_HOTKEY_SHIFT);
            mk(L"BUTTON", L"Alt", BS_OWNERDRAW, 428, 342, 70, 22, IDC_SET_HOTKEY_ALT);
            mk(L"STATIC", en ? L"Key:" : L"Клавиша:", SS_LEFT, 284, 374, 80, 20, 0);
            HWND combo = mk(L"COMBOBOX", L"", CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_BORDER, 284, 394, 214, 260, IDC_SET_HOTKEY_KEY);
            populate_hotkey_key_combo(combo);
            mk(L"BUTTON", en ? L"Apply" : L"Применить", BS_OWNERDRAW, 284, 432, 100, 28, IDC_SET_HOTKEY_APPLY);
            mk(L"BUTTON", en ? L"Reset" : L"Сбросить", BS_OWNERDRAW, 398, 432, 100, 28, IDC_SET_HOTKEY_RESET);
            mk(L"BUTTON", en ? L"Clear" : L"Очистить", BS_OWNERDRAW, 284, 466, 100, 28, IDC_SET_HOTKEY_CLEAR);
            populate_hotkey_list(hwnd);
            load_selected_hotkey_controls(hwnd);
            CheckRadioButton(hwnd, IDC_SET_LANG_RU, IDC_SET_LANG_EN, g_str == &kEn ? IDC_SET_LANG_EN : IDC_SET_LANG_RU);
            sync_performance_mode_controls(hwnd, IDC_SET_PERF_LABEL, IDC_SET_PERF_FAST_OPEN, IDC_SET_PERF_FAST_WORK, IDC_SET_PERF_COMFORT);
            set_toggle_checked(GetDlgItem(hwnd, IDC_SET_GAP_MARKERS), g.show_gap_markers);
            enable_file_drop_support(hwnd);
            return 0;
        }
        case WM_COMMAND: {
            const int id = LOWORD(wp);
            HWND ctl = reinterpret_cast<HWND>(lp);
            if (!ctl && id) ctl = GetDlgItem(hwnd, id);
            auto checked = [&]() { return is_toggle_checked(ctl); };
            switch (id) {
                case IDC_SET_LANG_RU:
                    if (HIWORD(wp) == BN_CLICKED && g_str != &kRu) { g_str = &kRu; save_runtime_settings(); rebuild_ui(); }
                    break;
                case IDC_SET_LANG_EN:
                    if (HIWORD(wp) == BN_CLICKED && g_str != &kEn) { g_str = &kEn; save_runtime_settings(); rebuild_ui(); }
                    break;
                case IDC_SET_PERF_FAST_OPEN:
                    if (HIWORD(wp) == BN_CLICKED || HIWORD(wp) == BN_DOUBLECLICKED) {
                        apply_performance_mode(PerformanceMode::FastOpen);
                    }
                    return 0;
                case IDC_SET_PERF_FAST_WORK:
                    if (HIWORD(wp) == BN_CLICKED || HIWORD(wp) == BN_DOUBLECLICKED) {
                        apply_performance_mode(PerformanceMode::FastWork);
                    }
                    return 0;
                case IDC_SET_PERF_COMFORT:
                    if (HIWORD(wp) == BN_CLICKED || HIWORD(wp) == BN_DOUBLECLICKED) {
                        apply_performance_mode(PerformanceMode::Comfort);
                    }
                    return 0;
                case IDC_SET_GAP_MARKERS:
                    if (HIWORD(wp) == BN_CLICKED || HIWORD(wp) == BN_DOUBLECLICKED) {
                        toggle_checked_state(ctl);
                        g.show_gap_markers = checked();
                        if (!g.show_gap_markers) hide_gap_details_card();
                        invalidate_plot_analysis_cache();
                        save_runtime_settings();
                        InvalidateRect(g.main, nullptr, FALSE);
                        refresh_settings_controls();
                    }
                    return 0;
                case IDC_SET_AXIS_X_LABEL_EDIT:
                case IDC_SET_AXIS_Y_LABEL_EDIT: {
                    if (HIWORD(wp) != EN_KILLFOCUS || g.updating_axis_label_edits) return 0;
                    wchar_t buf[128]{};
                    GetWindowTextW(ctl, buf, 128);
                    const bool is_x = (id == IDC_SET_AXIS_X_LABEL_EDIT);
                    std::wstring label = normalize_axis_label_text(buf, is_x ? L"X" : L"Y");
                    if (is_x) g.axis_x_label = label;
                    else g.axis_y_label = label;
                    g.updating_axis_label_edits = true;
                    SetWindowTextW(ctl, label.c_str());
                    g.updating_axis_label_edits = false;
                    save_runtime_settings();
                    InvalidateRect(g.main, nullptr, FALSE);
                    return 0;
                }
                case IDC_SET_HOTKEY_CTRL:
                case IDC_SET_HOTKEY_SHIFT:
                case IDC_SET_HOTKEY_ALT:
                    if (HIWORD(wp) == BN_CLICKED) toggle_checked_state(ctl);
                    return 0;
                case IDC_SET_HOTKEY_LIST:
                    if (HIWORD(wp) == LBN_SELCHANGE) load_selected_hotkey_controls(hwnd);
                    return 0;
                case IDC_SET_HOTKEY_APPLY:
                case IDC_SET_HOTKEY_RESET:
                case IDC_SET_HOTKEY_CLEAR: {
                    const int command = settings_selected_hotkey_command(hwnd);
                    if (!command) return 0;
                    BYTE fvirt = FVIRTKEY;
                    WORD key = 0;
                    if (id == IDC_SET_HOTKEY_RESET) {
                        HotkeyBinding def = default_hotkey_for_command(command);
                        fvirt = def.fvirt;
                        key = def.key;
                    } else if (id != IDC_SET_HOTKEY_CLEAR) {
                        if (is_toggle_checked(GetDlgItem(hwnd, IDC_SET_HOTKEY_CTRL))) fvirt |= FCONTROL;
                        if (is_toggle_checked(GetDlgItem(hwnd, IDC_SET_HOTKEY_SHIFT))) fvirt |= FSHIFT;
                        if (is_toggle_checked(GetDlgItem(hwnd, IDC_SET_HOTKEY_ALT))) fvirt |= FALT;
                        HWND combo = GetDlgItem(hwnd, IDC_SET_HOTKEY_KEY);
                        int sel = static_cast<int>(SendMessageW(combo, CB_GETCURSEL, 0, 0));
                        if (sel != CB_ERR) key = static_cast<WORD>(SendMessageW(combo, CB_GETITEMDATA, sel, 0));
                    }
                    int conflict = find_conflicting_hotkey(fvirt, key, command);
                    if (conflict) {
                        std::wstring msg = (g_str == &kEn ? L"Already used by: " : L"Уже используется действием: ");
                        msg += command_name(conflict);
                        MessageBoxW(hwnd, msg.c_str(), settings_window_title(), MB_OK | MB_ICONWARNING);
                        return 0;
                    }
                    set_hotkey_binding(command, fvirt, key);
                    rebuild_accelerators();
                    rebuild_menu_bar();
                    populate_hotkey_list(hwnd);
                    load_selected_hotkey_controls(hwnd);
                    save_runtime_settings();
                    set_status();
                    InvalidateRect(g.main, nullptr, TRUE);
                    return 0;
                }
                default: return 0;
            }
            InvalidateRect(g.main, nullptr, FALSE);
            return 0;
        }
        case WM_DROPFILES:
            if (g.main && IsWindow(g.main)) {
                SendMessageW(g.main, WM_DROPFILES, wp, lp);
                return 0;
            }
            return 0;
        case WM_ERASEBKGND: {
            HDC dc = reinterpret_cast<HDC>(wp);
            RECT rc;
            GetClientRect(hwnd, &rc);
            HBRUSH b = CreateSolidBrush(g_theme->bg_panel);
            FillRect(dc, &rc, b);
            DeleteObject(b);
            return 1;
        }
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* dis = reinterpret_cast<DRAWITEMSTRUCT*>(lp);
            if (dis && dis->CtlType == ODT_COMBOBOX && dis->CtlID == IDC_SET_HOTKEY_KEY) {
                draw_settings_combo_item(dis);
                return TRUE;
            }
            if (dis && dis->CtlType == ODT_LISTBOX && dis->CtlID == IDC_SET_HOTKEY_LIST) {
                draw_settings_list_item(dis);
                return TRUE;
            }
            if (!dis->hwndItem) break;
            const int ctl_id = GetDlgCtrlID(dis->hwndItem);
            if (ctl_id == IDC_SET_GROUP_GENERAL ||
                ctl_id == IDC_SET_GROUP_HOTKEYS) {
                wchar_t txt[128]{};
                GetWindowTextW(dis->hwndItem, txt, 128);
                draw_settings_group_box(dis->hDC, dis->rcItem, txt);
                return TRUE;
            }
            wchar_t txt[128];
            GetWindowTextW(dis->hwndItem, txt, 128);
            bool pressed = (dis->itemState & ODS_SELECTED) != 0;
            if (is_settings_toggle_button_id(ctl_id)) {
                if (is_settings_hotkey_modifier_id(ctl_id)) {
                    const bool enabled = IsWindowEnabled(dis->hwndItem) != FALSE;
                    draw_themed_check_control(dis->hDC, dis->rcItem, txt,
                        is_toggle_checked(dis->hwndItem), pressed, enabled, false, false);
                } else {
                    bool active = false;
                    if (ctl_id == IDC_SET_LANG_RU) active = g_str == &kRu;
                    else if (ctl_id == IDC_SET_LANG_EN) active = g_str == &kEn;
                    draw_themed_button(dis->hDC, dis->rcItem, txt, pressed, active, false);
                }
                return TRUE;
            }
            if (is_settings_checkbox_id(ctl_id)) {
                const bool enabled = IsWindowEnabled(dis->hwndItem) != FALSE;
                const bool radio = (ctl_id == IDC_SET_PERF_FAST_OPEN ||
                                    ctl_id == IDC_SET_PERF_FAST_WORK ||
                                    ctl_id == IDC_SET_PERF_COMFORT);
                draw_themed_check_control(dis->hDC, dis->rcItem, txt,
                    is_toggle_checked(dis->hwndItem), pressed, enabled, radio, false);
                return TRUE;
            }
            draw_themed_button(dis->hDC, dis->rcItem, txt, pressed, false, false);
            return TRUE;
        }
        case WM_MEASUREITEM: {
            MEASUREITEMSTRUCT* mis = reinterpret_cast<MEASUREITEMSTRUCT*>(lp);
            if (mis && mis->CtlType == ODT_COMBOBOX && mis->CtlID == IDC_SET_HOTKEY_KEY) {
                measure_settings_combo_item(mis);
                return TRUE;
            }
            if (mis && mis->CtlType == ODT_LISTBOX && mis->CtlID == IDC_SET_HOTKEY_LIST) {
                measure_settings_list_item(mis);
                return TRUE;
            }
            break;
        }
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN: {
            HDC dc = reinterpret_cast<HDC>(wp);
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, g_theme->text_primary);
            return reinterpret_cast<LRESULT>(g_panel_brush);
        }
        case WM_CTLCOLOREDIT: {
            HDC dc = reinterpret_cast<HDC>(wp);
            SetBkColor(dc, g_theme->bg_plot);
            SetTextColor(dc, g_theme->text_primary);
            return reinterpret_cast<LRESULT>(g_input_brush ? g_input_brush : g_panel_brush);
        }
        case WM_CTLCOLORLISTBOX: {
            HDC dc = reinterpret_cast<HDC>(wp);
            SetBkColor(dc, g_theme->bg_plot);
            SetTextColor(dc, g_theme->text_primary);
            return reinterpret_cast<LRESULT>(g_input_brush ? g_input_brush : g_panel_brush);
        }
        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);   // keep state; reopen instantly
            return 0;
        case WM_DESTROY:
            g.settings_wnd = nullptr;
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
