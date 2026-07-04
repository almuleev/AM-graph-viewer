// ---- welcome / start screen ----------------------------------------------

void rebuild_ui();

LRESULT CALLBACK WelcomeProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CREATE: {
            HINSTANCE inst = reinterpret_cast<LPCREATESTRUCT>(lp)->hInstance;
            HFONT font = g.ui_font ? g.ui_font : reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
            auto mkstatic = [&](const wchar_t* text, int id, HFONT use_font) {
                HWND ctl = CreateWindowExW(
                    0, L"STATIC", text,
                    WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SS_LEFT | SS_NOPREFIX,
                    0, 0, 10, 10, hwnd,
                    reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), inst, nullptr);
                SendMessageW(ctl, WM_SETFONT, reinterpret_cast<WPARAM>(use_font ? use_font : font), TRUE);
                return ctl;
            };
            auto mkbtn = [&](const wchar_t* text, int id) {
                HWND ctl = CreateWindowExW(
                    0, L"BUTTON", text,
                    WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | BS_OWNERDRAW,
                    0, 0, 10, 10, hwnd,
                    reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), inst, nullptr);
                SendMessageW(ctl, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
                return ctl;
            };

            mkstatic(g_str->welcome_title, IDW_TITLE, g.title_font ? g.title_font : font);
            mkstatic(g_str->welcome_subtitle, IDW_SUBTITLE, g.bold_font ? g.bold_font : font);
            mkstatic(welcome_version_text().c_str(), IDW_VERSION, g.axis_font ? g.axis_font : font);
            mkstatic(welcome_intro_text().c_str(), IDW_INTRO, font);
            mkstatic(welcome_features_text().c_str(), IDW_FEATURES, font);
            mkstatic(g_str->m_lang, IDW_LANG_LABEL, g.bold_font ? g.bold_font : font);
            mkbtn(g_str->lang_ru, IDM_LANG_RU);
            mkbtn(g_str->lang_en, IDM_LANG_EN);
            mkstatic((g_str == &kEn) ? L"Theme" : L"Тема", IDW_THEME_LABEL, g.bold_font ? g.bold_font : font);
            mkbtn(g_str->theme_light, IDW_THEME_LIGHT);
            mkbtn(g_str->theme_dark, IDW_THEME_DARK);
            HWND light_mode = CreateWindowExW(
                0, L"BUTTON", g_str->light_mode,
                WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP | BS_OWNERDRAW,
                0, 0, 10, 10, hwnd,
                reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDW_LIGHT_MODE)), inst, nullptr);
            SendMessageW(light_mode, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
            set_toggle_checked(light_mode, g.light_mode);
            mkstatic(welcome_actions_title_text(), IDW_ACTIONS_TITLE, g.bold_font ? g.bold_font : font);
            mkbtn(g_str->welcome_btn_recent, IDW_RECENT_FILES);
            mkbtn(settings_button_text(), IDC_PTSETTINGS);
            mkbtn(g_str->welcome_btn_hotkeys, IDM_HOTKEYS);
            mkbtn(g_str->welcome_btn_start, IDW_START);
            if (g_program_logo_icon) {
                SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(g_program_logo_icon));
                SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(g_program_logo_icon));
            }
            layout_welcome_controls(hwnd);
            enable_file_drop_support(hwnd);
            return 0;
        }
        case WM_SIZE:
            layout_welcome_controls(hwnd);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);
            HBRUSH bg = CreateSolidBrush(g_theme->bg_main);
            FillRect(hdc, &rc, bg);
            DeleteObject(bg);

            WelcomeLayout layout = compute_welcome_layout(hwnd);
            fill_rounded_rect(hdc, layout.hero, g_theme->bg_panel, g_theme->separator, 18);
            fill_rounded_rect(hdc, layout.action, g_theme->btn_bg, g_theme->separator, 18);

            RECT hero_accent = {
                layout.hero.left + 22,
                layout.hero.top + 18,
                layout.hero.left + min(140, rect_width(layout.hero) - 44),
                layout.hero.top + 24
            };
            HBRUSH accent_brush = CreateSolidBrush(g_theme->accent);
            FillRect(hdc, &hero_accent, accent_brush);
            DeleteObject(accent_brush);

            RECT action_header = {
                layout.action.left + 20,
                layout.action.top + 18,
                layout.action.right - 20,
                layout.action.top + 22
            };
            HBRUSH action_accent = CreateSolidBrush(g_theme->separator);
            FillRect(hdc, &action_header, action_accent);
            DeleteObject(action_accent);

            HFONT old_font = reinterpret_cast<HFONT>(SelectObject(
                hdc, g.axis_font ? g.axis_font : (g.ui_font ? g.ui_font : GetStockObject(DEFAULT_GUI_FONT))));
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, g_theme->text_secondary);
            RECT credit = {
                max(rc.left + 220, layout.bounds.right - 420),
                max(layout.bounds.bottom + 2, rc.bottom - 24),
                rc.right - 24,
                rc.bottom - 8
            };
            DrawTextW(
                hdc,
                welcome_author_credit_text(),
                -1,
                &credit,
                DT_RIGHT | DT_BOTTOM | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
            SelectObject(hdc, old_font);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_COMMAND:
            switch (LOWORD(wp)) {
                case IDC_OPEN: ShowWindow(hwnd, SW_HIDE); show_ui_controls(); SendMessageW(g.main, WM_COMMAND, IDC_OPEN, 0); return 0;
                case IDW_RECENT_FILES: show_recent_files_menu(hwnd); return 0;
                case IDC_PTSETTINGS: open_settings(); return 0;
                case IDM_HOTKEYS: show_hotkeys(); return 0;
                case IDW_START: ShowWindow(hwnd, SW_HIDE); show_ui_controls(); return 0;
                case IDW_LIGHT_MODE:
                    if (HIWORD(wp) == BN_CLICKED || HIWORD(wp) == BN_DOUBLECLICKED) {
                        HWND light = GetDlgItem(hwnd, IDW_LIGHT_MODE);
                        toggle_checked_state(light);
                        g.light_mode = is_toggle_checked(light);
                        invalidate_plot_analysis_cache();
                    }
                    save_runtime_settings();
                    if (g.settings_wnd) refresh_settings_controls();
                    return 0;
                case IDW_THEME_LIGHT: apply_theme_choice(&kLightTheme); return 0;
                case IDW_THEME_DARK: apply_theme_choice(&kDarkTheme); return 0;
                case IDM_LANG_RU: if (g_str != &kRu) { g_str = &kRu; save_runtime_settings(); rebuild_ui(); } return 0;
                case IDM_LANG_EN: if (g_str != &kEn) { g_str = &kEn; save_runtime_settings(); rebuild_ui(); } return 0;
            }
            return 0;
        case WM_DROPFILES:
            if (g.main && IsWindow(g.main)) {
                SendMessageW(g.main, WM_DROPFILES, wp, lp);
                return 0;
            }
            return 0;
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* dis = reinterpret_cast<DRAWITEMSTRUCT*>(lp);
            if (!dis->hwndItem) break;
            const HDC dc = dis->hDC;
            const RECT r = dis->rcItem;
            bool pressed = (dis->itemState & ODS_SELECTED) != 0;
            int ctl_id = GetDlgCtrlID(dis->hwndItem);
            bool is_lang_button = (ctl_id == IDM_LANG_RU || ctl_id == IDM_LANG_EN);
            bool is_theme_button = (ctl_id == IDW_THEME_LIGHT || ctl_id == IDW_THEME_DARK);
            bool active = (is_lang_button &&
                ((ctl_id == IDM_LANG_RU && g_str == &kRu) || (ctl_id == IDM_LANG_EN && g_str == &kEn))) ||
                (is_theme_button &&
                ((ctl_id == IDW_THEME_LIGHT && g_theme == &kLightTheme) || (ctl_id == IDW_THEME_DARK && g_theme == &kDarkTheme)));
            wchar_t txt[128]{};
            GetWindowTextW(dis->hwndItem, txt, 128);

            if (ctl_id == IDW_START) {
                draw_welcome_action_button(dc, r, txt, pressed, true, false);
                return TRUE;
            }
            if (ctl_id == IDW_RECENT_FILES) {
                draw_welcome_action_button(dc, r, txt, pressed, false, false);
                return TRUE;
            }
            if (ctl_id == IDC_OPEN) {
                draw_welcome_action_button(dc, r, txt, pressed, false, true);
                return TRUE;
            }
            if (ctl_id == IDC_PTSETTINGS || ctl_id == IDM_HOTKEYS) {
                draw_welcome_action_button(dc, r, txt, pressed, false, false);
                return TRUE;
            }
            if (ctl_id == IDW_LIGHT_MODE) {
                draw_themed_check_control(
                    dc, r, txt,
                    is_toggle_checked(dis->hwndItem), pressed,
                    IsWindowEnabled(dis->hwndItem) != FALSE,
                    false, false, g_theme->btn_bg);
                return TRUE;
            }
            draw_themed_button(dc, r, txt, pressed, active, false);
            return TRUE;
        }
        case WM_CTLCOLORSTATIC: {
            HDC dc = reinterpret_cast<HDC>(wp);
            SetBkMode(dc, TRANSPARENT);
            int ctl_id = GetDlgCtrlID(reinterpret_cast<HWND>(lp));
            if (ctl_id == IDW_SUBTITLE || ctl_id == IDW_VERSION) {
                SetTextColor(dc, g_theme->text_secondary);
            } else if (ctl_id == IDW_LANG_LABEL || ctl_id == IDW_THEME_LABEL || ctl_id == IDW_ACTIONS_TITLE) {
                SetTextColor(dc, g_theme->accent);
            } else {
                SetTextColor(dc, g_theme->text_primary);
            }
            if (ctl_id == IDW_TITLE || ctl_id == IDW_SUBTITLE || ctl_id == IDW_VERSION || ctl_id == IDW_INTRO || ctl_id == IDW_FEATURES) {
                return reinterpret_cast<LRESULT>(g_welcome_hero_brush ? g_welcome_hero_brush : g_welcome_brush);
            }
            return reinterpret_cast<LRESULT>(g_welcome_action_brush ? g_welcome_action_brush : g_welcome_brush);
        }
        case WM_CTLCOLORBTN: {
            HDC dc = reinterpret_cast<HDC>(wp);
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, g_theme->text_primary);
            return reinterpret_cast<LRESULT>(g_welcome_action_brush ? g_welcome_action_brush : g_welcome_brush);
        }
        case WM_DESTROY:
            g.welcome_wnd = nullptr;
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void show_welcome(HINSTANCE inst) {
    if (g.welcome_wnd) {
        hide_ui_controls();
        RECT rc;
        GetClientRect(g.main, &rc);
        SetWindowPos(g.welcome_wnd, HWND_TOP, 0, 0, rc.right, rc.bottom, SWP_SHOWWINDOW);
        RedrawWindow(g.welcome_wnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ALLCHILDREN);
        return;
    }
    RECT rc;
    GetClientRect(g.main, &rc);
    g.welcome_wnd = CreateWindowExW(
        0,
        L"LvmWelcome", L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        0, 0, rc.right, rc.bottom,
        g.main, nullptr, inst, nullptr);
    if (!g.welcome_wnd) return;
    hide_ui_controls();
    SetWindowPos(g.welcome_wnd, HWND_TOP, 0, 0, rc.right, rc.bottom, SWP_SHOWWINDOW);
    RedrawWindow(g.welcome_wnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ALLCHILDREN);
}

void raise_main_window() {
    if (!g.main || !IsWindow(g.main)) return;
    ShowWindow(g.main, SW_SHOW);
    SetWindowPos(g.main, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    SetForegroundWindow(g.main);
    SetActiveWindow(g.main);
}


