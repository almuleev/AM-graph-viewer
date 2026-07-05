// AM Graph Viewer - native Win32 GUI front end.
//
// Self-contained desktop viewer (no external GUI toolkit). Features:
//   - Main menu bar (File / View / Points / Lines / Markers / Help) + toolbar.
//   - Time plot and FFT (Hz) spectrum, with zoom/pan on both axes.
//   - Channel show/hide, colored legend, min/max envelope for dense data.
//   - Measure tool with a settings menu: choose which read-outs to show
//     (X, Y, Δx, Δy, 1/Δt, distance, point number) and optionally snap
//     markers to the nearest real data sample ("примагничивание").
//   - Reference guide lines: add vertical / horizontal lines on the plot.
//   - Two independent smoothing controls: a moving-average filter (changes
//     the rendered values) and a purely visual Catmull-Rom spline that
//     curves between samples without moving the underlying data points.
//   - Playback / pause that sweeps a playhead through the time signal.
//   - Export the visible segment to PNG (GDI+) or unified Save as (TXT/CSV/LVM).
//   - Keyboard shortcuts (see Справка → Горячие клавиши / F1).
//
// Build:
//   g++ -std=c++17 -O2 -municode -static -mwindows -o AMGraphViewer-v0.12.0-win-x64.exe
//       gui_main.cpp lvm_parser.cpp fft.cpp analysis.cpp
//       -lcomdlg32 -lgdi32 -luser32 -lgdiplus -lcomctl32
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#ifndef APP_VERSION_W
#define APP_VERSION_W L"v0.0.0"
#endif

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>

#include <algorithm>
#include <atomic>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwctype>
#include <fstream>
#include <limits>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using std::max;
using std::min;
#include <gdiplus.h>

#include "analysis.hpp"
#include "export_helpers.hpp"
#include "gap_details.hpp"
#include "formula_engine.hpp"
#include "lvm_parser.hpp"

namespace {

enum {
    IDC_OPEN = 1001,
    IDC_SAVEPNG,
    IDC_SAVECSV,
    IDC_MODE,
    IDC_PLAY,
    IDC_MEASURE,
    IDC_ZOOMIN,
    IDC_ZOOMOUT,
    IDC_RESET,
    IDC_PANLEFT,
    IDC_PANRIGHT,
    IDC_GOTO_START,
    IDC_GOTO_END,
    IDC_AUTOY,          // toolbar: auto-fit vertical scale (was lock_y)
    IDC_PTSETTINGS,     // toolbar: open the measurement-point settings panel
    IDC_SIDEPANEL,      // toolbar: show / hide the docked side panel
    IDC_SHOW_ALL,
    IDC_HIDE_ALL,
    IDC_EXPORT_SCOPE_CURRENT,
    IDC_EXPORT_SCOPE_FRAGMENT,
    IDC_EXPORT_SCOPE_RAW,
    IDC_EXPORT_APPLY_SETTINGS,
    IDC_EXPORT_APPLY_DATA,
    IDC_EXPORT_INCLUDE_CHANNEL_NAMES,
    IDC_EXPORT_INCLUDE_HIDDEN_CHANNELS,
    IDC_EXPORT_INCLUDE_POINTS,
    IDC_EXPORT_INCLUDE_MARKERS,
    IDC_EXPORT_INCLUDE_GUIDES,
    IDC_EXPORT_INCLUDE_FORMULAS,
    IDC_EXPORT_INCLUDE_FILTER,
    IDC_EXPORT_INCLUDE_GRAPH_SETTINGS,

    // Menu-only commands (no toolbar button).
    IDM_EXIT = 1100,
    IDM_VISMOOTH,       // visual (spline) smoothing toggle
    IDM_VPAN,           // vertical pan toggle
    IDM_SNAP,           // snap measurement markers to data
    IDM_ADD_VLINE,      // arm: place a vertical guide line
    IDM_ADD_HLINE,      // arm: place a horizontal guide line
    IDM_ADD_VLINE_EXACT, // add a vertical guide line by exact value
    IDM_ADD_HLINE_EXACT, // add a horizontal guide line by exact value
    IDM_ADD_MARKER,     // arm: place a marker
    IDM_CLEAR_LINES,
    IDM_CLEAR_MARKERS,
    IDM_CLEAR_POINTS,
    IDM_HOTKEYS,
    IDM_ABOUT,
    IDS_COLOR,          // settings panel: marker colour button
    IDW_START,          // welcome screen: start working

    // Measurement read-out toggles (used as control ids in the settings panel).
    IDM_PT_NUM = 1200,
    IDM_PT_X,
    IDM_PT_Y,
    IDM_PT_DX,
    IDM_PT_DY,
    IDM_PT_INVDT,
    IDM_PT_DIST,

    // Playback speed menu items.
    IDM_SPEED_00001 = 1300,
    IDM_SPEED_0001,
    IDM_SPEED_001,
    IDM_SPEED_01,
    IDM_SPEED_05,
    IDM_SPEED_1,
    IDM_SPEED_2,
    IDM_SPEED_5,
    IDM_SPEED_10,
    IDM_SPEED_CUSTOM,

    IDM_UNDO = 1400,
    IDM_REDO,
    IDM_THEME = 1500,
    IDM_LANG_RU = 1600,
    IDM_LANG_EN = 1601,
    IDM_MODE_TIME = 1700,
    IDM_MODE_FREQ,
    IDW_LIGHT_MODE = 1800,

    IDC_CHAN_BASE = 2000,
    IDC_CHAN_LABEL_BASE = 3000,
    IDC_CHAN_EDIT = 4000,

    IDC_SIDE_TAB_CHANNELS = 4200,
    IDC_SIDE_TAB_POINTS,
    IDC_SIDE_TAB_FILTER,
    IDC_SIDE_CHANNEL_COLOR,
    IDC_SIDE_CHANNEL_HINT,
    IDC_SIDE_GLOBAL_FORMULA_EDIT,
    IDC_SIDE_GLOBAL_FORMULA_APPLY,
    IDC_SIDE_FORMULA_EDIT,
    IDC_SIDE_FORMULA_APPLY_SELECTED,
    IDC_SIDE_FORMULA_APPLY_VISIBLE,
    IDC_SIDE_FORMULA_RESET_SELECTED,
    IDC_SIDE_FORMULA_RESET_ALL,
    IDC_SIDE_POINT_GROUP_LIST,
    IDC_SIDE_POINT_GROUP_VISIBLE,
    IDC_SIDE_POINT_GROUP_NEW,
    IDC_SIDE_POINT_GROUP_DELETE,
    IDC_SIDE_POINT_GROUP_NAME,
    IDC_SIDE_POINT_GROUP_RENAME,
    IDC_SIDE_POINT_COLOR_CURRENT,
    IDC_SIDE_POINT_GROUP_COLOR,
    IDC_SIDE_PT_NUM,
    IDC_SIDE_PT_X,
    IDC_SIDE_PT_Y,
    IDC_SIDE_PT_DX,
    IDC_SIDE_PT_DY,
    IDC_SIDE_PT_INVDT,
    IDC_SIDE_PT_DIST,
    IDC_SIDE_PT_SNAP,
    IDC_SIDE_FILTER_ENABLE,
    IDC_SIDE_FILTER_MODE_LABEL,
    IDC_SIDE_FILTER_MODE,
    IDC_SIDE_FILTER_TOPOLOGY_LABEL,
    IDC_SIDE_FILTER_TOPOLOGY,
    IDC_SIDE_FILTER_LOW_LABEL,
    IDC_SIDE_FILTER_LOW_VALUE,
    IDC_SIDE_FILTER_LOW_TRACK,
    IDC_SIDE_FILTER_HIGH_LABEL,
    IDC_SIDE_FILTER_HIGH_VALUE,
    IDC_SIDE_FILTER_HIGH_TRACK,

    IDC_SET_LANG_RU = 5000,
    IDC_SET_LANG_EN,
    IDC_SET_HOTKEY_LIST,
    IDC_SET_HOTKEY_CTRL,
    IDC_SET_HOTKEY_SHIFT,
    IDC_SET_HOTKEY_ALT,
    IDC_SET_HOTKEY_KEY,
    IDC_SET_HOTKEY_APPLY,
    IDC_SET_HOTKEY_RESET,
    IDC_SET_HOTKEY_CLEAR,
    IDC_SET_HOTKEY_RESET_ALL,

    IDC_SET_TRANSFORM_LIST = 5100,
    IDC_SET_GLOBAL_MUL,
    IDC_SET_GLOBAL_ADD,
    IDC_SET_CHANNEL_MUL,
    IDC_SET_CHANNEL_ADD,
    IDC_SET_TRANSFORM_APPLY,
    IDC_SET_TRANSFORM_RESET_CHANNEL,
    IDC_SET_TRANSFORM_RESET_ALL,
    IDC_SET_POINT_GROUP_LIST = 5120,
    IDC_SET_POINT_GROUP_VISIBLE,
    IDC_SET_POINT_COLOR_CURRENT,
    IDC_SET_POINT_GROUP_COLOR,
    IDC_SET_POINT_GROUP_NEW,
    IDC_SET_GAP_MARKERS,
    IDC_SET_AXIS_X_LABEL_STATIC,
    IDC_SET_AXIS_X_LABEL_EDIT,
    IDC_SET_AXIS_Y_LABEL_STATIC,
    IDC_SET_AXIS_Y_LABEL_EDIT,
    IDC_SET_GROUP_GENERAL = 5150,
    IDC_SET_GROUP_TRANSFORM,
    IDC_SET_GROUP_POINTS,
    IDC_SET_GROUP_HOTKEYS,

    IDW_TITLE = 5200,
    IDW_VERSION,
    IDW_ACTIONS_TITLE,
    IDW_ACTIONS_HINT,
    IDW_LANG_LABEL,
    IDW_THEME_LABEL,
    IDW_THEME_LIGHT,
    IDW_THEME_DARK,
    IDW_RECENT_FILES,
};

const int kTopBar = 72;        // two-row compact toolbar
const int kRightPanel = 312;
const int kBottomBar = 28;     // status-bar strip at the very bottom
const int kAxisBottom = 38;    // room under the plot for the X tick labels + title
const int kAxisLeft = 70;

const COLORREF kPalette[] = {
    RGB(31, 119, 180), RGB(255, 127, 14), RGB(44, 160, 44), RGB(214, 39, 40),
    RGB(148, 103, 189), RGB(140, 86, 75), RGB(227, 119, 194), RGB(127, 127, 127),
    RGB(188, 189, 34), RGB(23, 190, 207),
};
std::vector<COLORREF> g_channel_colors;
COLORREF channel_color(std::size_t i) {
    if (i < g_channel_colors.size()) return g_channel_colors[i];
    return kPalette[i % (sizeof(kPalette) / sizeof(kPalette[0]))];
}

struct Theme {
    COLORREF bg_main, bg_toolbar, bg_panel, bg_plot, bg_status;
    COLORREF grid, minor_grid, frame, axis_text;
    COLORREF text_primary, text_secondary;
    COLORREF accent, accent_hover, separator;
    COLORREF btn_bg, btn_border, btn_hover, btn_active;
    COLORREF btn_pressed; // "pressed" look for active toggle buttons
    COLORREF playhead;
    COLORREF marker_color;
};

static const Theme kLightTheme = {
    RGB(250, 252, 255),   // bg_main
    RGB(235, 240, 246),   // bg_toolbar
    RGB(242, 245, 250),   // bg_panel
    RGB(255, 255, 255),   // bg_plot
    RGB(245, 247, 250),   // bg_status
    RGB(230, 235, 240),   // grid
    RGB(240, 243, 246),   // minor_grid
    RGB(180, 190, 200),   // frame
    RGB(80, 90, 100),     // axis_text
    RGB(30, 40, 50),      // text_primary
    RGB(100, 110, 120),   // text_secondary
    RGB(0, 120, 212),     // accent
    RGB(0, 100, 180),     // accent_hover
    RGB(210, 218, 228),   // separator
    RGB(250, 251, 253),   // btn_bg
    RGB(200, 208, 218),   // btn_border
    RGB(230, 235, 242),   // btn_hover
    RGB(0, 120, 212),     // btn_active
    RGB(170, 175, 180),   // btn_pressed
    RGB(220, 40, 40),     // playhead
    RGB(200, 0, 0),       // marker_color
};

static const Theme kDarkTheme = {
    RGB(30, 32, 38),      // bg_main
    RGB(45, 48, 55),      // bg_toolbar
    RGB(38, 40, 48),      // bg_panel
    RGB(25, 28, 35),      // bg_plot
    RGB(35, 38, 45),      // bg_status
    RGB(55, 60, 70),      // grid
    RGB(40, 44, 52),      // minor_grid
    RGB(80, 85, 95),      // frame
    RGB(180, 185, 190),   // axis_text
    RGB(220, 225, 230),   // text_primary
    RGB(140, 145, 150),   // text_secondary
    RGB(0, 150, 255),     // accent
    RGB(30, 130, 235),    // accent_hover
    RGB(60, 65, 75),      // separator
    RGB(55, 58, 65),      // btn_bg
    RGB(75, 80, 90),      // btn_border
    RGB(70, 75, 85),      // btn_hover
    RGB(0, 150, 255),     // btn_active
    RGB(20, 25, 35),      // btn_pressed
    RGB(255, 60, 60),     // playhead
    RGB(255, 80, 80),     // marker_color
};

const Theme* g_theme = &kLightTheme;
HBRUSH g_panel_brush = nullptr;
HBRUSH g_input_brush = nullptr;
HBRUSH g_welcome_brush = nullptr;
HBRUSH g_welcome_hero_brush = nullptr;
HBRUSH g_welcome_action_brush = nullptr;
HICON g_program_logo_icon = nullptr;
std::wstring g_config_path;

struct OwnerDrawMenuEntry {
    std::wstring text;
    bool top_level = false;
    bool popup = false;
};

std::vector<std::unique_ptr<OwnerDrawMenuEntry>> g_menu_text_storage;

void update_theme_brushes() {
    if (g_panel_brush) DeleteObject(g_panel_brush);
    g_panel_brush = CreateSolidBrush(g_theme->bg_panel);
    if (g_input_brush) DeleteObject(g_input_brush);
    g_input_brush = CreateSolidBrush(g_theme->bg_plot);
    if (g_welcome_brush) DeleteObject(g_welcome_brush);
    g_welcome_brush = CreateSolidBrush(g_theme->bg_main);
    if (g_welcome_hero_brush) DeleteObject(g_welcome_hero_brush);
    g_welcome_hero_brush = CreateSolidBrush(g_theme->bg_panel);
    if (g_welcome_action_brush) DeleteObject(g_welcome_action_brush);
    g_welcome_action_brush = CreateSolidBrush(g_theme->btn_bg);
}

std::wstring app_directory_path() {
    wchar_t path[MAX_PATH]{};
    DWORD len = GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring full = (len > 0) ? std::wstring(path, path + len) : L"";
    std::size_t slash = full.find_last_of(L"\\/");
    if (slash != std::wstring::npos) full.resize(slash + 1);
    else full.clear();
    return full;
}

std::wstring app_config_path() {
    return app_directory_path() + L"AMGV.ini";
}

constexpr int IDI_APP_LOGO = 101;

void unload_program_logo() {
    if (g_program_logo_icon) {
        DestroyIcon(g_program_logo_icon);
        g_program_logo_icon = nullptr;
    }
}

bool load_program_logo() {
    unload_program_logo();
    HINSTANCE self = GetModuleHandleW(nullptr);
    HICON icon = reinterpret_cast<HICON>(
        LoadImageW(
            self,
            MAKEINTRESOURCEW(IDI_APP_LOGO),
            IMAGE_ICON,
            0,
            0,
            LR_DEFAULTSIZE | LR_DEFAULTCOLOR));
    if (!icon) {
        g_program_logo_icon = nullptr;
        return false;
    }
    g_program_logo_icon = icon;
    return true;
}

void load_app_settings() {
    if (g_config_path.empty()) g_config_path = app_config_path();
    wchar_t theme_buf[32]{};
    GetPrivateProfileStringW(L"ui", L"theme", L"light", theme_buf, 32, g_config_path.c_str());
    if (lstrcmpiW(theme_buf, L"dark") == 0) g_theme = &kDarkTheme;
    else g_theme = &kLightTheme;
}

void save_app_settings() {
    if (g_config_path.empty()) g_config_path = app_config_path();
    WritePrivateProfileStringW(
        L"ui",
        L"theme",
        (g_theme == &kDarkTheme) ? L"dark" : L"light",
        g_config_path.c_str());
}

const OwnerDrawMenuEntry* stash_menu_entry(const std::wstring& text, bool top_level, bool popup) {
    auto entry = std::make_unique<OwnerDrawMenuEntry>();
    entry->text = text;
    entry->top_level = top_level;
    entry->popup = popup;
    g_menu_text_storage.push_back(std::move(entry));
    return g_menu_text_storage.back().get();
}

const int IDC_SPEED_PROMPT_EDIT = 6200;
const int IDC_SPEED_PROMPT_OK = 6201;
const int IDC_SPEED_PROMPT_CANCEL = 6202;
const int IDC_RANGE_PROMPT_START_EDIT = 6203;
const int IDC_RANGE_PROMPT_END_EDIT = 6204;
const int IDC_RANGE_PROMPT_OK = 6205;
const int IDC_RANGE_PROMPT_CANCEL = 6206;
const int IDC_RANGE_PROMPT_AUTOFILL = 6210;
const int IDC_HOTKEYS_DIALOG_LIST = 6207;
const int IDC_HOTKEYS_DIALOG_CLOSE = 6208;
const int IDC_LOADING_CANCEL = 6209;

constexpr UINT WM_APP_ASYNC_SCAN_DONE = WM_APP + 1;
constexpr UINT WM_APP_ASYNC_LOAD_DONE = WM_APP + 2;
constexpr UINT_PTR kRuntimeSettingsSaveTimerId = 3;
constexpr UINT kRuntimeSettingsSaveDelayMs = 750;

// ---- string table --------------------------------------------------------
struct Strings {
    const wchar_t* app_title;
    const wchar_t* menu_file; const wchar_t* menu_view; const wchar_t* menu_meas; const wchar_t* menu_lines; const wchar_t* menu_markers; const wchar_t* menu_help;
    const wchar_t* m_open; const wchar_t* m_savepng; const wchar_t* m_savecsv; const wchar_t* m_undo; const wchar_t* m_redo; const wchar_t* m_exit;
    const wchar_t* m_timehz; const wchar_t* m_zoomin; const wchar_t* m_zoomout; const wchar_t* m_reset; const wchar_t* m_autoy; const wchar_t* m_smooth; const wchar_t* m_vpan; const wchar_t* m_play; const wchar_t* m_theme; const wchar_t* m_speed;
    const wchar_t* m_points; const wchar_t* m_ptsettings; const wchar_t* m_clearpts;
    const wchar_t* m_vline; const wchar_t* m_hline; const wchar_t* m_clearlines;
    const wchar_t* m_addmarker; const wchar_t* m_clearmarkers;
    const wchar_t* m_hotkeys; const wchar_t* m_about;
    const wchar_t* btn_open; const wchar_t* btn_png; const wchar_t* btn_csv; const wchar_t* btn_timehz; const wchar_t* btn_play; const wchar_t* btn_pause;
    const wchar_t* btn_measure; const wchar_t* btn_reset; const wchar_t* btn_autoy; const wchar_t* btn_settings;
    const wchar_t* panel_channels;
    const wchar_t* st_time; const wchar_t* st_hz; const wchar_t* st_channels; const wchar_t* st_points; const wchar_t* st_window; const wchar_t* st_yauto; const wchar_t* st_yfix; const wchar_t* st_lines; const wchar_t* st_markers; const wchar_t* st_speed;
    const wchar_t* plot_xlabel_time; const wchar_t* plot_xlabel_freq;
    const wchar_t* pt_num; const wchar_t* pt_x; const wchar_t* pt_y; const wchar_t* pt_dx; const wchar_t* pt_dy; const wchar_t* pt_invdt; const wchar_t* pt_dist;
    const wchar_t* fmt_pt_x; const wchar_t* fmt_pt_dx; const wchar_t* fmt_pt_dy; const wchar_t* fmt_pt_invdt; const wchar_t* fmt_pt_dist;
    const wchar_t* pt_snap;
    const wchar_t* dlg_ptsettings_title;
    const wchar_t* dlg_hotkeys_title; const wchar_t* dlg_about_title;
    const wchar_t* msg_nodata; const wchar_t* msg_openfirst; const wchar_t* msg_savepng_err; const wchar_t* msg_savecsv_err; const wchar_t* msg_read_err;
    const wchar_t* welcome_title;
    const wchar_t* welcome_btn_recent; const wchar_t* welcome_btn_settings; const wchar_t* welcome_btn_hotkeys; const wchar_t* welcome_btn_start;
    const wchar_t* hk_title;
    const wchar_t* about_body;
    const wchar_t* hover_open; const wchar_t* hover_png; const wchar_t* hover_csv; const wchar_t* hover_timehz; const wchar_t* hover_play; const wchar_t* hover_pause; const wchar_t* hover_measure; const wchar_t* hover_reset; const wchar_t* hover_autoy; const wchar_t* hover_settings;
    const wchar_t* lang_ru; const wchar_t* lang_en;
    const wchar_t* m_lang;
    const wchar_t* light_mode;
    const wchar_t* light_mode_status;
    const wchar_t* light_mode_range_title;
    const wchar_t* light_mode_range_start;
    const wchar_t* light_mode_range_end;
    const wchar_t* light_mode_range_apply;
    const wchar_t* light_mode_range_invalid_start;
    const wchar_t* light_mode_range_invalid_end;
    const wchar_t* msg_loading;
    const wchar_t* msg_loading_light;
    const wchar_t* msg_scanning_range;
    const wchar_t* msg_openprompt;
    const wchar_t* msg_delta_f;
    const wchar_t* msg_delta_t;
    const wchar_t* st_spline;
    const wchar_t* fmt_hz;
    const wchar_t* fmt_sec;
    const wchar_t* fmt_y;
    const wchar_t* unit_hz;
    const wchar_t* unit_sec;
    const wchar_t* theme_light;
    const wchar_t* theme_dark;
    const wchar_t* dlg_color;
    const wchar_t* msg_error_title;
    const wchar_t* msg_saved_png;
    const wchar_t* msg_saved_csv;
    const wchar_t* filter_open;
    const wchar_t* filter_png;
    const wchar_t* filter_csv;
    const wchar_t* csv_time;
    const wchar_t* csv_freq;
    const wchar_t* status_vline;
    const wchar_t* status_hline;
    const wchar_t* status_marker;
};

static const Strings kRu = {
    L"AM Graph Viewer",
    L"Файл", L"Вид", L"Точки", L"Линии", L"Маркеры", L"Справка",
    L"Открыть файл…\tCtrl+O", L"Сохранить PNG…\tCtrl+S", L"Сохранить как…\tCtrl+Shift+S", L"Отменить\tCtrl+Z", L"Повторить\tCtrl+Shift+Z", L"Выход\tAlt+F4",
    L"Время / Гц\tM", L"Увеличить\t+", L"Уменьшить\t−", L"Сбросить вид\tHome", L"Автомасштабирование", L"Сглаживание\tC", L"Вертикальное панорамирование\tP", L"Старт/стоп\tПробел", L"Тёмная тема\tT", L"Скорость воспроизведения…",
    L"Точки\tV", L"Настройки", L"Очистить\tDelete",
    L"Вертикальная\tL", L"Горизонтальная\tH", L"Очистить",
    L"Добавить\tK", L"Очистить",
    L"Горячие клавиши\tF1", L"О программе",
    L"Открыть", L"PNG", L"Сохранить как…", L"Время/Гц", L"Старт", L"Стоп", L"Точки", L"Сброс", L"АвтоМасштаб", L"Настройки",
    L"Каналы",
    L"Время", L"Гц (FFT)", L"Каналов", L"Точек", L"Окно", L"Y: авто", L"Y: фикс.", L"Линий", L"Маркеров", L"Скорость воспроизведения",
    L"Время, c", L"Частота, Гц",
    L"Показывать номер точки", L"Показывать координату X", L"Показывать координату Y", L"Расстояние между точками по X (Δx)", L"Расстояние между точками по Y (Δy)", L"Частота 1/Δt", L"Расстояние d (по прямой)",
    L"X=%.5g", L"Δx=%.5g", L"Δy=%.5g", L"1/Δt=%.5g Гц", L"d=%.5g",
    L"Примагничивать маркеры к графику",
    L"Настройки точек измерения",
    L"Горячие клавиши — AM Graph Viewer", L"О программе — AM Graph Viewer",
    L"Нет данных", L"Сначала откройте файл.", L"Не удалось сохранить PNG.", L"Не удалось выгрузить файл.", L"Ошибка чтения",
    L"AM Graph Viewer",
    L"Недавние файлы", L"Настройки точек…", L"Горячие клавиши", L"Начать работу",
    L"Файлы\n  O / Ctrl+O\t— Открыть\n  S / Ctrl+S\t— PNG\n  Ctrl+Shift+S\t— Сохранить как\n  Ctrl+Z\t— Отменить\n  Ctrl+Shift+Z\t— Повторить\n\nВид\n  M\t— Время/Гц\n  C\t— Сглаживание\n  + / ↑\t— Увеличить\n  − / ↓\t— Уменьшить\n  ← / →\t— Сдвиг влево/вправо\n  Home\t— Сброс\n  Ctrl+Home\t— В начало\n  Ctrl+End\t— В конец\n  Пробел\t— Старт/стоп\n\nЛинии и маркеры\n  L\t— Вертикальная линия\n  H\t— Горизонтальная линия\n  K\t— Маркер\n  Esc\t— Отменить добавление\n\nТочки\n  V\t— Режим точек вкл/выкл\n  Delete\t— Очистить точки\n\nМышь\n  Колесо\t— Масштаб под курсором\n  Shift+колесо\t— Прокрутка влево/вправо\n  Ctrl+колесо\t— Масштаб по высоте (Y)\n  Alt+колесо\t— Сдвиг вверх/вниз (Y)\n  ЛКМ + тяга\t— Панорамирование (вкл/выкл вертикальное через Вид)\n  ЛКМ\t— Поставить точку / линию / маркер (в режиме)\n  ПКМ\t— Очистить точки\n\n  F1\t— Эта справка",
    L"AM Graph Viewer — просмотрщик сигналов LabVIEW (.lvm / .txt / .csv)\n\nНативное приложение Win32 + GDI/GDI+, без внешних\nзависимостей и без Qt. Время и спектр (БПФ), измерения\nс примагничиванием, направляющие линии, визуальное\nсглаживание, экспорт PNG/CSV/TXT/LVM.\n\nСборка: build_gui.ps1 (MinGW g++) или make gui.",
    L"Открыть файл…", L"PNG", L"Сохранить как", L"Переключить Время / Гц", L"Старт", L"Стоп", L"Режим измерения точек", L"Сбросить вид", L"АвтоМасштаб", L"Настройки точек",
    L"Русский", L"English", L"Язык",
    L"Лёгкий режим",
    L"   |   Лёгкий режим: открыт только выбранный временной фрагмент",
    L"Лёгкий режим: диапазон открытия",
    L"С какой секунды открыть фрагмент:",
    L"По какую секунду открыть фрагмент:",
    L"Открыть фрагмент",
    L"Введите конечное число не меньше 0, например 0, 1.5 или 12.",
    L"Введите конечное число больше начального времени.",
    L"Загрузка файла...\r\nПожалуйста, подождите",
    L"Лёгкий режим: загрузка фрагмента...\r\nПожалуйста, подождите",
    L"Лёгкий режим: сканирование диапазона времени...\r\nПожалуйста, подождите",
    L"Откройте файл .lvm, .txt или .csv (кнопка «Открыть файл» / клавиша O)",
    L"   |   Δf = %.5g Гц,  Δamp = %.4g",
    L"   |   Δt = %.6g c,  Δy = %.5g,  1/Δt = %.6g Гц",
    L" (+сплайн)",
    L"%.5g Гц",
    L"%.5g c",
    L"y=%.5g",
    L"Гц",
    L"c",
    L"Светлая тема",
    L"Тёмная тема",
    L"Цвет маркеров…",
    L"Ошибка",
    L"Сохранено (PNG): ",
    L"Выгружено: ",
    L"LVM / текст / CSV файлы\0*.lvm;*.txt;*.csv\0Все файлы\0*.*\0",
    L"PNG изображение\0*.png\0Все файлы\0*.*\0",
    L"CSV файл\0*.csv\0Все файлы\0*.*\0",
    L"Время",
    L"Частота",
    L"Кликните на графике, чтобы поставить вертикальную линию (Esc — отмена). Можно добавить несколько линий подряд.",
    L"Кликните на графике, чтобы поставить горизонтальную линию (Esc — отмена). Можно добавить несколько линий подряд.",
    L"Кликните на графике, чтобы поставить маркер (Esc — отмена)."
};

static const Strings kEn = {
    L"AM Graph Viewer",
    L"File", L"View", L"Points", L"Lines", L"Markers", L"Help",
    L"Open file…\tCtrl+O", L"Save PNG…\tCtrl+S", L"Save as…\tCtrl+Shift+S", L"Undo\tCtrl+Z", L"Redo\tCtrl+Shift+Z", L"Exit\tAlt+F4",
    L"Time / Hz\tM", L"Zoom in\t+", L"Zoom out\t−", L"Reset view\tHome", L"Auto zoom", L"Smoothing\tC", L"Vertical pan\tP", L"Play / Pause\tSpace", L"Dark theme\tT", L"Playback speed…",
    L"Points\tV", L"Settings", L"Clear\tDelete",
    L"Vertical\tL", L"Horizontal\tH", L"Clear",
    L"Add\tK", L"Clear",
    L"Keyboard shortcuts\tF1", L"About",
    L"Open", L"PNG", L"Save as…", L"Time/Hz", L"▶ Play", L"⏸ Pause", L"Points", L"Reset", L"Auto zoom", L"Settings",
    L"Channels",
    L"Time", L"Hz (FFT)", L"Channels", L"Points", L"Window", L"Y: auto", L"Y: fixed", L"Lines", L"Markers", L"Playback speed",
    L"Time, s", L"Frequency, Hz",
    L"Show point number", L"Show X coordinate", L"Show Y coordinate", L"Distance along X (Δx)", L"Distance along Y (Δy)", L"Frequency 1/Δt", L"Straight-line distance d",
    L"X=%.5g", L"Δx=%.5g", L"Δy=%.5g", L"1/Δt=%.5g Hz", L"d=%.5g",
    L"Snap markers to graph",
    L"Measurement point settings",
    L"Keyboard shortcuts — AM Graph Viewer", L"About — AM Graph Viewer",
    L"No data", L"Open a file first.", L"Failed to save PNG.", L"Failed to export file.", L"Read error",
    L"AM Graph Viewer",
    L"Recent files", L"Point settings…", L"Keyboard shortcuts", L"Start working",
    L"Files\n  O / Ctrl+O\t— Open\n  S / Ctrl+S\t— PNG\n  Ctrl+Shift+S\t— Save as\n  Ctrl+Z\t— Undo\n  Ctrl+Shift+Z\t— Redo\n\nView\n  M\t— Time / Hz\n  C\t— Smoothing\n  + / ↑\t— Zoom in\n  − / ↓\t— Zoom out\n  ← / →\t— Pan left / right\n  Home\t— Reset view\n  Ctrl+Home\t— Go to start\n  Ctrl+End\t— Go to end\n  Space\t— Play / Pause\n\nLines and markers\n  L\t— Vertical line\n  H\t— Horizontal line\n  K\t— Marker\n  Esc\t— Cancel adding\n\nPoints\n  V\t— Measure mode on/off\n  Delete\t— Clear points\n\nMouse\n  Wheel\t— Zoom under cursor\n  Shift+wheel\t— Pan left / right\n  Ctrl+wheel\t— Zoom Y\n  Alt+wheel\t— Pan up/down (Y)\n  Left-drag\t— Pan (toggle vertical via View)\n  Left-click\t— Drop point / line / marker (in mode)\n  Right-click\t— Clear points\n\n  F1\t— This help",
    L"AM Graph Viewer — LabVIEW signal viewer (.lvm / .txt / .csv)\n\nNative Win32 + GDI/GDI+ application, no external\ndependencies, no Qt. Time and spectrum (FFT), measurements\nwith snapping, guide lines, visual smoothing, PNG and unified save-as export.\n\nBuild: build_gui.ps1 (MinGW g++) or make gui.",
    L"Open file…", L"PNG", L"Save as", L"Toggle Time / Hz", L"Playback", L"Pause", L"Measurement point mode", L"Reset view", L"Auto zoom", L"Point settings",
    L"Русский", L"English", L"Language",
    L"Light mode",
    L"   |   Light mode: only the selected time fragment is open",
    L"Light mode: open time range",
    L"Open the fragment starting from this second:",
    L"Open the fragment until this second:",
    L"Open fragment",
    L"Enter a finite number greater than or equal to 0, for example 0, 1.5, or 12.",
    L"Enter a finite number greater than the start time.",
    L"Loading file...\r\nPlease wait",
    L"Light mode: loading fragment...\r\nPlease wait",
    L"Light mode: scanning time range...\r\nPlease wait",
    L"Open a .lvm, .txt, or .csv file (click \"Open file\" / press O)",
    L"   |   Δf = %.5g Hz,  Δamp = %.4g",
    L"   |   Δt = %.6g s,  Δy = %.5g,  1/Δt = %.6g Hz",
    L" (+spline)",
    L"%.5g Hz",
    L"%.5g s",
    L"y=%.5g",
    L"Hz",
    L"s",
    L"Light theme",
    L"Dark theme",
    L"Marker colour…",
    L"Error",
    L"Saved (PNG): ",
    L"Exported: ",
    L"LVM / text / CSV files\0*.lvm;*.txt;*.csv\0All files\0*.*\0",
    L"PNG image\0*.png\0All files\0*.*\0",
    L"CSV file\0*.csv\0All files\0*.*\0",
    L"Time",
    L"Frequency",
    L"Click on the plot to place a vertical line (Esc to cancel). You can add multiple lines.",
    L"Click on the plot to place a horizontal line (Esc to cancel). You can add multiple lines.",
    L"Click on the plot to place a marker (Esc to cancel)."
};

const Strings* g_str = &kRu;

// Which read-outs to draw next to measurement markers (toggled from the
// "Measurements -> show at points" menu).
struct PointDisplay {
    bool number = true;   // #1, #2, �
    bool x = true;        // x coordinate
    bool y = true;        // y coordinate
    bool dx = true;       // Δx to the previous point
    bool dy = true;       // Δy to the previous point
    bool inv_dt = true;   // 1/Δt (Hz) in time mode only
    bool dist = false;    // Euclidean distance to the previous point
};

// A reference line the user dropped on the plot. `value` is in data units of
// the axis it pins (x for vertical, y for horizontal); `freq` records whether
// it belongs to the Hz view so it only shows in the matching mode.
struct GuideLine {
    bool vertical = true;
    double value = 0.0;
    bool freq = false;
};

struct PointGroup {
    std::wstring name;
    COLORREF color = RGB(0, 120, 215);
    bool visible = true;
    PointDisplay display;
    std::vector<std::pair<double, double>> points;
};

struct HotkeyBinding {
    int command = 0;
    BYTE fvirt = FVIRTKEY;
    WORD key = 0;
};

enum class AsyncLoadStage : unsigned char {
    None = 0,
    ScanningRange = 1,
    LoadingFile = 2
};

enum FilterMode {
    FilterModeLowPass = 0,
    FilterModeHighPass = 1,
    FilterModeBandPass = 2,
    FilterModeBandStop = 3,
};

enum FilterTopology {
    FilterTopologyButterworth = 0,
    FilterTopologyBessel = 1,
    FilterTopologyChebyshev = 2,
    FilterTopologyLinkwitzRiley = 3,
};


struct App {
    lvm::Dataset ds;
    std::vector<char> visible;
    std::vector<std::wstring> channel_labels;  // user-editable display names
    std::wstring global_formula = L"x";
    std::vector<FormulaToken> global_formula_rpn;
    bool formula_runtime_dirty = true;
    bool formula_ini_deferred = false;
    bool global_formula_identity = true;
    bool global_formula_affine = true;
    double global_formula_mul = 1.0;
    double global_formula_add = 0.0;
    std::vector<std::wstring> channel_formulas;
    std::vector<std::vector<FormulaToken>> channel_formula_rpn;
    std::vector<char> channel_formula_identity;
    std::vector<TransformRuntimeKind> channel_transform_kind;
    std::vector<double> channel_transform_mul;
    std::vector<double> channel_transform_add;
    std::vector<std::vector<double>> transformed_channel_cache;
    std::vector<char> transformed_channel_cache_valid;
    std::vector<std::vector<double>> filtered_channel_cache;
    std::vector<char> filtered_channel_cache_valid;
    bool has_non_identity_formula = false;
    bool has_non_affine_formula = false;
    bool freq_mode = false;

    double data_t0 = 0.0, data_t1 = 1.0;
    double win_start = 0.0, win_end = 1.0;
    double freq_start = 0.0, freq_end = 1.0;
    double approx_dt = 1.0;

    lvm::Spectrum spec;
    bool spec_valid = false;
    std::vector<int> spec_channel_indices;
    std::vector<char> spec_visible_state;
    double cached_global_gap_step = 0.0;
    bool cached_global_gap_step_ready = false;

    bool visual_smooth = false;  // Catmull-Rom spline rendering (data unchanged)

    bool auto_y = true;            // auto-fit vertical scale (true=auto, false=fixed)
    double y_lock_min = -1.0, y_lock_max = 1.0;

    bool auto_y_amp = true;        // auto-fit amplitude in Hz mode
    double y_amp_max = 1.0;        // locked amplitude max in Hz mode

    bool measure_mode = false;
    bool snap_to_data = true;       // snap markers to the nearest real sample
    PointDisplay pdisp;             // which read-outs to draw at markers
    std::wstring axis_x_label = L"X"; // graph label shown on the X axis corner
    std::wstring axis_y_label = L"Y"; // graph label shown on the Y axis corner
    COLORREF marker_color = g_theme->marker_color;
    std::vector<PointGroup> point_groups;
    int active_point_group = -1;

    std::vector<GuideLine> guides;  // vertical / horizontal reference lines
    int pending_line = 0;           // 0 none, 1 next click = vertical, 2 = horizontal
    std::vector<HotkeyBinding> hotkeys;

    bool playing = false;
    bool playhead_active = false;
    double playhead = 0.0;
    double play_anchor_data = 0.0;        // signal time when playback (re)started
    LARGE_INTEGER play_anchor_qpc = {};   // performance counter at that moment
    double play_speed = 1.0;

    // Mapping cache from the last paint (data <-> pixels) for hit-testing.
    double vx0 = 0, vx1 = 1, vy0 = 0, vy1 = 1;
    RECT vrect = {0, 0, 1, 1};
    bool vvalid = false;

    struct Marker {
        double x = 0.0;
        double y = 0.0;
        std::wstring label;
        bool freq = false;
        bool snapped = false;
        int channel = -1;
    };
    std::vector<Marker> markers;
    struct GapMarkerVisual {
        RECT rect = {0, 0, 0, 0};
        double duration = 0.0;
        long long estimated_missing_samples = 0;
    };
    struct TimeYRangeCache {
        bool valid = false;
        std::size_t lo = 0;
        std::size_t hi = 0;
        double win_start = 0.0;
        double win_end = 0.0;
        unsigned long long serial = 0;
        double ymin = -1.0;
        double ymax = 1.0;
    };
    std::vector<GapMarkerVisual> visible_gap_markers;
    TimeYRangeCache time_yrange_cache;
    unsigned long long plot_analysis_serial = 1;
    bool pending_marker = false;
    int active_marker = -1;
    bool light_mode = false;
    bool show_gap_markers = true;
    bool noise_threshold_enabled = false;
    double noise_threshold_min = -std::numeric_limits<double>::infinity();
    double noise_threshold_max = std::numeric_limits<double>::infinity();
    int noise_threshold_mode = FilterModeBandPass;
    int noise_threshold_topology = FilterTopologyButterworth;
    bool gap_details_visible = false;
    double gap_details_duration = 0.0;
    long long gap_details_missing_samples = 0;
    double gap_details_reference_step = 0.0;
    bool runtime_settings_save_pending = false;
    bool current_file_partial = false;
    double light_mode_open_start = 0.0;
    double light_mode_open_end = 10.0;
    AsyncLoadStage async_load_stage = AsyncLoadStage::None;
    unsigned long long async_load_token = 0;
    std::shared_ptr<std::atomic<bool>> async_load_cancel_flag;
    std::wstring cached_scan_path;
    double cached_scan_start = 0.0;
    double cached_scan_end = 0.0;
    bool cached_scan_valid = false;

    std::wstring file_name;
    std::vector<std::wstring> recent_files;
    std::string last_error;

    HWND main = nullptr;
    HWND open = nullptr, savepng = nullptr, savecsv = nullptr;
    HWND mode_time = nullptr, mode_freq = nullptr;
    HWND play = nullptr, measure = nullptr, marker_btn = nullptr;
    HWND vline_btn = nullptr, hline_btn = nullptr;
    HWND reset = nullptr, autoy = nullptr, ptsettings = nullptr, sidepanel_btn = nullptr;
    HWND show_all_btn = nullptr, hide_all_btn = nullptr;
    HWND status = nullptr;
    std::vector<HWND> checks;
    std::vector<HWND> check_labels;
    HWND channel_edit = nullptr;
    int editing_channel = -1;
    std::vector<HWND> buttons;   // owner-drawn toolbar buttons
    HWND hovered_btn = nullptr;
    std::wstring status_text;
    std::wstring status_detail_text;
    COLORREF status_detail_color = RGB(0, 0, 0);
    std::wstring hover_status_text;  // shown in status bar when hovering toolbar buttons
    std::vector<int> toolbar_seps;

    bool side_panel_visible = true;
    int side_panel_tab = 0; // 0 = channels, 1 = points, 2 = filter
    int side_selected_channel = -1;
    int side_scroll_y = 0;
    int side_scroll_max = 0;
    int side_content_height_channels = 0;
    int side_content_height_points = 0;
    int side_content_height_filter = 0;
    HWND side_tab_channels = nullptr;
    HWND side_tab_points = nullptr;
    HWND side_tab_filter = nullptr;
    HWND side_channel_hint = nullptr;
    HWND side_filter_enable = nullptr;
    HWND side_filter_mode_label = nullptr;
    HWND side_filter_mode = nullptr;
    HWND side_filter_topology_label = nullptr;
    HWND side_filter_topology = nullptr;
    HWND side_filter_low_label = nullptr;
    HWND side_filter_low_value = nullptr;
    HWND side_filter_low_track = nullptr;
    HWND side_filter_high_label = nullptr;
    HWND side_filter_high_value = nullptr;
    HWND side_filter_high_track = nullptr;
    HWND side_global_formula_label = nullptr;
    HWND side_global_formula_edit = nullptr;
    HWND side_global_formula_apply = nullptr;
    HWND side_channel_separator = nullptr;
    HWND side_channel_formula_label = nullptr;
    HWND side_formula_edit = nullptr;
    HWND side_channel_color = nullptr;
    HWND side_formula_apply_selected = nullptr;
    HWND side_formula_apply_visible = nullptr;
    HWND side_formula_reset_selected = nullptr;
    HWND side_formula_reset_all = nullptr;
    HWND side_point_group_list = nullptr;
    HWND side_point_group_visible = nullptr;
    HWND side_point_group_new = nullptr;
    HWND side_point_group_delete = nullptr;
    HWND side_point_group_name = nullptr;
    HWND side_point_group_rename = nullptr;
    HWND side_point_color_current = nullptr;
    HWND side_point_group_color = nullptr;
    HWND side_point_label_groups = nullptr;
    bool updating_axis_label_edits = false;
    bool updating_noise_threshold_edits = false;
    std::vector<HWND> side_channel_controls;
    std::vector<HWND> side_filter_controls;
    std::vector<HWND> side_point_controls;

    HWND settings_wnd = nullptr; // measurement-point settings panel (modeless)
    HWND welcome_wnd = nullptr;  // start screen

    HMENU menu = nullptr;        // main menu bar
    HACCEL accel = nullptr;      // current accelerator table (rebuilt from hotkeys)
    HFONT ui_font = nullptr;     // Segoe UI for controls / labels
    HFONT menu_font = nullptr;   // menu bar font sized for owner-drawn top menu
    HFONT bold_font = nullptr;   // semibold for headings
    HFONT title_font = nullptr;  // large font for the welcome title
    HFONT axis_font = nullptr;   // 11px for axis tick labels
    // icon_font removed � toolbar now uses text labels with ui_font

    bool dragging = false;
    int drag_x = 0, drag_y = 0;
    double drag_lo = 0.0, drag_hi = 0.0;
    double drag_y_lo = 0.0, drag_y_hi = 0.0;
    bool gap_click_pending = false;
    int gap_click_index = -1;

    bool fft_window_active = false;
    double fft_window_start = 0.0, fft_window_end = 0.0;
    bool fft_selecting = false;
    int fft_select_anchor_x = 0, fft_select_current_x = 0;
    double fft_select_anchor_t = 0.0, fft_select_current_t = 0.0;

    double spec_source_start = 0.0, spec_source_end = 0.0;
    bool spec_source_from_selection = false;
    bool spec_source_valid = false;

    bool vertical_pan = true;  // enable vertical panning with left-drag

    HDC backbuffer_dc = nullptr;
    HBITMAP backbuffer_bmp = nullptr;
    HBITMAP backbuffer_prev_bmp = nullptr;
    int backbuffer_w = 0;
    int backbuffer_h = 0;
};

App g;
ULONG_PTR g_gdiplus_token = 0;
std::size_t light_mode_render_stride(std::size_t sample_count, std::size_t target_samples) {
    if (!g.light_mode || target_samples == 0 || sample_count <= target_samples) return 1;
    return (sample_count + target_samples - 1) / target_samples;
}
void refresh_settings_controls();
void ensure_channel_formulas_loaded();
void invalidate_plot_analysis_cache();
void clear_spectrum_cache_state();
void recompute_transforms_from_state();
void load_side_transform_controls();
void compute_spectrum();
void normalize_filter_bounds();
void save_runtime_settings();
void add_recent_file(const std::wstring& path);
void show_recent_files_menu(HWND owner);
void write_ini_double(const wchar_t* section, const wchar_t* key, double value);
void write_ini_optional_double(const wchar_t* section, const wchar_t* key, double value);

constexpr std::size_t kMaxRecentFiles = 6;

std::wstring canonical_recent_file_path(const std::wstring& path) {
    wchar_t resolved[4096]{};
    if (path.empty()) return L"";
    constexpr DWORD resolved_count = static_cast<DWORD>(sizeof(resolved) / sizeof(resolved[0]));
    DWORD len = GetFullPathNameW(path.c_str(), resolved_count, resolved, nullptr);
    if (len > 0 && len < resolved_count) {
        return std::wstring(resolved, resolved + len);
    }
    return path;
}

void load_recent_files_from_ini() {
    if (g_config_path.empty()) g_config_path = app_config_path();
    g.recent_files.clear();
    for (std::size_t i = 0; i < kMaxRecentFiles; ++i) {
        wchar_t key[32]{};
        wchar_t value[4096]{};
        swprintf(key, 32, L"file_%u", static_cast<unsigned int>(i));
        constexpr DWORD value_count = static_cast<DWORD>(sizeof(value) / sizeof(value[0]));
        GetPrivateProfileStringW(L"recent", key, L"", value, value_count, g_config_path.c_str());
        if (value[0]) {
            std::wstring canonical = canonical_recent_file_path(value);
            if (canonical.empty()) continue;
            const bool seen = std::any_of(
                g.recent_files.begin(), g.recent_files.end(),
                [&](const std::wstring& item) {
                    return lstrcmpiW(item.c_str(), canonical.c_str()) == 0;
                });
            if (!seen) {
                g.recent_files.emplace_back(std::move(canonical));
            }
        }
    }
}

void save_recent_files_to_ini() {
    if (g_config_path.empty()) g_config_path = app_config_path();
    for (std::size_t i = 0; i < kMaxRecentFiles; ++i) {
        wchar_t key[32]{};
        swprintf(key, 32, L"file_%u", static_cast<unsigned int>(i));
        if (i < g.recent_files.size()) {
            WritePrivateProfileStringW(L"recent", key, g.recent_files[i].c_str(), g_config_path.c_str());
        } else {
            WritePrivateProfileStringW(L"recent", key, nullptr, g_config_path.c_str());
        }
    }
}

void add_recent_file(const std::wstring& path) {
    std::wstring canonical = canonical_recent_file_path(path);
    if (canonical.empty()) return;

    auto it = std::remove_if(g.recent_files.begin(), g.recent_files.end(),
                             [&](const std::wstring& item) {
                                 return lstrcmpiW(item.c_str(), canonical.c_str()) == 0;
                             });
    g.recent_files.erase(it, g.recent_files.end());
    g.recent_files.insert(g.recent_files.begin(), std::move(canonical));
    if (g.recent_files.size() > kMaxRecentFiles) {
        g.recent_files.resize(kMaxRecentFiles);
    }
    save_runtime_settings();
}

void save_runtime_settings_now() {
    if (g_config_path.empty()) g_config_path = app_config_path();

    WritePrivateProfileStringW(L"ui", L"language", (g_str == &kEn) ? L"en" : L"ru", g_config_path.c_str());
    WritePrivateProfileStringW(L"ui", L"smoothing", g.visual_smooth ? L"1" : L"0", g_config_path.c_str());
    WritePrivateProfileStringW(L"ui", L"vertical_pan", g.vertical_pan ? L"1" : L"0", g_config_path.c_str());
    WritePrivateProfileStringW(L"ui", L"snap_to_data", g.snap_to_data ? L"1" : L"0", g_config_path.c_str());
    WritePrivateProfileStringW(L"ui", L"light_mode", g.light_mode ? L"1" : L"0", g_config_path.c_str());
    WritePrivateProfileStringW(L"ui", L"show_gap_markers", g.show_gap_markers ? L"1" : L"0", g_config_path.c_str());
    normalize_filter_bounds();
    WritePrivateProfileStringW(L"ui", L"filter_enabled", g.noise_threshold_enabled ? L"1" : L"0", g_config_path.c_str());
    WritePrivateProfileStringW(L"ui", L"filter_mode", std::to_wstring(g.noise_threshold_mode).c_str(), g_config_path.c_str());
    WritePrivateProfileStringW(L"ui", L"filter_topology", std::to_wstring(g.noise_threshold_topology).c_str(), g_config_path.c_str());
    write_ini_optional_double(L"ui", L"filter_low_cutoff", g.noise_threshold_min);
    write_ini_optional_double(L"ui", L"filter_high_cutoff", g.noise_threshold_max);
    WritePrivateProfileStringW(L"ui", L"noise_threshold_enabled", nullptr, g_config_path.c_str());
    WritePrivateProfileStringW(L"ui", L"noise_threshold_min", nullptr, g_config_path.c_str());
    WritePrivateProfileStringW(L"ui", L"noise_threshold_max", nullptr, g_config_path.c_str());
    WritePrivateProfileStringW(L"ui", L"side_panel_visible", g.side_panel_visible ? L"1" : L"0", g_config_path.c_str());
    WritePrivateProfileStringW(L"ui", L"side_panel_tab", std::to_wstring(std::clamp(g.side_panel_tab, 0, 2)).c_str(), g_config_path.c_str());
    write_ini_double(L"ui", L"play_speed", g.play_speed);
    write_ini_double(L"ui", L"light_mode_open_start", g.light_mode_open_start);
    write_ini_double(L"ui", L"light_mode_open_end", g.light_mode_open_end);
    WritePrivateProfileStringW(L"ui", L"axis_x_label", g.axis_x_label.c_str(), g_config_path.c_str());
    WritePrivateProfileStringW(L"ui", L"axis_y_label", g.axis_y_label.c_str(), g_config_path.c_str());
    WritePrivateProfileStringW(L"points", L"x_label", nullptr, g_config_path.c_str());
    WritePrivateProfileStringW(L"points", L"y_label", nullptr, g_config_path.c_str());
    WritePrivateProfileStringW(L"points", L"number", g.pdisp.number ? L"1" : L"0", g_config_path.c_str());
    WritePrivateProfileStringW(L"points", L"x", g.pdisp.x ? L"1" : L"0", g_config_path.c_str());
    WritePrivateProfileStringW(L"points", L"y", g.pdisp.y ? L"1" : L"0", g_config_path.c_str());
    WritePrivateProfileStringW(L"points", L"dx", g.pdisp.dx ? L"1" : L"0", g_config_path.c_str());
    WritePrivateProfileStringW(L"points", L"dy", g.pdisp.dy ? L"1" : L"0", g_config_path.c_str());
    WritePrivateProfileStringW(L"points", L"inv_dt", g.pdisp.inv_dt ? L"1" : L"0", g_config_path.c_str());
    WritePrivateProfileStringW(L"points", L"dist", g.pdisp.dist ? L"1" : L"0", g_config_path.c_str());

    wchar_t color_buf[32]{};
    swprintf(color_buf, 32, L"%u", static_cast<unsigned int>(g.marker_color));
    WritePrivateProfileStringW(L"ui", L"marker_color", color_buf, g_config_path.c_str());
    if (!g.formula_ini_deferred) {
        WritePrivateProfileStringW(L"transform", L"global_formula", g.global_formula.c_str(), g_config_path.c_str());
        WritePrivateProfileStringW(L"transform", L"formula_count", nullptr, g_config_path.c_str());
        for (std::size_t i = 0; i < g.channel_formulas.size(); ++i) {
            wchar_t key_name[32]{};
            swprintf(key_name, 32, L"formula_%u", static_cast<unsigned int>(i));
            WritePrivateProfileStringW(L"transform", key_name, g.channel_formulas[i].c_str(), g_config_path.c_str());
        }
    }

    for (const auto& hk : g.hotkeys) {
        wchar_t key_name[32]{};
        wchar_t value[64]{};
        swprintf(key_name, 32, L"cmd_%d", hk.command);
        swprintf(value, 64, L"%u,%u", static_cast<unsigned int>(hk.fvirt), static_cast<unsigned int>(hk.key));
        WritePrivateProfileStringW(L"hotkeys", key_name, value, g_config_path.c_str());
    }
    save_recent_files_to_ini();
    save_app_settings();
}

void save_runtime_settings() {
    save_runtime_settings_now();
}

void flush_runtime_settings_save() {
    if (g.main && IsWindow(g.main)) {
        KillTimer(g.main, kRuntimeSettingsSaveTimerId);
    }
    if (!g.runtime_settings_save_pending) return;
    g.runtime_settings_save_pending = false;
    save_runtime_settings_now();
}

bool is_toggle_checked(HWND hwnd);
void set_toggle_checked(HWND hwnd, bool checked);

void apply_light_mode(bool enabled, bool persist = true) {
    const bool changed = g.light_mode != enabled;
    g.light_mode = enabled;
    if (!g.light_mode && g.formula_ini_deferred) {
        ensure_channel_formulas_loaded();
    }
    if (persist) save_runtime_settings();
    if (changed) {
        recompute_transforms_from_state();
        load_side_transform_controls();
        if (g.welcome_wnd && IsWindow(g.welcome_wnd)) {
            if (HWND light = GetDlgItem(g.welcome_wnd, IDW_LIGHT_MODE)) {
                set_toggle_checked(light, g.light_mode);
            }
            InvalidateRect(g.welcome_wnd, nullptr, FALSE);
        }
        if (g.settings_wnd) {
            if (HWND light = GetDlgItem(g.settings_wnd, IDW_LIGHT_MODE)) {
                set_toggle_checked(light, g.light_mode);
            }
            refresh_settings_controls();
        }
        if (g.main && IsWindow(g.main)) {
            InvalidateRect(g.main, nullptr, FALSE);
        }
    }
}

struct AsyncScanResult {
    unsigned long long token = 0;
    std::wstring path;
    bool ok = false;
    bool cancelled = false;
    double range_start = 0.0;
    double range_end = 0.0;
    std::string error;
};

struct AsyncLoadResult {
    unsigned long long token = 0;
    std::wstring path;
    lvm::Dataset ds;
    bool ok = false;
    bool cancelled = false;
    bool hide_channels = false;
    bool requested_time_window = false;
    double cached_global_gap_step = 0.0;
    bool cached_global_gap_step_ready = false;
    std::string error;
};

struct LegendItem { int channel; RECT rect; };
std::vector<LegendItem> g_legend_items;
RECT g_legend_box = {0,0,0,0};

#include "gui_state_history.cpp"
std::wstring to_w(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w(n > 0 ? n - 1 : 0, L'\0');
    if (n > 0) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], n);
    return w;
}

std::wstring to_w_acp(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w(n > 0 ? n - 1 : 0, L'\0');
    if (n > 0) MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, &w[0], n);
    return w;
}

std::string to_acp(const wchar_t* w) {
    int n = WideCharToMultiByte(CP_ACP, 0, w, -1, nullptr, 0, nullptr, nullptr);
    std::string s(n > 0 ? n - 1 : 0, '\0');
    if (n > 0) WideCharToMultiByte(CP_ACP, 0, w, -1, &s[0], n, nullptr, nullptr);
    return s;
}

std::string to_utf8(const std::wstring& w) {
    if (w.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string s(n > 0 ? n - 1 : 0, '\0');
    if (n > 0) WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &s[0], n, nullptr, nullptr);
    return s;
}

// Compact ASCII number for CSV; empty for NaN (matches the CLI / pandas).
std::string numfmt(double v) {
    if (std::isnan(v)) return "";
    char b[32];
    std::snprintf(b, sizeof(b), "%.15g", v);
    return b;
}

bool has_data() { return g.ds.ok && g.ds.rows() > 1 && g.ds.channel_count() > 0; }

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

bool has_fft_window() {
    return g.fft_window_active && g.fft_window_end > g.fft_window_start;
}

void clear_fft_window() {
    g.fft_window_active = false;
    g.fft_window_start = 0.0;
    g.fft_window_end = 0.0;
}

void clamp_time_window(double& start, double& end) {
    if (start > end) std::swap(start, end);
    start = std::max(start, g.data_t0);
    end = std::min(end, g.data_t1);
}

void set_fft_window(double start, double end) {
    clamp_time_window(start, end);
    if (end <= start) {
        clear_fft_window();
        return;
    }
    g.fft_window_active = true;
    g.fft_window_start = start;
    g.fft_window_end = end;
}

bool current_fft_source_window(double& start, double& end, bool& from_selection) {
    if (!has_data()) return false;
    if (has_fft_window()) {
        start = g.fft_window_start;
        end = g.fft_window_end;
        from_selection = true;
        return true;
    }
    start = g.win_start;
    end = g.win_end;
    clamp_time_window(start, end);
    from_selection = false;
    return end > start;
}

bool fft_window_contains_time(double t) {
    return has_fft_window() && t >= g.fft_window_start && t <= g.fft_window_end;
}

bool last_fft_source_window(double& start, double& end, bool& from_selection) {
    if (!g.spec_source_valid || g.spec_source_end <= g.spec_source_start) return false;
    start = g.spec_source_start;
    end = g.spec_source_end;
    from_selection = g.spec_source_from_selection;
    return true;
}

std::wstring fft_window_status(double start, double end, bool from_selection) {
    wchar_t buf[220];
    if (g_str == &kEn) {
        swprintf(buf, 220, from_selection
            ? L"   |   FFT window: selected %.6g..%.6g s (Shift-click or right-click to clear)"
            : L"   |   FFT window: visible %.6g..%.6g s",
            start, end);
    } else {
        swprintf(buf, 220, from_selection
            ? L"   |   FFT окно: выбранный участок %.6g..%.6g c (Shift-клик или ПКМ для сброса)"
            : L"   |   FFT окно: видимый участок %.6g..%.6g c",
            start, end);
    }
    return buf;
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
                 g_str->st_hz,
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
    }
    if (has_data()) {
        double fft_start, fft_end;
        bool from_selection = false;
        if (g.freq_mode) {
            if (last_fft_source_window(fft_start, fft_end, from_selection)) {
                s += fft_window_status(fft_start, fft_end, from_selection);
            }
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

void ensure_channel_formula_vectors();
double transform_channel_value(std::size_t ci, double raw);
int channel_index_by_name(const std::string& name);

void clear_spectrum_cache_state() {
    g.spec_valid = false;
    g.spec = lvm::Spectrum{};
    g.spec_channel_indices.clear();
    g.spec_visible_state.clear();
}

void refresh_spec_channel_indices() {
    g.spec_channel_indices.assign(g.spec.names.size(), -1);
    for (std::size_t i = 0; i < g.spec.names.size(); ++i) {
        g.spec_channel_indices[i] = channel_index_by_name(g.spec.names[i]);
    }
}

bool spectrum_visibility_changed() {
    if (!g.light_mode) return false;
    return g.spec_visible_state.size() != g.visible.size() || g.spec_visible_state != g.visible;
}

bool build_time_window_dataset(const lvm::Dataset& in, double start, double end, lvm::Dataset& out,
                               const std::vector<std::size_t>* selected_channels = nullptr) {
    out = lvm::Dataset{};
    out.stats = in.stats;
    out.ok = true;
    ensure_channel_formula_vectors();
    if (in.time.empty()) return true;

    std::vector<std::size_t> channel_indices;
    if (selected_channels && !selected_channels->empty()) {
        channel_indices = *selected_channels;
    } else {
        channel_indices.resize(in.channels.size());
        for (std::size_t i = 0; i < in.channels.size(); ++i) channel_indices[i] = i;
    }
    out.names.reserve(channel_indices.size());
    out.channels.resize(channel_indices.size());
    for (std::size_t channel_index : channel_indices) {
        if (channel_index < in.names.size()) out.names.push_back(in.names[channel_index]);
        else out.names.push_back("Channel_" + std::to_string(channel_index + 1));
    }

    const std::size_t lo = static_cast<std::size_t>(
        std::lower_bound(in.time.begin(), in.time.end(), start) - in.time.begin());
    const std::size_t hi = static_cast<std::size_t>(
        std::upper_bound(in.time.begin(), in.time.end(), end) - in.time.begin());
    if (lo >= hi) return true;

    out.time.assign(in.time.begin() + static_cast<std::ptrdiff_t>(lo),
                    in.time.begin() + static_cast<std::ptrdiff_t>(hi));
    for (std::size_t out_index = 0; out_index < channel_indices.size(); ++out_index) {
        const std::size_t c = channel_indices[out_index];
        auto& dst = out.channels[out_index];
        dst.reserve(hi - lo);
        const TransformRuntimeKind kind = (c < g.channel_transform_kind.size())
            ? g.channel_transform_kind[c]
            : TransformRuntimeKind::Identity;
        if (kind == TransformRuntimeKind::Identity) {
            const std::size_t base = dst.size();
            dst.insert(dst.end(),
                       in.channels[c].begin() + static_cast<std::ptrdiff_t>(lo),
                       in.channels[c].begin() + static_cast<std::ptrdiff_t>(hi));
            if (g.noise_threshold_enabled) {
                for (std::size_t i = base; i < dst.size(); ++i) {
                    dst[i] = rendered_channel_sample(c, lo + (i - base));
                }
            }
            continue;
        }
        if (kind == TransformRuntimeKind::Affine) {
            for (std::size_t r = lo; r < hi; ++r) {
                dst.push_back(rendered_channel_sample(c, r));
            }
            continue;
        }
        ensure_transformed_channel_cache(c);
        const auto& cache = g.transformed_channel_cache[c];
        const std::size_t base = dst.size();
        dst.insert(dst.end(),
                   cache.begin() + static_cast<std::ptrdiff_t>(lo),
                   cache.begin() + static_cast<std::ptrdiff_t>(hi));
        if (g.noise_threshold_enabled) {
            for (std::size_t i = base; i < dst.size(); ++i) {
                dst[i] = rendered_channel_sample(c, lo + (i - base));
            }
        }
    }
    return true;
}

void compute_spectrum_for_window(double start, double end, bool from_selection) {
    if (!has_data()) return;
    clamp_time_window(start, end);
    g.spec_source_start = start;
    g.spec_source_end = end;
    g.spec_source_from_selection = from_selection;
    g.spec_source_valid = end > start;
    bool has_visible_channel = false;
    for (char visible : g.visible) {
        if (visible) {
            has_visible_channel = true;
            break;
        }
    }
    if (!has_visible_channel) {
        clear_spectrum_cache_state();
        g.spec_source_valid = end > start;
        g.spec_visible_state = g.visible;
        return;
    }
    std::vector<std::size_t> visible_channels;
    if (g.light_mode) {
        visible_channels.reserve(g.visible.size());
        for (std::size_t i = 0; i < g.visible.size(); ++i) {
            if (g.visible[i]) visible_channels.push_back(i);
        }
    } else {
        visible_channels.resize(g.ds.channel_count());
        for (std::size_t i = 0; i < visible_channels.size(); ++i) visible_channels[i] = i;
    }
    if (g.light_mode && visible_channels.empty()) {
        clear_spectrum_cache_state();
        g.spec_source_valid = end > start;
        g.spec_visible_state = g.visible;
        return;
    }
    lvm::Dataset view;
    build_time_window_dataset(g.ds, start, end, view, &visible_channels);
    g.spec = lvm::compute_spectrum(view, 16384);
    g.spec_valid = g.spec.ok;
    refresh_spec_channel_indices();
    g.spec_visible_state = g.visible;
}

void compute_spectrum_from_current_source() {
    if (!has_data()) return;
    double start = 0.0, end = 0.0;
    bool from_selection = false;
    if (current_fft_source_window(start, end, from_selection)) {
        compute_spectrum_for_window(start, end, from_selection);
    }
}

void compute_spectrum() {
    if (!has_data()) return;
    double start = 0.0, end = 0.0;
    bool from_selection = false;
    if (g.freq_mode && last_fft_source_window(start, end, from_selection)) {
        compute_spectrum_for_window(start, end, from_selection);
        return;
    }
    compute_spectrum_from_current_source();
}

bool ensure_current_spectrum() {
    if (!has_data()) return false;
    if (!g.spec_valid || spectrum_visibility_changed()) compute_spectrum();
    return g.spec_valid;
}

int channel_index_by_name(const std::string& name) {
    for (std::size_t i = 0; i < g.ds.names.size(); ++i)
        if (g.ds.names[i] == name) return static_cast<int>(i);
    return -1;
}

void finish_channel_rename(bool apply) {
    if (g.editing_channel < 0 || g.editing_channel >= static_cast<int>(g.channel_labels.size())) return;
    const int ci = g.editing_channel;
    SettingsSnapshot before;
    bool track_change = false;
    if (apply && g.channel_edit) {
        before = capture_settings_snapshot();
        track_change = true;
    }
    if (apply && g.channel_edit) {
        wchar_t buf[256];
        GetWindowTextW(g.channel_edit, buf, 256);
        g.channel_labels[ci] = buf;
        if (ci < static_cast<int>(g.check_labels.size()) && g.check_labels[ci]) {
            const std::wstring label = channel_display_label(static_cast<std::size_t>(ci));
            SetWindowTextW(g.check_labels[ci], label.c_str());
        }
        InvalidateRect(g.main, nullptr, TRUE);
    }
    if (g.channel_edit) {
        DestroyWindow(g.channel_edit);
        g.channel_edit = nullptr;
    }
    g_channel_edit_proc = nullptr;
    g.editing_channel = -1;
    if (track_change) record_settings_change(before);
    if (g.settings_wnd) refresh_settings_controls();
    set_status();
    InvalidateRect(g.main, nullptr, FALSE);
}

LRESULT CALLBACK ChannelEditProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_GETDLGCODE:
            return CallWindowProcW(g_channel_edit_proc, hwnd, msg, wp, lp) | DLGC_WANTALLKEYS;
        case WM_KEYDOWN:
            if (wp == VK_RETURN) {
                finish_channel_rename(true);
                SetFocus(g.main);
                return 0;
            }
            if (wp == VK_ESCAPE) {
                finish_channel_rename(false);
                SetFocus(g.main);
                return 0;
            }
            break;
        case WM_KILLFOCUS:
            finish_channel_rename(true);
            return 0;
    }
    return CallWindowProcW(g_channel_edit_proc, hwnd, msg, wp, lp);
}

void finish_channel_rename_if_click_outside(HWND hwnd) {
    if (!g.channel_edit) return;
    RECT edit_rect;
    GetWindowRect(g.channel_edit, &edit_rect);
    MapWindowPoints(nullptr, hwnd, reinterpret_cast<LPPOINT>(&edit_rect), 2);
    POINT click_point;
    GetCursorPos(&click_point);
    ScreenToClient(hwnd, &click_point);
    if (!PtInRect(&edit_rect, click_point)) {
        finish_channel_rename(true);
        SetFocus(hwnd);
    }
}

void start_channel_rename(int ci) {
    if (ci < 0 || ci >= static_cast<int>(g.channel_labels.size())) return;
    finish_channel_rename(true);
    if (ci >= static_cast<int>(g.check_labels.size()) || !g.check_labels[ci]) return;

    RECT r;
    GetWindowRect(g.check_labels[ci], &r);
    MapWindowPoints(nullptr, g.main, reinterpret_cast<LPPOINT>(&r), 2);
    HINSTANCE inst = reinterpret_cast<HINSTANCE>(GetWindowLongPtr(g.main, GWLP_HINSTANCE));
    HFONT font = g.ui_font ? g.ui_font : reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    g.channel_edit = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", channel_display_label(static_cast<std::size_t>(ci)).c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        r.left - 2, r.top - 1, (r.right - r.left) + 4, (r.bottom - r.top) + 2,
        g.main, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_CHAN_EDIT)), inst, nullptr);
    if (!g.channel_edit) return;
    SendMessageW(g.channel_edit, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    SendMessageW(g.channel_edit, EM_SETSEL, 0, -1);
    g_channel_edit_proc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrW(g.channel_edit, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(ChannelEditProc)));
    g.editing_channel = ci;
    SetFocus(g.channel_edit);
}

void destroy_checks() {
    finish_channel_rename(true);
    for (HWND h : g.checks) DestroyWindow(h);
    g.checks.clear();
    for (HWND h : g.check_labels) DestroyWindow(h);
    g.check_labels.clear();
}

void rebuild_checks() {
    destroy_checks();
    if (!has_data()) return;
    if (g.side_selected_channel < 0 || g.side_selected_channel >= static_cast<int>(g.ds.channel_count())) g.side_selected_channel = 0;
    HFONT font = g.ui_font ? g.ui_font : reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    HINSTANCE inst = reinterpret_cast<HINSTANCE>(GetWindowLongPtr(g.main, GWLP_HINSTANCE));
    for (std::size_t i = 0; i < g.ds.channel_count(); ++i) {
        HWND c = CreateWindowExW(
            0, L"BUTTON", L"",
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | BS_OWNERDRAW, 0, 0, 10, 10, g.main,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_CHAN_BASE + i)), inst, nullptr);
        SendMessageW(c, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        set_toggle_checked(c, g.visible[i] != 0);
        g.checks.push_back(c);

        HWND lbl = CreateWindowExW(
            0, L"STATIC", channel_display_label(i).c_str(),
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SS_LEFT | SS_NOTIFY | SS_CENTERIMAGE,
            0, 0, 10, 10, g.main,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_CHAN_LABEL_BASE + i)), inst, nullptr);
        SendMessageW(lbl, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        g.check_labels.push_back(lbl);
    }
}

void hide_ui_controls() {
    if (g.main) {
        SetMenu(g.main, nullptr);
        DrawMenuBar(g.main);
    }
    for (HWND b : g.buttons) ShowWindow(b, SW_HIDE);
    for (HWND c : g.checks) ShowWindow(c, SW_HIDE);
    for (HWND c : g.check_labels) ShowWindow(c, SW_HIDE);
    for (HWND c : g.side_channel_controls) ShowWindow(c, SW_HIDE);
    for (HWND c : g.side_filter_controls) ShowWindow(c, SW_HIDE);
    for (HWND c : g.side_point_controls) ShowWindow(c, SW_HIDE);
    if (g.channel_edit) ShowWindow(g.channel_edit, SW_HIDE);
    if (g.status) ShowWindow(g.status, SW_HIDE);
}

void show_ui_controls() {
    if (g.main) {
        SetMenu(g.main, g.menu);
        DrawMenuBar(g.main);
    }
    for (HWND b : g.buttons) ShowWindow(b, SW_SHOW);
    apply_side_panel_visibility();
    if (g.channel_edit && g.side_panel_visible && g.side_panel_tab == 0) ShowWindow(g.channel_edit, SW_SHOW);
    if (g.status) ShowWindow(g.status, SW_SHOW);
}

bool welcome_visible() {
    return g.welcome_wnd && IsWindow(g.welcome_wnd) && IsWindowVisible(g.welcome_wnd);
}

int side_panel_width() {
    return (!welcome_visible() && g.side_panel_visible) ? kRightPanel : 0;
}

int side_selected_point_group() {
    if (!g.side_point_group_list) return -1;
    int sel = static_cast<int>(SendMessageW(g.side_point_group_list, LB_GETCURSEL, 0, 0));
    if (sel == LB_ERR) return -1;
    return static_cast<int>(SendMessageW(g.side_point_group_list, LB_GETITEMDATA, sel, 0));
}

void populate_filter_mode_combo(HWND combo) {
    if (!combo) return;
    SendMessageW(combo, CB_RESETCONTENT, 0, 0);
    auto add = [&](const wchar_t* text, int value) {
        int idx = static_cast<int>(SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text)));
        SendMessageW(combo, CB_SETITEMDATA, idx, static_cast<LPARAM>(value));
    };
    add(filter_mode_lowpass_text(), FilterModeLowPass);
    add(filter_mode_highpass_text(), FilterModeHighPass);
    add(filter_mode_bandpass_text(), FilterModeBandPass);
    add(filter_mode_bandstop_text(), FilterModeBandStop);
    SendMessageW(combo, CB_SETMINVISIBLE, 4, 0);
    SendMessageW(combo, CB_SETDROPPEDWIDTH, 220, 0);
}

void populate_filter_topology_combo(HWND combo) {
    if (!combo) return;
    SendMessageW(combo, CB_RESETCONTENT, 0, 0);
    auto add = [&](const wchar_t* text, int value) {
        int idx = static_cast<int>(SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text)));
        SendMessageW(combo, CB_SETITEMDATA, idx, static_cast<LPARAM>(value));
    };
    add(filter_topology_butterworth_text(), FilterTopologyButterworth);
    add(filter_topology_bessel_text(), FilterTopologyBessel);
    add(filter_topology_chebyshev_text(), FilterTopologyChebyshev);
    add(filter_topology_linkwitz_text(), FilterTopologyLinkwitzRiley);
    SendMessageW(combo, CB_SETMINVISIBLE, 4, 0);
    SendMessageW(combo, CB_SETDROPPEDWIDTH, 220, 0);
}

void expand_combo_dropdown(HWND combo) {
    if (!combo) return;
    COMBOBOXINFO cbi{};
    cbi.cbSize = sizeof(cbi);
    if (!GetComboBoxInfo(combo, &cbi) || !cbi.hwndList) return;

    RECT list_rc{};
    if (!GetWindowRect(cbi.hwndList, &list_rc)) return;

    int item_h = static_cast<int>(SendMessageW(combo, CB_GETITEMHEIGHT, 0, 0));
    if (item_h <= 0) item_h = 20;
    const int item_count = max(1, static_cast<int>(SendMessageW(combo, CB_GETCOUNT, 0, 0)));
    const int visible_items = min(item_count, 4);
    const int desired_h = max(item_h * visible_items + 4, item_h + 4);
    const int desired_w = max(static_cast<int>(list_rc.right - list_rc.left), 220);

    SetWindowPos(cbi.hwndList, HWND_TOP,
                 list_rc.left, list_rc.top,
                 desired_w, desired_h,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

void sync_filter_controls_from_state() {
    const double nyquist = current_filter_nyquist();
    const bool has_signal = has_data() && nyquist > 0.0;
    const bool filter_configurable = has_signal;

    if (g.side_filter_enable) {
        SetWindowTextW(g.side_filter_enable, filter_toggle_text());
        set_toggle_checked(g.side_filter_enable, g.noise_threshold_enabled);
        EnableWindow(g.side_filter_enable, has_signal);
    }
    if (g.side_filter_mode_label) SetWindowTextW(g.side_filter_mode_label, filter_mode_label_text());
    if (g.side_filter_topology_label) SetWindowTextW(g.side_filter_topology_label, filter_topology_label_text());
    if (g.side_filter_low_label) SetWindowTextW(g.side_filter_low_label, filter_low_cutoff_text());
    if (g.side_filter_high_label) SetWindowTextW(g.side_filter_high_label, filter_high_cutoff_text());

    if (g.side_filter_mode) {
        populate_filter_mode_combo(g.side_filter_mode);
        SendMessageW(g.side_filter_mode, CB_SETCURSEL,
            std::clamp(g.noise_threshold_mode,
                       static_cast<int>(FilterModeLowPass),
                       static_cast<int>(FilterModeBandStop)), 0);
        EnableWindow(g.side_filter_mode, filter_configurable);
    }
    if (g.side_filter_topology) {
        populate_filter_topology_combo(g.side_filter_topology);
        SendMessageW(g.side_filter_topology, CB_SETCURSEL,
            std::clamp(g.noise_threshold_topology,
                       static_cast<int>(FilterTopologyButterworth),
                       static_cast<int>(FilterTopologyLinkwitzRiley)), 0);
        EnableWindow(g.side_filter_topology, filter_configurable);
    }

    g.updating_noise_threshold_edits = true;
    if (g.side_filter_low_value) SetWindowTextW(g.side_filter_low_value, filter_frequency_text(g.noise_threshold_min).c_str());
    if (g.side_filter_high_value) SetWindowTextW(g.side_filter_high_value, filter_frequency_text(g.noise_threshold_max).c_str());
    if (g.side_filter_low_track) {
        SendMessageW(g.side_filter_low_track, TBM_SETRANGE, TRUE, MAKELONG(0, 1000));
        SendMessageW(g.side_filter_low_track, TBM_SETPOS, TRUE, frequency_to_filter_slider(g.noise_threshold_min, nyquist));
        EnableWindow(g.side_filter_low_track, filter_configurable);
    }
    if (g.side_filter_high_track) {
        SendMessageW(g.side_filter_high_track, TBM_SETRANGE, TRUE, MAKELONG(0, 1000));
        SendMessageW(g.side_filter_high_track, TBM_SETPOS, TRUE, frequency_to_filter_slider(g.noise_threshold_max, nyquist));
        EnableWindow(g.side_filter_high_track, filter_configurable);
    }
    g.updating_noise_threshold_edits = false;
}

void load_side_transform_controls() {
    const bool formulas_ready = !g.formula_ini_deferred;
    const std::wstring default_formula = default_channel_formula_text();
    if (g.side_global_formula_edit) {
        SetWindowTextW(g.side_global_formula_edit,
            formulas_ready ? g.global_formula.c_str() : default_formula.c_str());
        EnableWindow(g.side_global_formula_edit, has_data());
    }
    if (g.side_global_formula_apply) EnableWindow(g.side_global_formula_apply, has_data());
    const int ci = g.side_selected_channel;
    const bool valid = ci >= 0 && ci < static_cast<int>(g.ds.channel_count());
    if (g.side_formula_edit) {
        const wchar_t* text = default_formula.c_str();
        if (valid && static_cast<std::size_t>(ci) < g.channel_formulas.size()) {
            text = formulas_ready
                ? g.channel_formulas[static_cast<std::size_t>(ci)].c_str()
                : default_formula.c_str();
        }
        SetWindowTextW(g.side_formula_edit, text);
        EnableWindow(g.side_formula_edit, has_data());
    }
    if (g.side_channel_color) EnableWindow(g.side_channel_color, valid);
    if (g.side_formula_apply_selected) EnableWindow(g.side_formula_apply_selected, valid);
    if (g.side_formula_apply_visible) EnableWindow(g.side_formula_apply_visible, has_data());
    if (g.side_formula_reset_selected) EnableWindow(g.side_formula_reset_selected, valid);
    if (g.side_formula_reset_all) EnableWindow(g.side_formula_reset_all, has_data());
    sync_filter_controls_from_state();
}

void load_side_point_group_controls() {
    const int index = side_selected_point_group();
    const bool valid = index >= 0 && index < static_cast<int>(g.point_groups.size());
    if (valid) {
        g.active_point_group = index;
        sync_point_display_from_active_group();
    }
    if (g.side_point_group_visible) {
        set_toggle_checked(
            g.side_point_group_visible,
            valid && g.point_groups[static_cast<std::size_t>(index)].visible);
        EnableWindow(g.side_point_group_visible, valid);
    }
    if (g.side_point_group_color) EnableWindow(g.side_point_group_color, valid);
    if (g.side_point_group_delete) EnableWindow(g.side_point_group_delete, valid);
    if (g.side_point_group_name) {
        SetWindowTextW(g.side_point_group_name,
            valid ? g.point_groups[static_cast<std::size_t>(index)].name.c_str() : L"");
        EnableWindow(g.side_point_group_name, valid);
    }
    if (g.side_point_group_rename) EnableWindow(g.side_point_group_rename, valid);
    if (HWND num = GetDlgItem(g.main, IDC_SIDE_PT_NUM)) EnableWindow(num, valid);
    if (HWND x = GetDlgItem(g.main, IDC_SIDE_PT_X)) EnableWindow(x, valid);
    if (HWND y = GetDlgItem(g.main, IDC_SIDE_PT_Y)) EnableWindow(y, valid);
    if (HWND dx = GetDlgItem(g.main, IDC_SIDE_PT_DX)) EnableWindow(dx, valid);
    if (HWND dy = GetDlgItem(g.main, IDC_SIDE_PT_DY)) EnableWindow(dy, valid);
    if (HWND invdt = GetDlgItem(g.main, IDC_SIDE_PT_INVDT)) EnableWindow(invdt, valid);
    if (HWND dist = GetDlgItem(g.main, IDC_SIDE_PT_DIST)) EnableWindow(dist, valid);
    if (HWND snap = GetDlgItem(g.main, IDC_SIDE_PT_SNAP)) EnableWindow(snap, TRUE);
}

void populate_side_point_group_list() {
    if (!g.side_point_group_list) return;
    const int previous = side_selected_point_group();
    SendMessageW(g.side_point_group_list, WM_SETREDRAW, FALSE, 0);
    SendMessageW(g.side_point_group_list, LB_RESETCONTENT, 0, 0);
    normalize_active_point_group();
    int selected_index = LB_ERR;
    if (g.point_groups.empty()) {
        const int idx = static_cast<int>(SendMessageW(
            g.side_point_group_list, LB_ADDSTRING, 0,
            reinterpret_cast<LPARAM>(point_group_empty_text())));
        if (idx != LB_ERR) {
            SendMessageW(g.side_point_group_list, LB_SETITEMDATA, idx, static_cast<LPARAM>(-1));
            selected_index = idx;
            g.side_scroll_y = 0;
        }
    } else {
        for (std::size_t i = 0; i < g.point_groups.size(); ++i) {
            std::wstring label = point_group_list_label(i, g.point_groups[i]);
            int idx = static_cast<int>(SendMessageW(g.side_point_group_list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str())));
            SendMessageW(g.side_point_group_list, LB_SETITEMDATA, idx, static_cast<LPARAM>(i));
            if (static_cast<int>(i) == previous || static_cast<int>(i) == g.active_point_group) selected_index = idx;
        }
    }
    if (selected_index != LB_ERR) {
        SendMessageW(g.side_point_group_list, LB_SETCURSEL, selected_index, 0);
        SendMessageW(g.side_point_group_list, LB_SETTOPINDEX, selected_index, 0);
    }
    SendMessageW(g.side_point_group_list, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(g.side_point_group_list, nullptr, TRUE);
    load_side_point_group_controls();
}

bool side_panel_hit_test(const POINT& pt) {
    if (!g.side_panel_visible || welcome_visible()) return false;
    RECT rc{};
    GetClientRect(g.main, &rc);
    const int panel_left = rc.right - side_panel_width();
    return pt.x >= panel_left && pt.x <= rc.right && pt.y >= kTopBar && pt.y <= rc.bottom - kBottomBar;
}

void update_side_panel_scrollbar(int viewport_top, int content_height) {
    g.side_content_height_channels = max(g.side_content_height_channels, 0);
    g.side_content_height_points = max(g.side_content_height_points, 0);
    g.side_content_height_filter = max(g.side_content_height_filter, 0);
    RECT rc{};
    GetClientRect(g.main, &rc);
    const int viewport_bottom = rc.bottom - kBottomBar - 6;
    const int viewport_height = max(0, viewport_bottom - viewport_top);
    g.side_scroll_max = max(0, content_height - viewport_height);
    if (g.side_scroll_y > g.side_scroll_max) g.side_scroll_y = g.side_scroll_max;
    if (g.side_scroll_y < 0) g.side_scroll_y = 0;
}

void scroll_side_panel(int delta) {
    if (delta == 0 || g.side_scroll_max <= 0) return;
    const int next = std::clamp(g.side_scroll_y + delta, 0, g.side_scroll_max);
    if (next == g.side_scroll_y) return;
    g.side_scroll_y = next;
    layout();
    RedrawWindow(g.main, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

void set_side_panel_tab(int tab) {
    g.side_panel_tab = (tab >= 0 && tab <= 2) ? tab : 0;
    const bool show_channels = g.side_panel_visible && g.side_panel_tab == 0 && !welcome_visible();
    const bool show_points = g.side_panel_visible && g.side_panel_tab == 1 && !welcome_visible();
    const bool show_filter = g.side_panel_visible && g.side_panel_tab == 2 && !welcome_visible();
    if (g.show_all_btn) ShowWindow(g.show_all_btn, show_channels ? SW_SHOW : SW_HIDE);
    if (g.hide_all_btn) ShowWindow(g.hide_all_btn, show_channels ? SW_SHOW : SW_HIDE);
    for (HWND h : g.side_channel_controls) if (h) ShowWindow(h, show_channels ? SW_SHOW : SW_HIDE);
    for (HWND h : g.side_filter_controls) if (h) ShowWindow(h, show_filter ? SW_SHOW : SW_HIDE);
    for (HWND h : g.side_point_controls) if (h) ShowWindow(h, show_points ? SW_SHOW : SW_HIDE);
    for (HWND h : g.checks) if (h) ShowWindow(h, show_channels ? SW_SHOW : SW_HIDE);
    for (HWND h : g.check_labels) if (h) ShowWindow(h, show_channels ? SW_SHOW : SW_HIDE);
    if (!show_channels && g.channel_edit) ShowWindow(g.channel_edit, SW_HIDE);
    if (g.side_tab_channels) InvalidateRect(g.side_tab_channels, nullptr, FALSE);
    if (g.side_tab_points) InvalidateRect(g.side_tab_points, nullptr, FALSE);
    if (g.side_tab_filter) InvalidateRect(g.side_tab_filter, nullptr, FALSE);
    if (show_channels) {
        load_side_transform_controls();
    } else if (show_points) {
        populate_side_point_group_list();
    } else if (show_filter) {
        sync_filter_controls_from_state();
    }
}

void apply_side_panel_visibility() {
    const bool show = g.side_panel_visible && !welcome_visible();
    if (g.side_tab_channels) ShowWindow(g.side_tab_channels, show ? SW_SHOW : SW_HIDE);
    if (g.side_tab_points) ShowWindow(g.side_tab_points, show ? SW_SHOW : SW_HIDE);
    if (g.side_tab_filter) ShowWindow(g.side_tab_filter, show ? SW_SHOW : SW_HIDE);
    set_side_panel_tab(g.side_panel_tab);
}

void refresh_side_panel_controls() {
    sync_point_display_from_active_group();
    if (g.sidepanel_btn) SetWindowTextW(g.sidepanel_btn, side_panel_button_text());
    if (g.side_tab_channels) SetWindowTextW(g.side_tab_channels, side_tab_channels_text());
    if (g.side_tab_points) SetWindowTextW(g.side_tab_points, side_tab_points_text());
    if (g.side_tab_filter) SetWindowTextW(g.side_tab_filter, side_tab_filter_text());
    if (g.side_channel_hint) SetWindowTextW(g.side_channel_hint, side_channel_hint_text());
    if (g.side_filter_enable) SetWindowTextW(g.side_filter_enable, filter_toggle_text());
    if (g.side_filter_mode_label) SetWindowTextW(g.side_filter_mode_label, filter_mode_label_text());
    if (g.side_filter_topology_label) SetWindowTextW(g.side_filter_topology_label, filter_topology_label_text());
    if (g.side_filter_low_label) SetWindowTextW(g.side_filter_low_label, filter_low_cutoff_text());
    if (g.side_filter_high_label) SetWindowTextW(g.side_filter_high_label, filter_high_cutoff_text());
    if (g.side_global_formula_label) SetWindowTextW(g.side_global_formula_label, side_global_formula_label_text());
    if (g.side_global_formula_apply) SetWindowTextW(g.side_global_formula_apply, side_global_formula_apply_text());
    if (g.side_channel_separator) SetWindowTextW(g.side_channel_separator, L"");
    if (g.side_channel_formula_label) SetWindowTextW(g.side_channel_formula_label, side_channel_formula_label_text());
    if (g.side_channel_color) SetWindowTextW(g.side_channel_color, side_channel_color_button_text());
    if (g.side_point_group_visible) SetWindowTextW(g.side_point_group_visible, point_group_visible_text());
    if (g.side_point_group_new) SetWindowTextW(g.side_point_group_new, point_group_new_button_text());
    if (g.side_point_group_delete) SetWindowTextW(g.side_point_group_delete, side_point_group_delete_text());
    if (g.side_point_group_rename) SetWindowTextW(g.side_point_group_rename, side_point_group_rename_text());
    if (g.side_point_color_current) SetWindowTextW(g.side_point_color_current, point_current_color_button_text());
    if (g.side_point_group_color) SetWindowTextW(g.side_point_group_color, point_selected_group_color_button_text());
    if (g.side_point_label_groups) SetWindowTextW(g.side_point_label_groups, point_group_list_title());
    if (g.side_formula_apply_selected) SetWindowTextW(g.side_formula_apply_selected, side_formula_apply_selected_text());
    if (g.side_formula_apply_visible) SetWindowTextW(g.side_formula_apply_visible, side_formula_apply_visible_text());
    if (g.side_formula_reset_selected) SetWindowTextW(g.side_formula_reset_selected, side_formula_reset_selected_text());
    if (g.side_formula_reset_all) SetWindowTextW(g.side_formula_reset_all, side_formula_reset_all_text());

    const struct ToggleMap { int id; bool value; const wchar_t* text; } point_toggles[] = {
        {IDC_SIDE_PT_NUM, g.pdisp.number, side_pt_num_text()},
        {IDC_SIDE_PT_X, g.pdisp.x, side_pt_x_text()},
        {IDC_SIDE_PT_Y, g.pdisp.y, side_pt_y_text()},
        {IDC_SIDE_PT_DX, g.pdisp.dx, side_pt_dx_text()},
        {IDC_SIDE_PT_DY, g.pdisp.dy, side_pt_dy_text()},
        {IDC_SIDE_PT_INVDT, g.pdisp.inv_dt, side_pt_invdt_text()},
        {IDC_SIDE_PT_DIST, g.pdisp.dist, side_pt_dist_text()},
        {IDC_SIDE_PT_SNAP, g.snap_to_data, side_pt_snap_text()},
    };
    for (const auto& item : point_toggles) {
        HWND ctl = GetDlgItem(g.main, item.id);
        if (!ctl) continue;
        SetWindowTextW(ctl, item.text);
        set_toggle_checked(ctl, item.value);
    }
    sync_filter_controls_from_state();
    apply_side_panel_visibility();
}

void redraw_button(HWND btn);
void redraw_toolbar_buttons();
void redraw_window_with_children(HWND hwnd);
void show_welcome(HINSTANCE inst);
void raise_main_window();
void enable_file_drop_support(HWND hwnd);
void toggle_welcome_recent_files_panel(HWND owner);
void show_welcome_recent_files_panel(HWND owner);
void hide_welcome_recent_files_panel();
bool welcome_recent_files_panel_visible();

#include "gui_layout.cpp"
void clamp_range(double& lo, double& hi, double minb, double maxb, double minw) {
    double w = hi - lo;
    const double full = maxb - minb;
    if (w < minw) w = minw;
    if (w > full) w = full;
    if (lo < minb) lo = minb;
    hi = lo + w;
    if (hi > maxb) { hi = maxb; lo = hi - w; if (lo < minb) lo = minb; }
}

bool active_axis(double*& lo, double*& hi, double& minb, double& maxb, double& minw) {
    if (g.freq_mode) {
        if (!ensure_current_spectrum() || g.spec.freqs.size() < 2) return false;
        lo = &g.freq_start; hi = &g.freq_end;
        minb = 0.0; maxb = g.spec.nyquist;
        minw = (g.spec.freqs[1] - g.spec.freqs[0]) * 0.5;
        return true;
    }
    if (!has_data()) return false;
    lo = &g.win_start; hi = &g.win_end;
    minb = g.data_t0; maxb = g.data_t1;
    minw = std::max(g.approx_dt * 0.5, (g.data_t1 - g.data_t0) * 1e-6);
    return true;
}

bool current_time_yrange(double& ymin, double& ymax);
bool current_freq_yrange(double& ymin, double& ymax);
void set_mode(bool freq_mode);
void invalidate_plot();
void release_backbuffer();
void rebuild_ui();
void rebuild_accelerators();
HMENU make_menu();
std::wstring hotkey_text_for_command(int command);
bool parse_wide_double_text(const wchar_t* text, double& out);
bool prompt_numeric_value(const wchar_t* title, const wchar_t* label,
                          const wchar_t* apply_text, const wchar_t* cancel_text,
                          const wchar_t* invalid_text, double initial_value,
                          bool positive_only, double& out_value);
void fill_rounded_rect(HDC dc, const RECT& r, COLORREF fill, COLORREF border, int radius);
void draw_welcome_action_button(HDC dc, const RECT& r, const wchar_t* txt, bool pressed, bool primary, bool outlined);
void draw_themed_button(HDC dc, const RECT& r, const wchar_t* txt, bool pressed, bool active, bool hover);
void draw_themed_check_control(HDC dc, const RECT& r, const wchar_t* txt,
                               bool checked, bool pressed, bool enabled,
                               bool radio, bool compact,
                               COLORREF surface_bg);

const wchar_t* speed_menu_text() {
    return (g_str == &kEn) ? L"Playback speed…" : L"Скорость воспроизведения…";
}


void set_play_speed(double speed) {
    if (!(speed > 0.0) || !std::isfinite(speed)) return;
    if (g.playing) {
        LARGE_INTEGER now{}, freq{};
        QueryPerformanceCounter(&now);
        QueryPerformanceFrequency(&freq);
        const double elapsed = static_cast<double>(now.QuadPart - g.play_anchor_qpc.QuadPart) /
                               static_cast<double>(freq.QuadPart);
        g.playhead = g.play_anchor_data + elapsed * g.play_speed;
        if (has_data()) {
            if (g.playhead < g.data_t0) g.playhead = g.data_t0;
            if (g.playhead > g.data_t1) g.playhead = g.data_t1;
        }
        g.play_anchor_data = g.playhead;
        g.play_anchor_qpc = now;
    }
    g.play_speed = speed;
    save_runtime_settings();
    set_status();
}

#include "gui_dialogs.cpp"

void add_guide_line(bool vertical, double value) {
    GuideLine gl;
    gl.vertical = vertical;
    gl.value = value;
    gl.freq = g.freq_mode;
    g.guides.push_back(gl);
    UndoAction ua;
    ua.type = UndoAction::ADD_LINE;
    ua.line = gl;
    push_undo(ua);
    g.pending_line = 0;
    sync_menu();
    set_status();
    invalidate_plot();
}

void invalidate_plot() {
    RECT pr = plot_rect();
    RECT rc; GetClientRect(g.main, &rc);
    pr.bottom = rc.bottom; // include status bar
    InvalidateRect(g.main, &pr, FALSE);
}

void zoom_at(double center_frac, double factor) {
    double *lo, *hi, minb, maxb, minw;
    if (!active_axis(lo, hi, minb, maxb, minw)) return;
    const double w = *hi - *lo;
    const double c = *lo + w * center_frac;
    double nw = w * factor;
    if (nw < minw) nw = minw;
    const double full = maxb - minb;
    if (nw > full) nw = full;
    double nlo = c - nw * center_frac;
    double nhi = nlo + nw;
    if (nlo < minb) { nlo = minb; nhi = nlo + nw; }
    if (nhi > maxb) { nhi = maxb; nlo = nhi - nw; if (nlo < minb) nlo = minb; }
    *lo = nlo;
    *hi = nhi;
    set_status();
    invalidate_plot();
}

void zoom_y_at(double center_frac, double factor) {
    if (!has_data()) return;
    double ymin, ymax;
    current_time_yrange(ymin, ymax);
    if (!g.auto_y) { ymin = g.y_lock_min; ymax = g.y_lock_max; }
    const double w = ymax - ymin;
    if (w <= 0) return;
    const double c = ymin + w * center_frac;
    double nw = w * factor;
    const double minw = std::max(1e-12, w * 1e-6);
    if (nw < minw) nw = minw;
    double nlo = c - nw * center_frac;
    double nhi = nlo + nw;
    g.y_lock_min = nlo;
    g.y_lock_max = nhi;
    g.auto_y = false;
    if (g.autoy) { SendMessageW(g.autoy, BM_SETCHECK, BST_UNCHECKED, 0); InvalidateRect(g.autoy, nullptr, FALSE); }
    sync_menu();
    set_status();
    invalidate_plot();
}

void zoom_y_amp_at(double center_frac, double factor) {
    (void)center_frac;
    double ymax = visible_spectrum_ymax();
    if (ymax <= 0) ymax = 1.0;
    double ytop = ymax * 1.08;
    if (!g.auto_y_amp) ytop = g.y_amp_max;
    const double w = ytop;
    if (w <= 0) return;
    double nw = w * factor;
    const double minw = std::max(1e-12, w * 1e-6);
    if (nw < minw) nw = minw;
    g.y_amp_max = nw;
    g.auto_y_amp = false;
    set_status();
    invalidate_plot();
}

void pan_by(double frac) {
    double *lo, *hi, minb, maxb, minw;
    if (!active_axis(lo, hi, minb, maxb, minw)) return;
    const double w = *hi - *lo;
    *lo += w * frac;
    *hi += w * frac;
    clamp_range(*lo, *hi, minb, maxb, minw);
    set_status();
    invalidate_plot();
}

bool prepare_plot_drag(int mx, int my) {
    double *lo, *hi, minb, maxb, minw;
    if (!active_axis(lo, hi, minb, maxb, minw)) return false;
    g.drag_x = mx;
    g.drag_y = my;
    g.drag_lo = *lo;
    g.drag_hi = *hi;
    if (g.freq_mode) {
        if (g.auto_y_amp) {
            double ymax = visible_spectrum_ymax();
            if (ymax <= 0) ymax = 1.0;
            g.drag_y_hi = ymax * 1.08;
        } else {
            g.drag_y_hi = g.y_amp_max;
        }
        g.drag_y_lo = 0.0;
    } else {
        if (g.auto_y) {
            current_time_yrange(g.drag_y_lo, g.drag_y_hi);
        } else {
            g.drag_y_lo = g.y_lock_min;
            g.drag_y_hi = g.y_lock_max;
        }
    }
    return true;
}

void pan_y_by(double frac) {
    if (g.freq_mode) {
        double ymax = visible_spectrum_ymax();
        if (ymax <= 0) ymax = 1.0;
        double ytop = ymax * 1.08;
        if (!g.auto_y_amp) ytop = g.y_amp_max;
        const double w = ytop;
        const double shift = w * frac;
        g.y_amp_max = ytop + shift;
        if (g.y_amp_max < 1e-12) g.y_amp_max = 1e-12;
        g.auto_y_amp = false;
        set_status();
        invalidate_plot();
        return;
    }
    if (!has_data()) return;
    double ymin, ymax;
    current_time_yrange(ymin, ymax);
    if (g.auto_y) {
        g.y_lock_min = ymin;
        g.y_lock_max = ymax;
        g.auto_y = false;
        if (g.autoy) { SendMessageW(g.autoy, BM_SETCHECK, BST_UNCHECKED, 0); InvalidateRect(g.autoy, nullptr, FALSE); }
        sync_menu();
    }
    const double w = g.y_lock_max - g.y_lock_min;
    const double shift = w * frac;
    g.y_lock_min += shift;
    g.y_lock_max += shift;
    set_status();
    invalidate_plot();
}

void goto_start() {
    double *lo, *hi, minb, maxb, minw;
    if (!active_axis(lo, hi, minb, maxb, minw)) return;
    const double w = *hi - *lo;
    *lo = minb;
    *hi = minb + w;
    if (*hi > maxb) { *hi = maxb; *lo = maxb - w; if (*lo < minb) *lo = minb; }
    set_status();
    invalidate_plot();
}

void goto_end() {
    double *lo, *hi, minb, maxb, minw;
    if (!active_axis(lo, hi, minb, maxb, minw)) return;
    const double w = *hi - *lo;
    *hi = maxb;
    *lo = maxb - w;
    if (*lo < minb) { *lo = minb; *hi = minb + w; if (*hi > maxb) *hi = maxb; }
    set_status();
    invalidate_plot();
}

void reset_view() {
    g.win_start = g.data_t0;
    g.win_end = g.data_t1;
    g.freq_start = 0.0;
    g.freq_end = g.spec_valid ? g.spec.nyquist : 1.0;
    set_status();
    invalidate_plot();
}

// ---- playback ------------------------------------------------------------

void stop_play() {
    g.playing = false;
    KillTimer(g.main, 1);
    if (g.play) SetWindowTextW(g.play, g_str->btn_play);
    redraw_button(g.play);
}

void start_play() {
    if (!has_data() || g.freq_mode) return;
    g.playing = true;
    g.playhead_active = true;
    if (g.playhead < g.win_start || g.playhead >= g.data_t1) g.playhead = g.win_start;
    // Anchor the playhead to wall-clock time so playback runs at 1 s of signal
    // per 1 s of real time, independent of timer jitter.
    g.play_anchor_data = g.playhead;
    QueryPerformanceCounter(&g.play_anchor_qpc);
    SetTimer(g.main, 1, 16, nullptr);   // ~60 fps for smooth scrolling
    if (g.play) SetWindowTextW(g.play, g_str->btn_pause);
    redraw_button(g.play);
}

void toggle_play() {
    if (g.playing) stop_play();
    else start_play();
}

// ---- loading overlay and file drop --------------------------------------
#include "gui_loading_drop.cpp"

double precompute_global_gap_step(const std::vector<double>& time) {
    if (time.size() <= 1) return 0.0;
    std::vector<double> diffs;
    diffs.reserve(std::min<std::size_t>(4096, time.size() - 1));
    const std::size_t stride = std::max<std::size_t>(1, (time.size() - 1) / 4096);
    for (std::size_t i = 1; i < time.size() && diffs.size() < 4096; i += stride) {
        const double diff = time[i] - time[i - 1];
        if (std::isfinite(diff) && diff > 0.0) diffs.push_back(diff);
    }
    if (diffs.empty()) return 0.0;
    std::sort(diffs.begin(), diffs.end());
    const std::size_t mid = diffs.size() / 2;
    return (diffs.size() % 2 == 0)
        ? 0.5 * (diffs[mid - 1] + diffs[mid])
        : diffs[mid];
}

void ensure_global_gap_step_ready() {
    if (g.cached_global_gap_step_ready) return;
    g.cached_global_gap_step = precompute_global_gap_step(g.ds.time);
    g.cached_global_gap_step_ready = true;
}

void invalidate_plot_analysis_cache() {
    ++g.plot_analysis_serial;
    g.time_yrange_cache.valid = false;
}

template <typename TResult>
void post_async_result(UINT message, std::unique_ptr<TResult> result) {
    TResult* raw = result.release();
    if (!raw) return;
    if (!g.main || !IsWindow(g.main) || !PostMessageW(g.main, message, 0, reinterpret_cast<LPARAM>(raw))) {
        delete raw;
    }
}

void request_async_load_cancel() {
    if (g.async_load_cancel_flag) {
        g.async_load_cancel_flag->store(true, std::memory_order_relaxed);
    }
    if (g_loading_cancel_btn && IsWindow(g_loading_cancel_btn)) {
        EnableWindow(g_loading_cancel_btn, FALSE);
    }
}

void apply_loaded_dataset(lvm::Dataset ds, const std::wstring& wpath, bool hide_channels,
                          bool requested_time_window, double cached_global_gap_step,
                          bool cached_global_gap_step_ready) {
    if (ds.time.empty()) {
        g.last_error = "No time data available.";
        MessageBoxW(g.main, to_w(g.last_error).c_str(), g_str->msg_read_err, MB_ICONERROR | MB_OK);
        return;
    }

    g.current_file_partial = requested_time_window || ds.partial;
    g.ds = std::move(ds);
    g.visible.assign(g.ds.channel_count(), hide_channels ? 0 : 1);
    g.channel_labels.assign(g.ds.channel_count(), L"");
    for (std::size_t i = 0; i < g.ds.channel_count(); ++i) {
        if (i < g.ds.names.size() && !g.ds.names[i].empty()) {
            g.channel_labels[i] = to_w(g.ds.names[i]);
        } else {
            g.channel_labels[i] = std::wstring(L"Channel_") + std::to_wstring(i + 1);
        }
    }
    g_channel_colors.clear();
    g_channel_colors.reserve(g.ds.channel_count());
    for (std::size_t i = 0; i < g.ds.channel_count(); ++i) g_channel_colors.push_back(kPalette[i % (sizeof(kPalette) / sizeof(kPalette[0]))]);
    g.side_selected_channel = g.ds.channel_count() > 0 ? 0 : -1;
    g.side_scroll_y = 0;
    g.channel_formulas.assign(g.ds.channel_count(), default_channel_formula_text());
    g.channel_formula_rpn.assign(g.ds.channel_count(), {});
    g.global_formula = default_channel_formula_text();
    g.global_formula_rpn.clear();
    g.formula_ini_deferred = g.light_mode;
    if (!g.formula_ini_deferred) load_channel_formulas_from_ini();
    invalidate_formula_runtime();
    g.data_t0 = g.ds.time.front();
    g.data_t1 = g.ds.time.back();
    if (g.data_t1 <= g.data_t0) g.data_t1 = g.data_t0 + 1.0;
    g.win_start = g.data_t0;
    g.win_end = g.data_t1;
    g.approx_dt = (g.data_t1 - g.data_t0) / static_cast<double>(g.ds.rows());
    g.cached_global_gap_step = cached_global_gap_step;
    g.cached_global_gap_step_ready = cached_global_gap_step_ready;
    invalidate_plot_analysis_cache();
    clear_measure_point_groups();
    g.guides.clear();
    g.markers.clear();
    g.active_marker = -1;
    clear_fft_window();
    g.fft_selecting = false;
    g.spec_source_valid = false;
    g.spec_source_start = 0.0;
    g.spec_source_end = 0.0;
    g.spec_source_from_selection = false;
    g_undo.clear();
    g_redo.clear();
    hide_gap_details_card();
    stop_play();
    g.playhead = g.data_t0;
    g.playhead_active = false;
    g.auto_y = true;   // a fresh file starts on auto-fit
    if (hide_channels) g.auto_y_amp = true;
    if (g.freq_mode) {
        // New files should open in the time plot by default.
        set_mode(false);
    }
    if (g.autoy) { SendMessageW(g.autoy, BM_SETCHECK, BST_CHECKED, 0); InvalidateRect(g.autoy, nullptr, FALSE); }
    if (g.menu) CheckMenuItem(g.menu, IDC_AUTOY, MF_BYCOMMAND | MF_CHECKED);
    apply_export_metadata_from_comments(g.ds.export_comments);
    bool reloaded_ini_formulas = false;
    if (!g.light_mode && g.formula_ini_deferred) {
        load_channel_formulas_from_ini();
        g.formula_ini_deferred = false;
        reloaded_ini_formulas = true;
    }
    if (reloaded_ini_formulas) {
        recompute_transforms_from_state();
    }
    clear_spectrum_cache_state();
    g.spec_source_valid = false;
    g.freq_start = 0.0;
    g.freq_end = 1.0;
    if (!requested_time_window) {
        g.cached_scan_path = wpath;
        g.cached_scan_start = g.data_t0;
        g.cached_scan_end = g.data_t1;
        g.cached_scan_valid = true;
    }

    const wchar_t* base = wcsrchr(wpath.c_str(), L'\\');
    g.file_name = base ? base + 1 : wpath;
    SetWindowTextW(g.main, (std::wstring(g_str->app_title) + L" — " + g.file_name).c_str());
    add_recent_file(wpath);
    if (g.welcome_wnd) { ShowWindow(g.welcome_wnd, SW_HIDE); show_ui_controls(); }

    rebuild_checks();
    refresh_side_panel_controls();
    refresh_settings_controls();
    layout();
    set_status();
    release_backbuffer();
    invalidate_plot();
    RedrawWindow(g.main, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
    raise_main_window();
    g.last_error.clear();
}

bool start_async_scan_task(const std::wstring& wpath) {
    if (g.async_load_stage != AsyncLoadStage::None) {
        g.last_error = "A file is already loading.";
        return false;
    }
    g.last_error.clear();
    g.async_load_stage = AsyncLoadStage::ScanningRange;
    const unsigned long long token = ++g.async_load_token;
    auto cancel_flag = std::make_shared<std::atomic<bool>>(false);
    g.async_load_cancel_flag = cancel_flag;
    show_loading(g_str->msg_scanning_range, true);

    const std::wstring path_copy = wpath;
    const std::string narrow_path = to_acp(wpath.c_str());
    try {
        std::thread([token, path_copy, narrow_path, cancel_flag]() {
            auto result = std::make_unique<AsyncScanResult>();
            result->token = token;
            result->path = path_copy;
            std::string scan_error;
            result->ok = lvm::scan_time_bounds(narrow_path, result->range_start, result->range_end, scan_error, cancel_flag.get());
            result->cancelled = !result->ok && cancel_flag->load(std::memory_order_relaxed);
            if (!result->ok) result->error = std::move(scan_error);
            post_async_result(WM_APP_ASYNC_SCAN_DONE, std::move(result));
        }).detach();
    } catch (const std::exception& ex) {
        g.async_load_stage = AsyncLoadStage::None;
        g.async_load_cancel_flag.reset();
        hide_loading();
        g.last_error = ex.what();
        return false;
    }
    return true;
}

bool start_async_load_task(const std::wstring& wpath, const double* fragment_start = nullptr,
                           const double* fragment_end = nullptr, bool hide_channels = false) {
    if (g.async_load_stage != AsyncLoadStage::None) {
        g.last_error = "A file is already loading.";
        return false;
    }

    lvm::LoadOptions load_options{};
    if (fragment_start && fragment_end && std::isfinite(*fragment_start) &&
        std::isfinite(*fragment_end) && *fragment_end > *fragment_start) {
        load_options.use_time_window = true;
        load_options.time_start = *fragment_start;
        load_options.time_end = *fragment_end;
    }

    g.last_error.clear();
    g.async_load_stage = AsyncLoadStage::LoadingFile;
    const unsigned long long token = ++g.async_load_token;
    auto cancel_flag = std::make_shared<std::atomic<bool>>(false);
    g.async_load_cancel_flag = cancel_flag;
    show_loading(hide_channels ? g_str->msg_loading_light : g_str->msg_loading, true);

    const std::wstring path_copy = wpath;
    const std::string narrow_path = to_acp(wpath.c_str());
    try {
        std::thread([token, path_copy, narrow_path, load_options, hide_channels, cancel_flag]() mutable {
            auto result = std::make_unique<AsyncLoadResult>();
            result->token = token;
            result->path = path_copy;
            result->hide_channels = hide_channels;
            result->requested_time_window = load_options.use_time_window;
            load_options.cancel_flag = cancel_flag.get();
            result->ds = lvm::read_lvm_file(narrow_path, load_options);
            result->ok = result->ds.ok;
            result->cancelled = !result->ok && cancel_flag->load(std::memory_order_relaxed);
            if (result->ok) {
                const std::vector<double> raw_time = result->ds.raw_time.empty() ? result->ds.time : result->ds.raw_time;
                lvm::drop_duplicate_time_channels(result->ds, raw_time);
                if (!load_options.use_time_window) {
                    // Sectioned LabVIEW exports can still contain small backward jumps
                    // between blocks; normalize them so the plot stays strictly ordered.
                    lvm::make_monotonic(result->ds.time);
                }
                if (!hide_channels) {
                    result->cached_global_gap_step = precompute_global_gap_step(result->ds.time);
                    result->cached_global_gap_step_ready = true;
                }
            } else {
                result->error = result->ds.error;
            }
            post_async_result(WM_APP_ASYNC_LOAD_DONE, std::move(result));
        }).detach();
    } catch (const std::exception& ex) {
        g.async_load_stage = AsyncLoadStage::None;
        g.async_load_cancel_flag.reset();
        hide_loading();
        g.last_error = ex.what();
        return false;
    }
    return true;
}

bool prompt_and_start_light_mode_load(const std::wstring& wpath, double range_start, double range_end) {
    double fragment_start = 0.0;
    double fragment_end = 0.0;
    if (!prompt_light_mode_window(range_start, range_end, fragment_start, fragment_end)) {
        g.last_error.clear();
        return false;
    }
    return start_async_load_task(wpath, &fragment_start, &fragment_end, true);
}

bool load_path_interactive(const std::wstring& wpath) {
    g.last_error.clear();
    if (g.light_mode) {
        if (g.cached_scan_valid && lstrcmpiW(g.cached_scan_path.c_str(), wpath.c_str()) == 0) {
            return prompt_and_start_light_mode_load(wpath, g.cached_scan_start, g.cached_scan_end);
        }
        return start_async_scan_task(wpath);
    }
    return start_async_load_task(wpath);
}

void open_file() {
    wchar_t file[2048] = L"";
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g.main;
    ofn.lpstrFilter = g_str->filter_open;
    ofn.lpstrFile = file;
    ofn.nMaxFile = 2048;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (!GetOpenFileNameW(&ofn)) return;
    if (!load_path_interactive(file) && !g.last_error.empty())
        MessageBoxW(g.main, to_w(g.last_error).c_str(), g_str->msg_read_err, MB_ICONERROR | MB_OK);
}

void show_recent_files_menu(HWND owner) {
    toggle_welcome_recent_files_panel(owner);
}

// ---- series ---------------------------------------------------------------

struct ChannelRenderView {
    const std::vector<double>* raw = nullptr;
    const std::vector<double>* cache = nullptr;
    TransformRuntimeKind kind = TransformRuntimeKind::Identity;
    double mul = 1.0;
    double add = 0.0;
};

ChannelRenderView make_channel_render_view(std::size_t channel_index) {
    ChannelRenderView view;
    if (channel_index >= g.ds.channels.size()) return view;
    view.raw = &g.ds.channels[channel_index];
    view.kind = (channel_index < g.channel_transform_kind.size())
        ? g.channel_transform_kind[channel_index]
        : TransformRuntimeKind::Identity;
    if (g.noise_threshold_enabled) {
        ensure_filtered_channel_cache(channel_index);
        if (channel_index < g.filtered_channel_cache.size()) {
            view.cache = &g.filtered_channel_cache[channel_index];
        }
    } else if (view.kind == TransformRuntimeKind::Affine) {
        view.mul = g.channel_transform_mul[channel_index];
        view.add = g.channel_transform_add[channel_index];
    } else if (view.kind == TransformRuntimeKind::CachedFormula) {
        ensure_transformed_channel_cache(channel_index);
        view.cache = &g.transformed_channel_cache[channel_index];
    }
    return view;
}

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

bool any_visible_channel() {
    for (char visible : g.visible) {
        if (visible) return true;
    }
    return false;
}

bool current_time_yrange_window(std::size_t lo, std::size_t hi, double& ymin, double& ymax) {
    if (!has_data() || hi <= lo) return false;
    if (!any_visible_channel()) {
        ymin = -1.0;
        ymax = 1.0;
        return true;
    }
    if (g.time_yrange_cache.valid &&
        g.time_yrange_cache.serial == g.plot_analysis_serial &&
        g.time_yrange_cache.lo == lo &&
        g.time_yrange_cache.hi == hi &&
        g.time_yrange_cache.win_start == g.win_start &&
        g.time_yrange_cache.win_end == g.win_end) {
        ymin = g.time_yrange_cache.ymin;
        ymax = g.time_yrange_cache.ymax;
        return true;
    }
    ensure_channel_formula_vectors();
    const std::size_t stride = light_mode_render_stride(hi - lo, g.light_mode ? 120000u : 250000u);
    ymin = 1e300; ymax = -1e300;
    for (std::size_t c = 0; c < g.ds.channel_count(); ++c) {
        if (!g.visible[c]) continue;
        const ChannelRenderView view = make_channel_render_view(c);
        for (std::size_t i = lo; i < hi; i += stride) {
            const double v = channel_render_value(view, i);
            if (std::isnan(v)) continue;
            if (v < ymin) ymin = v;
            if (v > ymax) ymax = v;
        }
        if (stride > 1) {
            const std::size_t last = hi - 1;
            const double v = channel_render_value(view, last);
            if (!std::isnan(v)) {
                if (v < ymin) ymin = v;
                if (v > ymax) ymax = v;
            }
        }
    }
    if (ymin > ymax) { ymin = -1; ymax = 1; }
    if (ymax - ymin < 1e-12) { ymin -= 1; ymax += 1; }
    const double pad = (ymax - ymin) * 0.05;
    ymin -= pad; ymax += pad;
    g.time_yrange_cache.valid = true;
    g.time_yrange_cache.lo = lo;
    g.time_yrange_cache.hi = hi;
    g.time_yrange_cache.win_start = g.win_start;
    g.time_yrange_cache.win_end = g.win_end;
    g.time_yrange_cache.serial = g.plot_analysis_serial;
    g.time_yrange_cache.ymin = ymin;
    g.time_yrange_cache.ymax = ymax;
    return true;
}

// Auto-fit vertical range over the currently visible time window (with 5% pad).
bool current_time_yrange(double& ymin, double& ymax) {
    if (!has_data()) return false;
    const std::vector<double>& t = g.ds.time;
    const std::size_t n = t.size();
    std::size_t lo = static_cast<std::size_t>(
        std::lower_bound(t.begin(), t.end(), g.win_start) - t.begin());
    std::size_t hi = static_cast<std::size_t>(
        std::upper_bound(t.begin(), t.end(), g.win_end) - t.begin());
    if (lo > 0) --lo;
    if (hi < n) ++hi;
    return current_time_yrange_window(lo, hi, ymin, ymax);
}

// Auto-fit amplitude range over the currently visible FFT window (with 5% pad).
bool current_freq_yrange(double& ymin, double& ymax) {
    if (!g.freq_mode) return false;
    if (!ensure_current_spectrum() || g.spec.freqs.size() < 2) return false;
    ymin = 0.0;
    ymax = 0.0;
    std::size_t klo = static_cast<std::size_t>(
        std::lower_bound(g.spec.freqs.begin(), g.spec.freqs.end(), g.freq_start) - g.spec.freqs.begin());
    std::size_t khi = static_cast<std::size_t>(
        std::upper_bound(g.spec.freqs.begin(), g.spec.freqs.end(), g.freq_end) - g.spec.freqs.begin());
    if (klo > 0) --klo;
    if (khi < g.spec.freqs.size()) ++khi;
    for (std::size_t j = 0; j < g.spec.amp.size(); ++j) {
        const int ci = (j < g.spec_channel_indices.size()) ? g.spec_channel_indices[j] : -1;
        if (ci < 0 || !g.visible[ci]) continue;
        for (std::size_t k = klo; k < khi; ++k) {
            if (g.spec.amp[j][k] > ymax) ymax = g.spec.amp[j][k];
        }
    }
    if (ymax <= 0.0) ymax = 1.0;
    const double pad = ymax * 0.05;
    ymax += pad;
    return true;
}

double visible_spectrum_ymax() {
    if (!ensure_current_spectrum()) return 0.0;
    double ymax = 0.0;
    for (std::size_t j = 0; j < g.spec.amp.size(); ++j) {
        const int ci = (j < g.spec_channel_indices.size()) ? g.spec_channel_indices[j] : -1;
        if (ci < 0 || !g.visible[ci]) continue;
        for (double v : g.spec.amp[j]) {
            if (v > ymax) ymax = v;
        }
    }
    return ymax;
}

#include "gui_render.cpp"
// ---- PNG export (GDI+) ---------------------------------------------------

int png_encoder_clsid(CLSID* clsid) {
    UINT num = 0, size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;
    auto* info = reinterpret_cast<Gdiplus::ImageCodecInfo*>(malloc(size));
    if (!info) return -1;
    Gdiplus::GetImageEncoders(num, size, info);
    int found = -1;
    for (UINT i = 0; i < num; ++i)
        if (wcscmp(info[i].MimeType, L"image/png") == 0) { *clsid = info[i].Clsid; found = static_cast<int>(i); break; }
    free(info);
    return found;
}

bool save_png(const std::wstring& path) {
    RECT pr = plot_rect();
    int W = (pr.right - pr.left) + 90;
    int H = (pr.bottom - pr.top) + 60;
    if (W < 400) W = 1000;
    if (H < 240) H = 600;

    HDC screen = GetDC(g.main);
    HDC mem = CreateCompatibleDC(screen);
    HBITMAP bmp = CreateCompatibleBitmap(screen, W, H);
    HGDIOBJ obmp = SelectObject(mem, bmp);

    RECT all = {0, 0, W, H};
    HBRUSH bg = CreateSolidBrush(g_theme->bg_plot);
    FillRect(mem, &all, bg);
    DeleteObject(bg);
    SelectObject(mem, reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT)));
    SetBkMode(mem, TRANSPARENT);
    RECT inner = {70, 14, W - 20, H - 46};
    draw_chart(mem, inner);

    SelectObject(mem, obmp);

    bool ok = false;
    {
        Gdiplus::Bitmap gb(bmp, nullptr);
        CLSID clsid;
        if (png_encoder_clsid(&clsid) >= 0)
            ok = (gb.Save(path.c_str(), &clsid, nullptr) == Gdiplus::Ok);
    }

    DeleteObject(bmp);
    DeleteDC(mem);
    ReleaseDC(g.main, screen);
    // draw_chart updated the mapping cache for the off-screen rect; restore it.
    InvalidateRect(g.main, nullptr, FALSE);
    return ok;
}

#include "gui_processing.cpp"
                           const ExportOptions& opts,
                           bool csv,
                           double range_start,
                           double range_end,
                           bool actual_selected_range) {
    const char* line_end = csv ? "\n" : "\r\n";

    write_export_comment(out, L"[export]", line_end);
    write_export_key_value(out, L"range_mode", export_range_mode_key(opts.selected_range), line_end);
    write_export_key_value(out, L"range_source", actual_selected_range ? L"selected" : L"visible", line_end);
    write_export_key_value(out, L"data_mode", export_processing_mode_key(opts.apply_processing_to_data), line_end);
    write_export_key_value(out, L"plot_mode", g.freq_mode ? L"frequency" : L"time", line_end);
    write_export_key_value(out, L"range_start", format_edit_number(range_start), line_end);
    write_export_key_value(out, L"range_end", format_edit_number(range_end), line_end);
    if (!g.file_name.empty()) {
        write_export_key_value(out, L"source_file", g.file_name, line_end);
    }
    write_export_key_value(out, L"partial_fragment", g.current_file_partial ? L"1" : L"0", line_end);
    write_export_comment(out, L"", line_end);

    if (opts.include_graph_settings) {
        write_export_comment(out, L"[graph_settings]", line_end);
        write_export_key_value(out, L"axis_x_label", g.axis_x_label, line_end);
        write_export_key_value(out, L"axis_y_label", g.axis_y_label, line_end);
        write_export_key_value(out, L"marker_color", export_color_triplet(g.marker_color), line_end);
        write_export_key_value(out, L"smoothing", g.visual_smooth ? L"1" : L"0", line_end);
        write_export_key_value(out, L"vertical_pan", g.vertical_pan ? L"1" : L"0", line_end);
        write_export_key_value(out, L"snap_to_data", g.snap_to_data ? L"1" : L"0", line_end);
        write_export_key_value(out, L"show_gap_markers", g.show_gap_markers ? L"1" : L"0", line_end);
        write_export_key_value(out, L"light_mode", g.light_mode ? L"1" : L"0", line_end);
        write_export_key_value(out, L"auto_y", g.auto_y ? L"1" : L"0", line_end);
        write_export_key_value(out, L"y_lock_min", format_optional_edit_number(g.y_lock_min), line_end);
        write_export_key_value(out, L"y_lock_max", format_optional_edit_number(g.y_lock_max), line_end);
        write_export_key_value(out, L"auto_y_amp", g.auto_y_amp ? L"1" : L"0", line_end);
        write_export_key_value(out, L"y_amp_max", format_optional_edit_number(g.y_amp_max), line_end);
        write_export_key_value(out, L"play_speed", format_edit_number(g.play_speed), line_end);
        write_export_key_value(out, L"point_display", export_point_display_text(g.pdisp), line_end);
        write_export_comment(out, L"", line_end);
    }

    if (opts.include_filter_settings) {
        write_export_comment(out, L"[filter_settings]", line_end);
        write_export_key_value(out, L"enabled", g.noise_threshold_enabled ? L"1" : L"0", line_end);
        write_export_key_value(out, L"mode", export_filter_mode_key(g.noise_threshold_mode), line_end);
        write_export_key_value(out, L"topology", export_filter_topology_key(g.noise_threshold_topology), line_end);
        write_export_key_value(out, L"low_cutoff", format_optional_edit_number(g.noise_threshold_min), line_end);
        write_export_key_value(out, L"high_cutoff", format_optional_edit_number(g.noise_threshold_max), line_end);
        write_export_comment(out, L"", line_end);
    }

    if (opts.include_channel_names || opts.include_hidden_channels) {
        write_export_comment(out, L"[channels]", line_end);
        ensure_channel_formula_vectors();
        for (std::size_t c = 0; c < g.ds.channel_count(); ++c) {
            std::wstring line = L"channel[" + std::to_wstring(c + 1) + L"] name=" + channel_display_label(c);
            line += L", visible=";
            line += (c < g.visible.size() && g.visible[c]) ? L"1" : L"0";
            line += L", color=" + export_color_triplet(channel_color(c));
            write_export_comment(out, line, line_end);
        }
        write_export_comment(out, L"", line_end);
    }

    if (opts.include_formulas) {
        write_export_comment(out, L"[formulas]", line_end);
        write_export_key_value(out, L"global", g.global_formula, line_end);
        for (std::size_t c = 0; c < g.channel_formulas.size(); ++c) {
            write_export_key_value(out, L"channel[" + std::to_wstring(c + 1) + L"]", g.channel_formulas[c], line_end);
        }
        write_export_comment(out, L"", line_end);
    }

    if (opts.include_points && !g.point_groups.empty()) {
        write_export_comment(out, L"[point_groups]", line_end);
        for (std::size_t i = 0; i < g.point_groups.size(); ++i) {
            const auto& group = g.point_groups[i];
            std::wstring line = L"group[" + std::to_wstring(i + 1) + L"] name=" + group.name;
            line += L", visible=" + std::wstring(group.visible ? L"1" : L"0");
            line += L", color=" + export_color_triplet(group.color);
            line += L", display=" + export_point_display_text(group.display);
            write_export_comment(out, line, line_end);
            for (std::size_t j = 0; j < group.points.size(); ++j) {
                const auto& pt = group.points[j];
                std::wstring point_line = L"point[" + std::to_wstring(j + 1) + L"] x=" + format_edit_number(pt.first);
                point_line += L", y=" + format_edit_number(pt.second);
                write_export_comment(out, point_line, line_end);
            }
        }
        write_export_comment(out, L"", line_end);
    }

    if (opts.include_markers && !g.markers.empty()) {
        write_export_comment(out, L"[markers]", line_end);
        for (std::size_t i = 0; i < g.markers.size(); ++i) {
            const auto& m = g.markers[i];
            std::wstring line = L"marker[" + std::to_wstring(i + 1) + L"] label=" + m.label;
            line += L", x=" + format_edit_number(m.x);
            line += L", y=" + format_edit_number(m.y);
            line += L", mode=" + std::wstring(m.freq ? L"frequency" : L"time");
            line += L", snapped=" + std::wstring(m.snapped ? L"1" : L"0");
            line += L", channel=" + std::to_wstring(m.channel);
            write_export_comment(out, line, line_end);
        }
        write_export_comment(out, L"", line_end);
    }

    if (opts.include_guides && !g.guides.empty()) {
        write_export_comment(out, L"[guides]", line_end);
        for (std::size_t i = 0; i < g.guides.size(); ++i) {
            const auto& gl = g.guides[i];
            std::wstring line = L"guide[" + std::to_wstring(i + 1) + L"] kind=" + (gl.vertical ? L"vertical" : L"horizontal");
            line += L", value=" + format_edit_number(gl.value);
            line += L", mode=" + std::wstring(gl.freq ? L"frequency" : L"time");
            write_export_comment(out, line, line_end);
        }
        write_export_comment(out, L"", line_end);
    }
}

#include "gui_export_metadata.cpp"
// ---- hit testing ---------------------------------------------------------

bool px_to_data(int px, int py, double& dx, double& dy) {
    if (!g.vvalid) return false;
    const RECT& p = g.vrect;
    if (p.right <= p.left || p.bottom <= p.top) return false;
    dx = g.vx0 + static_cast<double>(px - p.left) / (p.right - p.left) * (g.vx1 - g.vx0);
    dy = g.vy0 + static_cast<double>(p.bottom - py) / (p.bottom - p.top) * (g.vy1 - g.vy0);
    return true;
}

// Snap a clicked coordinate to the nearest visible real sample (Time mode) or
// spectrum point (Hz mode) by on-screen distance, so a marker lands on the
// visually closest data point. The stored data is never modified � only the
// marker is adjusted.
bool snap_to_nearest_target(double& dx, double& dy, int* out_channel = nullptr) {
    if (!g.vvalid) return false;
    const RECT& p = g.vrect;
    const int pw = p.right - p.left;
    const int ph = p.bottom - p.top;
    if (pw <= 0 || ph <= 0) return false;

    auto to_px = [&](double x) -> double {
        return static_cast<double>(p.left) + (x - g.vx0) / (g.vx1 - g.vx0) * pw;
    };
    auto to_py = [&](double y) -> double {
        return static_cast<double>(p.bottom) - (y - g.vy0) / (g.vy1 - g.vy0) * ph;
    };

    const double target_px = to_px(dx);
    const double target_py = to_py(dy);
    double best_dist2 = std::numeric_limits<double>::max();
    double best_x = dx;
    double best_y = dy;
    int best_ci = -1;

    if (g.freq_mode) {
        if (!ensure_current_spectrum() || g.spec.freqs.empty() || g.vx1 <= g.vx0 || g.vy1 <= g.vy0) return false;
        const auto& f = g.spec.freqs;
        std::size_t lo = static_cast<std::size_t>(std::lower_bound(f.begin(), f.end(), g.vx0) - f.begin());
        std::size_t hi = static_cast<std::size_t>(std::upper_bound(f.begin(), f.end(), g.vx1) - f.begin());
        if (lo >= hi) return false;
        for (std::size_t j = 0; j < g.spec.amp.size(); ++j) {
            const int ci = (j < g.spec_channel_indices.size()) ? g.spec_channel_indices[j] : -1;
            if (ci < 0 || !g.visible[ci]) continue;
            for (std::size_t k = lo; k < hi; ++k) {
                const double x = f[k];
                const double y = g.spec.amp[j][k];
                if (!std::isfinite(y)) continue;
                const double px = to_px(x);
                const double py = to_py(y);
                const double dxp = px - target_px;
                const double dyp = py - target_py;
                const double dist2 = dxp * dxp + dyp * dyp;
                if (dist2 < best_dist2) {
                    best_dist2 = dist2;
                    best_x = x;
                    best_y = y;
                    best_ci = ci;
                }
            }
        }
        if (best_ci >= 0) {
            dx = best_x;
            dy = best_y;
            if (out_channel) *out_channel = best_ci;
            return true;
        }
        return false;
    }
    if (!has_data()) return false;
    ensure_channel_formula_vectors();
    const auto& t = g.ds.time;
    if (t.empty() || g.vx1 <= g.vx0 || g.vy1 <= g.vy0) return false;
    std::size_t lo = static_cast<std::size_t>(std::lower_bound(t.begin(), t.end(), g.vx0) - t.begin());
    std::size_t hi = static_cast<std::size_t>(std::upper_bound(t.begin(), t.end(), g.vx1) - t.begin());
    if (lo >= hi) return false;
    for (std::size_t c = 0; c < g.ds.channel_count(); ++c) {
        if (!g.visible[c]) continue;
        for (std::size_t i = lo; i < hi; ++i) {
            const double y = rendered_channel_sample(c, i);
            if (!std::isfinite(y)) continue;
            const double x = t[i];
            const double px = to_px(x);
            const double py = to_py(y);
            const double dxp = px - target_px;
            const double dyp = py - target_py;
            const double dist2 = dxp * dxp + dyp * dyp;
            if (dist2 < best_dist2) {
                best_dist2 = dist2;
                best_x = x;
                best_y = y;
                best_ci = static_cast<int>(c);
            }
        }
    }
    if (best_ci >= 0) {
        dx = best_x;
        dy = best_y;
        if (out_channel) *out_channel = best_ci;
        return true;
    }
    return false;
}

void snap_to_nearest(double& dx, double& dy) {
    snap_to_nearest_target(dx, dy, nullptr);
}

int hit_test_marker(int px, int py) {
    if (!g.vvalid || g.markers.empty()) return -1;
    const RECT& p = g.vrect;
    if (px < p.left || px > p.right || py < p.top || py > p.bottom) return -1;
    if (g.vx1 <= g.vx0 || g.vy1 <= g.vy0) return -1;
    auto mx = [&](double dx) {
        return p.left + static_cast<int>((dx - g.vx0) / (g.vx1 - g.vx0) * (p.right - p.left));
    };
    auto my = [&](double dy) {
        return p.bottom - static_cast<int>((dy - g.vy0) / (g.vy1 - g.vy0) * (p.bottom - p.top));
    };
    int best = -1;
    int best_score = 999999;
    for (std::size_t i = 0; i < g.markers.size(); ++i) {
        const App::Marker& m = g.markers[i];
        if (m.freq != g.freq_mode) continue;
        const int dxp = std::abs(px - mx(m.x));
        if (dxp > 6) continue;
        int score = dxp * 10;
        if (m.snapped) {
            const int dyp = std::abs(py - my(m.y));
            if (dyp <= 8) score = dxp + dyp;
        }
        if (score < best_score) {
            best_score = score;
            best = static_cast<int>(i);
        }
    }
    return best;
}

const HotkeyBinding* find_hotkey_binding(int command) {
    for (const auto& hk : g.hotkeys)
        if (hk.command == command) return &hk;
    return nullptr;
}

std::vector<HotkeyBinding> default_hotkeys() {
    return {
        {IDC_OPEN, FVIRTKEY | FCONTROL, 'O'},
        {IDC_SAVEPNG, FVIRTKEY | FCONTROL, 'S'},
        {IDC_SAVECSV, FVIRTKEY | FCONTROL | FSHIFT, 'S'},
        {IDM_UNDO, FVIRTKEY | FCONTROL, 'Z'},
        {IDM_REDO, FVIRTKEY | FCONTROL | FSHIFT, 'Z'},
        {IDM_MODE_TIME, FVIRTKEY, 'T'},
        {IDM_MODE_FREQ, FVIRTKEY, 'F'},
        {IDC_MEASURE, FVIRTKEY, 'P'},
        {IDM_ADD_MARKER, FVIRTKEY, 'M'},
        {IDM_ADD_VLINE, FVIRTKEY, 'V'},
        {IDM_ADD_HLINE, FVIRTKEY, 'H'},
        {IDC_AUTOY, FVIRTKEY, 'A'},
        {IDM_VISMOOTH, FVIRTKEY, 'C'},
        {IDM_VPAN, FVIRTKEY, 'Y'},
        {IDM_THEME, FVIRTKEY, 'D'},
        {IDC_PLAY, FVIRTKEY, VK_SPACE},
        {IDC_ZOOMIN, FVIRTKEY, VK_OEM_PLUS},
        {IDC_ZOOMOUT, FVIRTKEY, VK_OEM_MINUS},
        {IDC_PANLEFT, FVIRTKEY, VK_LEFT},
        {IDC_PANRIGHT, FVIRTKEY, VK_RIGHT},
        {IDC_RESET, FVIRTKEY, VK_HOME},
        {IDC_GOTO_START, FVIRTKEY | FCONTROL, VK_HOME},
        {IDC_GOTO_END, FVIRTKEY | FCONTROL, VK_END},
        {IDM_CLEAR_POINTS, FVIRTKEY, VK_DELETE},
        {IDM_HOTKEYS, FVIRTKEY, VK_F1},
    };
}

void ensure_hotkeys_initialized() {
    if (g.hotkeys.empty()) g.hotkeys = default_hotkeys();
}

int read_ini_int(const wchar_t* section, const wchar_t* key, int def_value) {
    if (g_config_path.empty()) g_config_path = app_config_path();
    return static_cast<int>(GetPrivateProfileIntW(section, key, def_value, g_config_path.c_str()));
}

double read_ini_double(const wchar_t* section, const wchar_t* key, double def_value) {
    if (g_config_path.empty()) g_config_path = app_config_path();
    wchar_t buf[64]{};
    std::wstring def = format_edit_number(def_value);
    GetPrivateProfileStringW(section, key, def.c_str(), buf, 64, g_config_path.c_str());
    double value = def_value;
    if (parse_wide_double_text(buf, value)) return value;
    return def_value;
}

double read_ini_optional_double(const wchar_t* section, const wchar_t* key, double def_value) {
    if (g_config_path.empty()) g_config_path = app_config_path();
    wchar_t buf[64]{};
    GetPrivateProfileStringW(section, key, L"", buf, 64, g_config_path.c_str());
    double value = def_value;
    if (parse_wide_double_text(buf, value)) return value;
    return def_value;
}

std::wstring trim_wide_ascii(const std::wstring& text) {
    const wchar_t* ws = L" \t\r\n\f\v";
    const std::size_t begin = text.find_first_not_of(ws);
    if (begin == std::wstring::npos) return L"";
    const std::size_t end = text.find_last_not_of(ws);
    return text.substr(begin, end - begin + 1);
}

std::wstring read_ini_wstring(const wchar_t* section, const wchar_t* key, const std::wstring& def_value) {
    if (g_config_path.empty()) g_config_path = app_config_path();
    wchar_t buf[128]{};
    GetPrivateProfileStringW(section, key, def_value.c_str(), buf, 128, g_config_path.c_str());
    return trim_wide_ascii(buf);
}

std::wstring normalize_axis_label_text(const std::wstring& text, const wchar_t* fallback) {
    std::wstring out = trim_wide_ascii(text);
    if (out.empty()) out = fallback ? fallback : L"";
    return out;
}

void write_ini_double(const wchar_t* section, const wchar_t* key, double value) {
    if (g_config_path.empty()) g_config_path = app_config_path();
    std::wstring text = format_edit_number(value);
    WritePrivateProfileStringW(section, key, text.c_str(), g_config_path.c_str());
}

void write_ini_optional_double(const wchar_t* section, const wchar_t* key, double value) {
    if (g_config_path.empty()) g_config_path = app_config_path();
    if (!std::isfinite(value)) {
        WritePrivateProfileStringW(section, key, L"", g_config_path.c_str());
        return;
    }
    std::wstring text = format_edit_number(value);
    WritePrivateProfileStringW(section, key, text.c_str(), g_config_path.c_str());
}

void load_channel_formulas_from_ini() {
    ensure_channel_formula_storage();
    wchar_t global_buf[512]{};
    GetPrivateProfileStringW(L"transform", L"global_formula", default_channel_formula_text().c_str(), global_buf, 512, g_config_path.c_str());
    g.global_formula = normalize_formula_text(global_buf);
    g.global_formula_rpn.clear();
    std::wstring global_error;
    if (!compile_formula_rpn(g.global_formula, g.global_formula_rpn, global_error, g_str == &kEn)) {
        g.global_formula = default_channel_formula_text();
        g.global_formula_rpn.clear();
        compile_formula_rpn(g.global_formula, g.global_formula_rpn, global_error, g_str == &kEn);
    }
    for (std::size_t i = 0; i < g.channel_formulas.size(); ++i) {
        wchar_t key_name[32]{};
        swprintf(key_name, 32, L"formula_%u", static_cast<unsigned int>(i));
        wchar_t buf[512]{};
        GetPrivateProfileStringW(L"transform", key_name, default_channel_formula_text().c_str(), buf, 512, g_config_path.c_str());
        g.channel_formulas[i] = normalize_formula_text(buf);
        g.channel_formula_rpn[i].clear();
        std::wstring error;
        if (!compile_formula_rpn(g.channel_formulas[i], g.channel_formula_rpn[i], error, g_str == &kEn)) {
            g.channel_formulas[i] = default_channel_formula_text();
            g.channel_formula_rpn[i].clear();
            compile_formula_rpn(g.channel_formulas[i], g.channel_formula_rpn[i], error, g_str == &kEn);
        }
    }
    invalidate_formula_runtime();
    ensure_channel_formula_vectors();
}

void load_runtime_settings() {
    if (g_config_path.empty()) g_config_path = app_config_path();

    wchar_t lang_buf[16]{};
    GetPrivateProfileStringW(L"ui", L"language", L"ru", lang_buf, 16, g_config_path.c_str());
    g_str = (lstrcmpiW(lang_buf, L"en") == 0) ? &kEn : &kRu;

    g.visual_smooth = read_ini_int(L"ui", L"smoothing", g.visual_smooth ? 1 : 0) != 0;
    g.vertical_pan = read_ini_int(L"ui", L"vertical_pan", g.vertical_pan ? 1 : 0) != 0;
    g.snap_to_data = read_ini_int(L"ui", L"snap_to_data", g.snap_to_data ? 1 : 0) != 0;
    g.show_gap_markers = read_ini_int(L"ui", L"show_gap_markers", g.show_gap_markers ? 1 : 0) != 0;
    g.noise_threshold_enabled = read_ini_int(L"ui", L"filter_enabled", g.noise_threshold_enabled ? 1 : 0) != 0;
    g.noise_threshold_mode = read_ini_int(L"ui", L"filter_mode", g.noise_threshold_mode);
    g.noise_threshold_topology = read_ini_int(L"ui", L"filter_topology", g.noise_threshold_topology);
    g.noise_threshold_min = read_ini_optional_double(L"ui", L"filter_low_cutoff", g.noise_threshold_min);
    g.noise_threshold_max = read_ini_optional_double(L"ui", L"filter_high_cutoff", g.noise_threshold_max);
    g.noise_threshold_mode = std::clamp(g.noise_threshold_mode,
                                        static_cast<int>(FilterModeLowPass),
                                        static_cast<int>(FilterModeBandStop));
    g.noise_threshold_topology = std::clamp(g.noise_threshold_topology,
                                            static_cast<int>(FilterTopologyButterworth),
                                            static_cast<int>(FilterTopologyLinkwitzRiley));
    normalize_filter_bounds();
    g.side_panel_visible = read_ini_int(L"ui", L"side_panel_visible", g.side_panel_visible ? 1 : 0) != 0;
    g.side_panel_tab = read_ini_int(L"ui", L"side_panel_tab", g.side_panel_tab);
    if (g.side_panel_tab < 0 || g.side_panel_tab > 2) g.side_panel_tab = 0;
    g.play_speed = read_ini_double(L"ui", L"play_speed", g.play_speed);
    if (!(g.play_speed > 0.0) || !std::isfinite(g.play_speed)) g.play_speed = 1.0;
    g.light_mode = read_ini_int(L"ui", L"light_mode", g.light_mode ? 1 : 0) != 0;
    g.light_mode_open_start = read_ini_double(L"ui", L"light_mode_open_start", g.light_mode_open_start);
    g.light_mode_open_end = read_ini_double(L"ui", L"light_mode_open_end", g.light_mode_open_end);
    if (!std::isfinite(g.light_mode_open_start) || g.light_mode_open_start < 0.0) g.light_mode_open_start = 0.0;
    if (!std::isfinite(g.light_mode_open_end) || g.light_mode_open_end <= g.light_mode_open_start) {
        g.light_mode_open_end = g.light_mode_open_start + 10.0;
    }

    g.pdisp.number = read_ini_int(L"points", L"number", g.pdisp.number ? 1 : 0) != 0;
    g.pdisp.x = read_ini_int(L"points", L"x", g.pdisp.x ? 1 : 0) != 0;
    g.pdisp.y = read_ini_int(L"points", L"y", g.pdisp.y ? 1 : 0) != 0;
    g.pdisp.dx = read_ini_int(L"points", L"dx", g.pdisp.dx ? 1 : 0) != 0;
    g.pdisp.dy = read_ini_int(L"points", L"dy", g.pdisp.dy ? 1 : 0) != 0;
    g.pdisp.inv_dt = read_ini_int(L"points", L"inv_dt", g.pdisp.inv_dt ? 1 : 0) != 0;
    g.pdisp.dist = read_ini_int(L"points", L"dist", g.pdisp.dist ? 1 : 0) != 0;
    g.axis_x_label = normalize_axis_label_text(
        read_ini_wstring(L"ui", L"axis_x_label", read_ini_wstring(L"points", L"x_label", g.axis_x_label)), L"X");
    g.axis_y_label = normalize_axis_label_text(
        read_ini_wstring(L"ui", L"axis_y_label", read_ini_wstring(L"points", L"y_label", g.axis_y_label)), L"Y");

    g.marker_color = static_cast<COLORREF>(read_ini_int(
        L"ui", L"marker_color", static_cast<int>(g_theme->marker_color)));

    g.hotkeys = default_hotkeys();
    for (auto& hk : g.hotkeys) {
        wchar_t key_name[32]{};
        swprintf(key_name, 32, L"cmd_%d", hk.command);
        wchar_t value[64]{};
        GetPrivateProfileStringW(L"hotkeys", key_name, L"", value, 64, g_config_path.c_str());
        if (!value[0]) continue;
        unsigned int fvirt = hk.fvirt;
        unsigned int key = hk.key;
        if (swscanf(value, L"%u,%u", &fvirt, &key) == 2) {
            hk.fvirt = static_cast<BYTE>(fvirt);
            hk.key = static_cast<WORD>(key);
        }
    }
    load_recent_files_from_ini();
}

std::wstring key_name(WORD key) {
    switch (key) {
        case 0: return g_str == &kEn ? L"None" : L"Нет";
        case VK_TAB: return g_str == &kEn ? L"Tab" : L"Tab";
        case VK_BACK: return g_str == &kEn ? L"Backspace" : L"Backspace";
        case VK_RETURN: return g_str == &kEn ? L"Enter" : L"Enter";
        case VK_INSERT: return g_str == &kEn ? L"Insert" : L"Insert";
        case VK_PRIOR: return g_str == &kEn ? L"Page Up" : L"Page Up";
        case VK_NEXT: return g_str == &kEn ? L"Page Down" : L"Page Down";
        case VK_SPACE: return g_str == &kEn ? L"Space" : L"Пробел";
        case VK_LEFT: return g_str == &kEn ? L"Left" : L"Влево";
        case VK_RIGHT: return g_str == &kEn ? L"Right" : L"Вправо";
        case VK_UP: return g_str == &kEn ? L"Up" : L"Вверх";
        case VK_DOWN: return g_str == &kEn ? L"Down" : L"Вниз";
        case VK_HOME: return L"Home";
        case VK_END: return L"End";
        case VK_DELETE: return L"Delete";
        case VK_ESCAPE: return L"Esc";
        case VK_PAUSE: return g_str == &kEn ? L"Pause" : L"Pause";
        case VK_CAPITAL: return g_str == &kEn ? L"Caps Lock" : L"Caps Lock";
        case VK_NUMLOCK: return g_str == &kEn ? L"Num Lock" : L"Num Lock";
        case VK_SCROLL: return g_str == &kEn ? L"Scroll Lock" : L"Scroll Lock";
        case VK_SNAPSHOT: return g_str == &kEn ? L"Print Screen" : L"Print Screen";
        case VK_APPS: return g_str == &kEn ? L"Menu" : L"Menu";
        case VK_OEM_PLUS: return L"+";
        case VK_OEM_MINUS: return L"-";
        default:
            if (key >= VK_F13 && key <= VK_F24) return L"F" + std::to_wstring(key - VK_F1 + 1);
            if (key >= 'A' && key <= 'Z') return std::wstring(1, static_cast<wchar_t>(key));
            if (key >= '0' && key <= '9') return std::wstring(1, static_cast<wchar_t>(key));
            if (key >= VK_F1 && key <= VK_F12) return L"F" + std::to_wstring(key - VK_F1 + 1);
            wchar_t buf[16]{};
            swprintf(buf, 16, L"VK_%04X", static_cast<unsigned int>(key & 0xFFFFu));
            return buf;
    }
}

// Dialog and settings helpers live in a separate include to keep this file manageable.
#include "gui_settings_hotkeys.cpp"

void open_settings() {
    if (!g.settings_wnd) {
        HINSTANCE inst = reinterpret_cast<HINSTANCE>(GetWindowLongPtr(g.main, GWLP_HINSTANCE));
            g.settings_wnd = CreateWindowExW(
            WS_EX_TOOLWINDOW, L"LvmPtSettings", settings_window_title(),
            WS_POPUP | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, 540, 536,
            g.main, nullptr, inst, nullptr);
        if (!g.settings_wnd) return;
        RECT mr, sr;
        GetWindowRect(g.main, &mr);
        GetWindowRect(g.settings_wnd, &sr);
        const int sw = sr.right - sr.left, sh = sr.bottom - sr.top;
        SetWindowPos(g.settings_wnd, nullptr,
                     mr.left + ((mr.right - mr.left) - sw) / 2,
                     mr.top + ((mr.bottom - mr.top) - sh) / 2,
                     0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }
    refresh_settings_controls();
    ShowWindow(g.settings_wnd, SW_SHOW);
    SetForegroundWindow(g.settings_wnd);
}

#include "gui_welcome.cpp"

// ---- UI rebuild (language switch) --------------------------------------
void rebuild_ui() {
    if (!g.main) return;
    finish_channel_rename(true);
    rebuild_menu_bar();
    
    // Update buttons
    SetWindowTextW(g.open, g_str->btn_open);
    SetWindowTextW(g.savepng, g_str->btn_png);
    SetWindowTextW(g.savecsv, g_str->btn_csv);
    SetWindowTextW(g.mode_time, g_str->st_time);
    SetWindowTextW(g.mode_freq, g_str->st_hz);
    SetWindowTextW(g.play, g.playing ? g_str->btn_pause : g_str->btn_play);
    SetWindowTextW(g.measure, g_str->btn_measure);
    SetWindowTextW(g.marker_btn, g_str == &kEn ? L"Marker" : L"Маркер");
    SetWindowTextW(g.vline_btn, g_str == &kEn ? L"V-Line" : L"V-линия");
    SetWindowTextW(g.hline_btn, g_str == &kEn ? L"H-Line" : L"H-линия");
    SetWindowTextW(g.reset, g_str->btn_reset);
    SetWindowTextW(g.autoy, g_str->btn_autoy);
    SetWindowTextW(g.ptsettings, settings_button_text());
    SetWindowTextW(g.sidepanel_btn, side_panel_button_text());
    SetWindowTextW(g.show_all_btn, channel_show_all_text());
    SetWindowTextW(g.hide_all_btn, channel_hide_all_text());
    refresh_side_panel_controls();
    
    // Update welcome window if visible
    if (g.welcome_wnd && IsWindowVisible(g.welcome_wnd)) {
        HINSTANCE inst = reinterpret_cast<HINSTANCE>(GetWindowLongPtr(g.main, GWLP_HINSTANCE));
        DestroyWindow(g.welcome_wnd);
        g.welcome_wnd = nullptr;
        show_welcome(inst);
    }
    
    if (g.settings_wnd) {
        bool was_visible = IsWindowVisible(g.settings_wnd) != FALSE;
        DestroyWindow(g.settings_wnd);
        g.settings_wnd = nullptr;
        if (was_visible) open_settings();
    }
    
    // Update main window title
    if (!g.file_name.empty()) {
        SetWindowTextW(g.main, (std::wstring(g_str->app_title) + L" — " + g.file_name).c_str());
    } else {
        SetWindowTextW(g.main, g_str->app_title);
    }
    
    InvalidateRect(g.main, nullptr, TRUE);
    set_status();
}

// ---- window procedure ----------------------------------------------------

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CREATE: {
            HINSTANCE inst = reinterpret_cast<LPCREATESTRUCT>(lp)->hInstance;

            // Modern UI font (falls back to the stock font if unavailable).
            g.ui_font = CreateFontW(-15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            NONCLIENTMETRICSW ncm{};
            ncm.cbSize = sizeof(ncm);
            if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0)) {
                g.menu_font = CreateFontIndirectW(&ncm.lfMenuFont);
            }
            if (!g.menu_font) {
                g.menu_font = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                          CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            }
            g.bold_font = CreateFontW(-15, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                      CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            g.title_font = CreateFontW(-30, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            if (!g.ui_font) g.ui_font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
            HFONT font = g.ui_font;

            g.menu = make_menu();
            SetMenu(hwnd, g.menu);
            MENUINFO mi{};
            mi.cbSize = sizeof(mi);
            mi.fMask = MIM_BACKGROUND | MIM_APPLYTOSUBMENUS;
            mi.hbrBack = CreateSolidBrush(g_theme->bg_toolbar);
            SetMenuInfo(g.menu, &mi);

            auto mk = [&](const wchar_t* text, int id, DWORD extra, bool in_toolbar = true) {
                HWND b = CreateWindowExW(0, L"BUTTON", text, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | BS_OWNERDRAW | extra,
                                         0, 0, 10, 10, hwnd,
                                         reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), inst, nullptr);
                SendMessageW(b, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
                if (in_toolbar) g.buttons.push_back(b);
                else ShowWindow(b, SW_HIDE);
                return b;
            };
            g.open = mk(g_str->btn_open, IDC_OPEN, 0);
            g.savepng = mk(g_str->btn_png, IDC_SAVEPNG, 0, false);
            g.savecsv = mk(g_str->btn_csv, IDC_SAVECSV, 0, false);
            g.mode_time = mk(g_str->st_time, IDM_MODE_TIME, 0);
            g.mode_freq = mk(g_str->st_hz, IDM_MODE_FREQ, 0);
            g.play = mk(g_str->btn_play, IDC_PLAY, 0);
            g.measure = mk(g_str->btn_measure, IDC_MEASURE, 0);
            g.marker_btn = mk(g_str == &kEn ? L"Marker" : L"Маркер", IDM_ADD_MARKER, 0);
            g.vline_btn = mk(g_str == &kEn ? L"V-Line" : L"V-линия", IDM_ADD_VLINE, 0);
            g.hline_btn = mk(g_str == &kEn ? L"H-Line" : L"H-линия", IDM_ADD_HLINE, 0);
            g.reset = mk(g_str->btn_reset, IDC_RESET, 0);
            g.autoy = mk(g_str->btn_autoy, IDC_AUTOY, 0);
            g.ptsettings = mk(settings_button_text(), IDC_PTSETTINGS, 0);
            g.sidepanel_btn = mk(side_panel_button_text(), IDC_SIDEPANEL, 0);
            g.show_all_btn = mk(channel_show_all_text(), IDC_SHOW_ALL, 0);
            g.hide_all_btn = mk(channel_hide_all_text(), IDC_HIDE_ALL, 0);

            auto mk_panel_btn = [&](const wchar_t* text, int id, std::vector<HWND>& bucket) {
                HWND b = mk(text, id, 0);
                bucket.push_back(b);
                return b;
            };
            auto mk_panel_ctl = [&](const wchar_t* cls, const wchar_t* text, DWORD style, int id, std::vector<HWND>& bucket) {
                HWND c = CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | style,
                                         0, 0, 10, 10, hwnd,
                                         reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), inst, nullptr);
                if (c) {
                    SendMessageW(c, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
                    bucket.push_back(c);
                }
                return c;
            };

            g.side_tab_channels = mk(side_tab_channels_text(), IDC_SIDE_TAB_CHANNELS, 0);
            g.side_tab_points = mk(side_tab_points_text(), IDC_SIDE_TAB_POINTS, 0);
            g.side_tab_filter = mk(side_tab_filter_text(), IDC_SIDE_TAB_FILTER, 0);
            g.side_channel_hint = mk_panel_ctl(L"STATIC", side_channel_hint_text(),
                                               SS_LEFT | SS_NOPREFIX, IDC_SIDE_CHANNEL_HINT, g.side_filter_controls);
            g.side_filter_enable = mk_panel_ctl(L"BUTTON", filter_toggle_text(),
                                                BS_OWNERDRAW, IDC_SIDE_FILTER_ENABLE, g.side_filter_controls);
            g.side_filter_mode_label = mk_panel_ctl(L"STATIC", filter_mode_label_text(),
                                                    SS_LEFT | SS_NOPREFIX, 0, g.side_filter_controls);
            g.side_filter_mode = mk_panel_ctl(L"COMBOBOX", L"",
                                              CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VSCROLL | WS_BORDER | WS_TABSTOP,
                                              IDC_SIDE_FILTER_MODE, g.side_filter_controls);
            g.side_filter_topology_label = mk_panel_ctl(L"STATIC", filter_topology_label_text(),
                                                        SS_LEFT | SS_NOPREFIX, 0, g.side_filter_controls);
            g.side_filter_topology = mk_panel_ctl(L"COMBOBOX", L"",
                                                  CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VSCROLL | WS_BORDER | WS_TABSTOP,
                                                  IDC_SIDE_FILTER_TOPOLOGY, g.side_filter_controls);
            g.side_filter_low_label = mk_panel_ctl(L"STATIC", filter_low_cutoff_text(),
                                                   SS_LEFT | SS_NOPREFIX, 0, g.side_filter_controls);
            g.side_filter_low_value = mk_panel_ctl(L"STATIC", L"", SS_RIGHT | SS_NOPREFIX, 0, g.side_filter_controls);
            g.side_filter_low_track = mk_panel_ctl(TRACKBAR_CLASS, L"", TBS_HORZ | TBS_NOTICKS | WS_TABSTOP,
                                                   IDC_SIDE_FILTER_LOW_TRACK, g.side_filter_controls);
            g.side_filter_high_label = mk_panel_ctl(L"STATIC", filter_high_cutoff_text(),
                                                    SS_LEFT | SS_NOPREFIX, 0, g.side_filter_controls);
            g.side_filter_high_value = mk_panel_ctl(L"STATIC", L"", SS_RIGHT | SS_NOPREFIX, 0, g.side_filter_controls);
            g.side_filter_high_track = mk_panel_ctl(TRACKBAR_CLASS, L"", TBS_HORZ | TBS_NOTICKS | WS_TABSTOP,
                                                    IDC_SIDE_FILTER_HIGH_TRACK, g.side_filter_controls);
            g.side_global_formula_label = mk_panel_ctl(L"STATIC", side_global_formula_label_text(), SS_LEFT, 0, g.side_channel_controls);
            g.side_global_formula_edit = mk_panel_ctl(L"EDIT", default_channel_formula_text().c_str(),
                                                      WS_BORDER | ES_AUTOHSCROLL, IDC_SIDE_GLOBAL_FORMULA_EDIT, g.side_channel_controls);
            g.side_global_formula_apply = mk_panel_btn(side_global_formula_apply_text(), IDC_SIDE_GLOBAL_FORMULA_APPLY, g.side_channel_controls);
            g.side_channel_separator = mk_panel_ctl(L"STATIC", L"", SS_ETCHEDHORZ, 0, g.side_channel_controls);
            g.side_channel_formula_label = mk_panel_ctl(L"STATIC", side_channel_formula_label_text(), SS_LEFT, 0, g.side_channel_controls);
            g.side_formula_edit = mk_panel_ctl(L"EDIT", default_channel_formula_text().c_str(),
                                               WS_BORDER | ES_AUTOHSCROLL, IDC_SIDE_FORMULA_EDIT, g.side_channel_controls);
            g.side_channel_color = mk_panel_btn(side_channel_color_button_text(), IDC_SIDE_CHANNEL_COLOR, g.side_channel_controls);
            g.side_formula_apply_selected = mk_panel_btn(side_formula_apply_selected_text(), IDC_SIDE_FORMULA_APPLY_SELECTED, g.side_channel_controls);
            g.side_formula_apply_visible = mk_panel_btn(side_formula_apply_visible_text(), IDC_SIDE_FORMULA_APPLY_VISIBLE, g.side_channel_controls);
            g.side_formula_reset_selected = mk_panel_btn(side_formula_reset_selected_text(), IDC_SIDE_FORMULA_RESET_SELECTED, g.side_channel_controls);
            g.side_formula_reset_all = mk_panel_btn(side_formula_reset_all_text(), IDC_SIDE_FORMULA_RESET_ALL, g.side_channel_controls);

            const struct PointToggleSeed { int id; const wchar_t* text; bool on; } point_toggle_seeds[] = {
                {IDC_SIDE_PT_NUM, side_pt_num_text(), g.pdisp.number},
                {IDC_SIDE_PT_X, side_pt_x_text(), g.pdisp.x},
                {IDC_SIDE_PT_Y, side_pt_y_text(), g.pdisp.y},
                {IDC_SIDE_PT_DX, side_pt_dx_text(), g.pdisp.dx},
                {IDC_SIDE_PT_DY, side_pt_dy_text(), g.pdisp.dy},
                {IDC_SIDE_PT_INVDT, side_pt_invdt_text(), g.pdisp.inv_dt},
                {IDC_SIDE_PT_DIST, side_pt_dist_text(), g.pdisp.dist},
                {IDC_SIDE_PT_SNAP, side_pt_snap_text(), g.snap_to_data},
            };
            for (const auto& seed : point_toggle_seeds) {
                HWND c = mk_panel_ctl(L"BUTTON", seed.text, BS_OWNERDRAW, seed.id, g.side_point_controls);
                if (c) set_toggle_checked(c, seed.on);
            }
            g.side_point_color_current = mk_panel_btn(point_current_color_button_text(), IDC_SIDE_POINT_COLOR_CURRENT, g.side_point_controls);
            g.side_point_label_groups = mk_panel_ctl(L"STATIC", point_group_list_title(), SS_LEFT, 0, g.side_point_controls);
            g.side_point_group_list = mk_panel_ctl(L"LISTBOX", L"", LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_VSCROLL | WS_BORDER, IDC_SIDE_POINT_GROUP_LIST, g.side_point_controls);
            g.side_point_group_visible = mk_panel_ctl(L"BUTTON", point_group_visible_text(), BS_OWNERDRAW, IDC_SIDE_POINT_GROUP_VISIBLE, g.side_point_controls);
            g.side_point_group_color = mk_panel_btn(point_selected_group_color_button_text(), IDC_SIDE_POINT_GROUP_COLOR, g.side_point_controls);
            g.side_point_group_new = mk_panel_btn(point_group_new_button_text(), IDC_SIDE_POINT_GROUP_NEW, g.side_point_controls);
            g.side_point_group_delete = mk_panel_btn(side_point_group_delete_text(), IDC_SIDE_POINT_GROUP_DELETE, g.side_point_controls);
            g.side_point_group_name = mk_panel_ctl(L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL, IDC_SIDE_POINT_GROUP_NAME, g.side_point_controls);
            g.side_point_group_rename = mk_panel_btn(side_point_group_rename_text(), IDC_SIDE_POINT_GROUP_RENAME, g.side_point_controls);

            g.status = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE,
                                       0, 0, 10, 10, hwnd, nullptr, inst, nullptr);
            SendMessageW(g.status, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
            ShowWindow(g.status, SW_HIDE);   // owner-drawn in on_paint

            g.axis_font = CreateFontW(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                      CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            enable_file_drop_support(hwnd);
            SetTimer(hwnd, 2, 50, nullptr);   // hover tracking timer
            update_theme_brushes();
            sync_menu();
            refresh_side_panel_controls();
            set_status();
            return 0;
        }
        case WM_SIZE:
            layout();
            if (welcome_visible()) {
                RECT rc;
                GetClientRect(hwnd, &rc);
                SetWindowPos(g.welcome_wnd, HWND_TOP, 0, 0, rc.right, rc.bottom, SWP_SHOWWINDOW);
                InvalidateRect(g.welcome_wnd, nullptr, FALSE);
                return 0;
            }
            RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
            return 0;
        case WM_GETMINMAXINFO: {
            MINMAXINFO* m = reinterpret_cast<MINMAXINFO*>(lp);
            m->ptMinTrackSize.x = 980;
            m->ptMinTrackSize.y = 560;
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_PARENTNOTIFY:
            if (LOWORD(wp) == WM_LBUTTONDOWN) {
                finish_channel_rename_if_click_outside(hwnd);
            }
            break;
        case WM_CTLCOLORBTN: {
            HDC dc = reinterpret_cast<HDC>(wp);
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, g_theme->text_primary);
            SelectObject(dc, g.ui_font);
            return reinterpret_cast<LRESULT>(g_panel_brush);
        }
        case WM_CTLCOLORSTATIC: {
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
        case WM_CTLCOLOREDIT: {
            HDC dc = reinterpret_cast<HDC>(wp);
            SetBkColor(dc, g_theme->bg_plot);
            SetTextColor(dc, g_theme->text_primary);
            return reinterpret_cast<LRESULT>(g_input_brush ? g_input_brush : g_panel_brush);
        }
        case WM_MEASUREITEM: {
            MEASUREITEMSTRUCT* mis = reinterpret_cast<MEASUREITEMSTRUCT*>(lp);
            if (mis && mis->CtlType == ODT_MENU) {
                measure_owner_draw_menu(mis);
                return TRUE;
            }
            if (mis && mis->CtlType == ODT_COMBOBOX &&
                (mis->CtlID == IDC_SIDE_FILTER_MODE || mis->CtlID == IDC_SIDE_FILTER_TOPOLOGY)) {
                measure_settings_combo_item(mis);
                return TRUE;
            }
            break;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            on_paint(hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* dis = reinterpret_cast<DRAWITEMSTRUCT*>(lp);
            if (dis && dis->CtlType == ODT_MENU) {
                draw_owner_draw_menu(dis);
                return TRUE;
            }
            if (dis && dis->CtlType == ODT_COMBOBOX &&
                (dis->CtlID == IDC_SIDE_FILTER_MODE || dis->CtlID == IDC_SIDE_FILTER_TOPOLOGY)) {
                draw_settings_combo_item(dis);
                return TRUE;
            }
            HWND btn = dis->hwndItem;
            if (!btn) break;
            HDC dc = dis->hDC;
            const int ctl_id = GetDlgCtrlID(btn);
            if (is_channel_checkbox_id(ctl_id) || is_side_toggle_id(ctl_id)) {
                wchar_t txt[128]{};
                GetWindowTextW(btn, txt, 128);
                const bool pressed = (dis->itemState & ODS_SELECTED) != 0;
                const bool enabled = IsWindowEnabled(btn) != FALSE;
                const bool compact = is_channel_checkbox_id(ctl_id);
                draw_themed_check_control(dc, dis->rcItem, txt,
                    is_toggle_checked(btn), pressed, enabled, false, compact);
                return TRUE;
            }
            RECT r = dis->rcItem;
            bool pressed = (dis->itemState & ODS_SELECTED) != 0;
            bool active = false;
            if (btn == g.measure) {
                active = g.measure_mode;
            } else if (btn == g.autoy) {
                active = g.auto_y;
            } else if (btn == g.mode_time) {
                active = !g.freq_mode;
            } else if (btn == g.mode_freq) {
                active = g.freq_mode;
            } else if (btn == g.marker_btn) {
                active = g.pending_marker;
            } else if (btn == g.vline_btn) {
                active = g.pending_line == 1;
            } else if (btn == g.hline_btn) {
                active = g.pending_line == 2;
            } else if (btn == g.sidepanel_btn) {
                active = g.side_panel_visible;
            } else if (btn == g.side_tab_channels) {
                active = g.side_panel_tab == 0;
            } else if (btn == g.side_tab_points) {
                active = g.side_panel_tab == 1;
            } else if (btn == g.side_tab_filter) {
                active = g.side_panel_tab == 2;
            }
            bool hover = (btn == g.hovered_btn);
            wchar_t txt[128];
            GetWindowTextW(btn, txt, 128);
            draw_themed_button(dc, r, txt, pressed, active, hover);
            return TRUE;
        }
        case WM_HSCROLL: {
            HWND ctl = reinterpret_cast<HWND>(lp);
            if (!ctl || g.updating_noise_threshold_edits) return 0;
            const int ctl_id = GetDlgCtrlID(ctl);
            if (ctl_id != IDC_SIDE_FILTER_LOW_TRACK && ctl_id != IDC_SIDE_FILTER_HIGH_TRACK) return 0;
            const double nyquist = current_filter_nyquist();
            if (!(nyquist > 0.0)) return 0;
            if (ctl_id == IDC_SIDE_FILTER_LOW_TRACK) {
                g.noise_threshold_min = filter_slider_to_frequency(static_cast<int>(SendMessageW(ctl, TBM_GETPOS, 0, 0)), nyquist);
            } else {
                g.noise_threshold_max = filter_slider_to_frequency(static_cast<int>(SendMessageW(ctl, TBM_GETPOS, 0, 0)), nyquist);
            }
            normalize_filter_bounds();
            recompute_transforms_from_state();
            save_runtime_settings();
            refresh_side_panel_controls();
            set_status();
            InvalidateRect(hwnd, nullptr, TRUE);
            return 0;
        }
        case WM_TIMER:
            if (LOWORD(wp) == kRuntimeSettingsSaveTimerId) {
                flush_runtime_settings_save();
                return 0;
            }
            if (LOWORD(wp) == 2) {
                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(hwnd, &pt);
                HWND new_hover = nullptr;
                for (HWND h : g.buttons) {
                    RECT r;
                    GetWindowRect(h, &r);
                    MapWindowPoints(nullptr, hwnd, (LPPOINT)&r, 2);
                    if (PtInRect(&r, pt)) { new_hover = h; break; }
                }
                if (new_hover != g.hovered_btn) {
                    if (g.hovered_btn) InvalidateRect(g.hovered_btn, nullptr, FALSE);
                    if (new_hover) InvalidateRect(new_hover, nullptr, FALSE);
                    g.hovered_btn = new_hover;
                    // Update hover status description and invalidate status bar
                    g.hover_status_text = toolbar_hover_text(new_hover);
                    RECT rc; GetClientRect(hwnd, &rc);
                    RECT sr = {0, rc.bottom - kBottomBar, rc.right, rc.bottom};
                    InvalidateRect(hwnd, &sr, FALSE);
                }
                return 0;
            }
            if (g.playing && !g.freq_mode && has_data()) {
                // Real-time playhead: 1 s of signal per 1 s of wall-clock time.
                LARGE_INTEGER now, freq;
                QueryPerformanceCounter(&now);
                QueryPerformanceFrequency(&freq);
                const double elapsed =
                    static_cast<double>(now.QuadPart - g.play_anchor_qpc.QuadPart) /
                    static_cast<double>(freq.QuadPart);
                g.playhead = g.play_anchor_data + elapsed * g.play_speed;
                if (g.playhead >= g.data_t1) {
                    g.playhead = g.data_t1;
                    stop_play();
                } else {
                    // Once the playhead passes the middle, keep it centred by
                    // scrolling a little each frame � smooth, no big jumps.
                    const double span = g.win_end - g.win_start;
                    if (g.playhead > g.win_start + span * 0.5) {
                        g.win_start = g.playhead - span * 0.5;
                        g.win_end = g.win_start + span;
                        if (g.win_end > g.data_t1) { g.win_end = g.data_t1; g.win_start = g.win_end - span; }
                        if (g.win_start < g.data_t0) { g.win_start = g.data_t0; g.win_end = g.win_start + span; }
                    }
                }
                set_status();
                RECT pr = plot_rect();
                RECT rc; GetClientRect(hwnd, &rc);
                pr.bottom = rc.bottom; // include status bar
                InvalidateRect(hwnd, &pr, FALSE);
            }
            return 0;
        case WM_COMMAND: {
            const int id = LOWORD(wp);
            switch (id) {
                case IDC_OPEN: open_file(); return 0;
                case IDC_SAVEPNG: save_png_dialog(); return 0;
                case IDC_SAVECSV: save_as_dialog(); return 0;
                case IDM_EXIT: DestroyWindow(hwnd); return 0;
                case IDC_MODE:
                    set_mode(!g.freq_mode);
                    return 0;
                case IDM_MODE_TIME:
                    set_mode(false);
                    return 0;
                case IDM_MODE_FREQ:
                    set_mode(true);
                    return 0;
                case IDC_PLAY: toggle_play(); return 0;
                case IDC_SHOW_ALL:
                {
                    const SettingsSnapshot before = capture_settings_snapshot();
                    set_all_channels_visible(true);
                    record_settings_change(before);
                    InvalidateRect(hwnd, nullptr, TRUE);
                    return 0;
                }
                case IDC_HIDE_ALL:
                {
                    const SettingsSnapshot before = capture_settings_snapshot();
                    set_all_channels_visible(false);
                    record_settings_change(before);
                    InvalidateRect(hwnd, nullptr, TRUE);
                    return 0;
                }
                case IDC_MEASURE:
                    g.measure_mode = !g.measure_mode;
                    if (g.measure_mode) g.pending_line = 0;
                    if (g.measure_mode) g.pending_marker = false;
                    SendMessageW(g.measure, BM_SETCHECK,
                                 g.measure_mode ? BST_CHECKED : BST_UNCHECKED, 0);
                    InvalidateRect(g.measure, nullptr, FALSE);
                    sync_menu();
                    set_status();
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                case IDC_PTSETTINGS: open_settings(); return 0;
                case IDC_SIDEPANEL:
                    g.side_panel_visible = !g.side_panel_visible;
                    save_runtime_settings();
                    layout();
                    set_status();
                    redraw_window_with_children(hwnd);
                    return 0;
                case IDC_SIDE_TAB_CHANNELS:
                    g.side_scroll_y = 0;
                    set_side_panel_tab(0);
                    save_runtime_settings();
                    layout();
                    redraw_window_with_children(hwnd);
                    return 0;
                case IDC_SIDE_TAB_POINTS:
                    g.side_scroll_y = 0;
                    set_side_panel_tab(1);
                    save_runtime_settings();
                    layout();
                    redraw_window_with_children(hwnd);
                    return 0;
                case IDC_SIDE_TAB_FILTER:
                    g.side_scroll_y = 0;
                    set_side_panel_tab(2);
                    save_runtime_settings();
                    layout();
                    redraw_window_with_children(hwnd);
                    return 0;
                case IDC_SIDE_GLOBAL_FORMULA_EDIT:
                case IDC_SIDE_FORMULA_EDIT:
                    if (HIWORD(wp) == EN_SETFOCUS && g.formula_ini_deferred) {
                        ensure_channel_formulas_loaded();
                        load_side_transform_controls();
                    }
                    return 0;
                case IDC_SIDE_GLOBAL_FORMULA_APPLY: {
                    if (!has_data()) return 0;
                    const SettingsSnapshot before = capture_settings_snapshot();
                    std::wstring formula;
                    std::wstring error;
                    std::vector<FormulaToken> compiled;
                    if (!read_formula_edit(g.side_global_formula_edit, formula, compiled, error)) {
                        std::wstring message = (g_str == &kEn) ? L"Invalid global coefficient:\n" : L"Некорректный общий коэффициент:\n";
                        message += error;
                        MessageBoxW(hwnd, message.c_str(), settings_window_title(), MB_OK | MB_ICONWARNING);
                        return 0;
                    }
                    assign_global_formula(formula, compiled);
                    on_signal_transform_changed(true);
                    record_settings_change(before);
                    if (g.settings_wnd) refresh_settings_controls();
                    load_side_transform_controls();
                    return 0;
                }
                case IDC_SIDE_CHANNEL_COLOR:
                    if (g.side_selected_channel >= 0 &&
                        g.side_selected_channel < static_cast<int>(g.ds.channel_count())) {
                        CHOOSECOLORW cc = {};
                        cc.lStructSize = sizeof(cc);
                        cc.hwndOwner = hwnd;
                        cc.lpCustColors = g_custom_colors;
                        cc.rgbResult = channel_color(static_cast<std::size_t>(g.side_selected_channel));
                        cc.Flags = CC_FULLOPEN | CC_RGBINIT;
                        if (ChooseColorW(&cc)) {
                            const SettingsSnapshot before = capture_settings_snapshot();
                            if (static_cast<std::size_t>(g.side_selected_channel) >= g_channel_colors.size()) {
                                g_channel_colors.resize(g.ds.channel_count());
                            }
                            g_channel_colors[static_cast<std::size_t>(g.side_selected_channel)] = cc.rgbResult;
                            record_settings_change(before);
                            if (g.settings_wnd) refresh_settings_controls();
                            refresh_side_panel_controls();
                            InvalidateRect(hwnd, nullptr, FALSE);
                        }
                    }
                    return 0;
                case IDC_SIDE_FORMULA_APPLY_SELECTED:
                case IDC_SIDE_FORMULA_APPLY_VISIBLE: {
                    if (!has_data()) return 0;
                    const SettingsSnapshot before = capture_settings_snapshot();
                    std::wstring formula;
                    std::wstring error;
                    std::vector<FormulaToken> compiled;
                    if (!read_formula_edit(g.side_formula_edit, formula, compiled, error)) {
                        std::wstring message = (g_str == &kEn) ? L"Invalid coefficient:\n" : L"Некорректный коэффициент:\n";
                        message += error;
                        MessageBoxW(hwnd, message.c_str(), settings_window_title(), MB_OK | MB_ICONWARNING);
                        return 0;
                    }

                    ensure_channel_formula_vectors();
                    bool changed = false;
                    if (LOWORD(wp) == IDC_SIDE_FORMULA_APPLY_SELECTED) {
                        if (g.side_selected_channel >= 0 &&
                            g.side_selected_channel < static_cast<int>(g.channel_formulas.size())) {
                            assign_formula_to_channel(static_cast<std::size_t>(g.side_selected_channel), formula, compiled);
                            changed = true;
                        }
                    } else {
                        for (std::size_t i = 0; i < g.visible.size() && i < g.channel_formulas.size(); ++i) {
                            if (!g.visible[i]) continue;
                            assign_formula_to_channel(i, formula, compiled);
                            changed = true;
                        }
                    }

                    if (changed) {
                        on_signal_transform_changed(true);
                        record_settings_change(before);
                        if (g.settings_wnd) refresh_settings_controls();
                        load_side_transform_controls();
                    }
                    return 0;
                }
                case IDC_SIDE_FORMULA_RESET_SELECTED:
                    if (g.side_selected_channel >= 0) {
                        const SettingsSnapshot before = capture_settings_snapshot();
                        reset_channel_transform(static_cast<std::size_t>(g.side_selected_channel));
                        on_signal_transform_changed(true);
                        record_settings_change(before);
                        if (g.settings_wnd) refresh_settings_controls();
                        load_side_transform_controls();
                    }
                    return 0;
                case IDC_SIDE_FORMULA_RESET_ALL:
                    if (has_data()) {
                        const SettingsSnapshot before = capture_settings_snapshot();
                        reset_all_channel_transforms();
                        on_signal_transform_changed(true);
                        record_settings_change(before);
                        if (g.settings_wnd) refresh_settings_controls();
                        load_side_transform_controls();
                    }
                    return 0;
                case IDC_AUTOY:
                    g.auto_y = !g.auto_y;
                    if (!g.auto_y) current_time_yrange(g.y_lock_min, g.y_lock_max);
                    SendMessageW(g.autoy, BM_SETCHECK,
                                 g.auto_y ? BST_CHECKED : BST_UNCHECKED, 0);
                    InvalidateRect(g.autoy, nullptr, FALSE);
                    sync_menu();
                    set_status();
                    InvalidateRect(hwnd, nullptr, TRUE);
                    return 0;
                case IDM_VISMOOTH:
                    g.visual_smooth = !g.visual_smooth;
                    save_runtime_settings();
                    sync_menu();
                    set_status();
                    InvalidateRect(hwnd, nullptr, TRUE);
                    return 0;
                case IDM_VPAN:
                    g.vertical_pan = !g.vertical_pan;
                    save_runtime_settings();
                    sync_menu();
                    set_status();
                    return 0;
                case IDM_THEME:
                    apply_theme_choice((g_theme == &kLightTheme) ? &kDarkTheme : &kLightTheme);
                    return 0;
                case IDM_ADD_VLINE:
                    if (!has_data()) { show_styled_info_prompt(hwnd, g_str->msg_nodata, g_str->msg_openfirst, false); return 0; }
                    if (g.pending_line == 1) {
                        g.pending_line = 0;
                        set_status();
                        sync_menu();
                        InvalidateRect(hwnd, nullptr, FALSE);
                        return 0;
                    }
                    g.pending_line = 1;
                    g.pending_marker = false;
                    g.measure_mode = false;
                    SendMessageW(g.measure, BM_SETCHECK, BST_UNCHECKED, 0);
                    status_msg(g_str->status_vline);
                    sync_menu();
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                case IDM_ADD_VLINE_EXACT: {
                    if (!has_data()) { show_styled_info_prompt(hwnd, g_str->msg_nodata, g_str->msg_openfirst, false); return 0; }
                    double value = 0.0;
                    if (!prompt_exact_guide_value(true, value)) return 0;
                    add_guide_line(true, value);
                    return 0;
                }
                case IDM_ADD_HLINE:
                    if (!has_data()) { show_styled_info_prompt(hwnd, g_str->msg_nodata, g_str->msg_openfirst, false); return 0; }
                    if (g.pending_line == 2) {
                        g.pending_line = 0;
                        set_status();
                        sync_menu();
                        InvalidateRect(hwnd, nullptr, FALSE);
                        return 0;
                    }
                    g.pending_line = 2;
                    g.pending_marker = false;
                    g.measure_mode = false;
                    SendMessageW(g.measure, BM_SETCHECK, BST_UNCHECKED, 0);
                    status_msg(g_str->status_hline);
                    sync_menu();
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                case IDM_ADD_HLINE_EXACT: {
                    if (!has_data()) { show_styled_info_prompt(hwnd, g_str->msg_nodata, g_str->msg_openfirst, false); return 0; }
                    double value = 0.0;
                    if (!prompt_exact_guide_value(false, value)) return 0;
                    add_guide_line(false, value);
                    return 0;
                }
                case IDM_CLEAR_LINES:
                    if (!g.guides.empty()) {
                        UndoAction ua; ua.type = UndoAction::CLEAR_LINES; ua.saved_lines = g.guides;
                        push_undo(ua);
                        g.guides.clear(); InvalidateRect(hwnd, nullptr, FALSE);
                    }
                    set_status();
                    return 0;
                case IDM_CLEAR_POINTS:
                    if (has_measure_points()) {
                        UndoAction ua;
                        ua.type = UndoAction::CLEAR_POINTS;
                        ua.saved_point_groups = g.point_groups;
                        ua.saved_active_point_group = g.active_point_group;
                        push_undo(ua);
                        clear_measure_point_groups();
                        if (g.settings_wnd) populate_point_group_list(g.settings_wnd);
                        refresh_side_panel_controls();
                        InvalidateRect(hwnd, nullptr, FALSE);
                    }
                    set_status();
                    return 0;
                case IDM_ADD_MARKER:
                    if (!has_data()) { show_styled_info_prompt(hwnd, g_str->msg_nodata, g_str->msg_openfirst, false); return 0; }
                    if (g.pending_marker) {
                        g.pending_marker = false;
                        set_status();
                        sync_menu();
                        InvalidateRect(hwnd, nullptr, FALSE);
                        return 0;
                    }
                    g.pending_marker = true;
                    g.pending_line = 0;
                    g.measure_mode = false;
                    SendMessageW(g.measure, BM_SETCHECK, BST_UNCHECKED, 0);
                    status_msg(g_str->status_marker);
                    sync_menu();
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                case IDM_CLEAR_MARKERS:
                    if (!g.markers.empty()) {
                        UndoAction ua; ua.type = UndoAction::CLEAR_MARKERS; ua.saved_markers = g.markers;
                        push_undo(ua);
                        g.markers.clear(); InvalidateRect(hwnd, nullptr, FALSE);
                    }
                    g.active_marker = -1;
                    set_status();
                    return 0;
                case IDM_SPEED_00001: set_play_speed(0.0001); return 0;
                case IDM_SPEED_0001: set_play_speed(0.001); return 0;
                case IDM_SPEED_001: set_play_speed(0.01); return 0;
                case IDM_SPEED_01: set_play_speed(0.1); return 0;
                case IDM_SPEED_05: set_play_speed(0.5); return 0;
                case IDM_SPEED_1: set_play_speed(1.0); return 0;
                case IDM_SPEED_2: set_play_speed(2.0); return 0;
                case IDM_SPEED_5: set_play_speed(5.0); return 0;
                case IDM_SPEED_10: set_play_speed(10.0); return 0;
                case IDM_SPEED_CUSTOM: {
                    double speed = g.play_speed;
                    if (prompt_custom_play_speed(speed)) set_play_speed(speed);
                    return 0;
                }
                case IDM_UNDO:
                    pop_undo();
                    InvalidateRect(hwnd, nullptr, FALSE);
                    set_status();
                    return 0;
                case IDM_REDO:
                    pop_redo();
                    InvalidateRect(hwnd, nullptr, FALSE);
                    set_status();
                    return 0;
                case IDC_ZOOMIN: zoom_at(0.5, 0.7); return 0;
                case IDC_ZOOMOUT: zoom_at(0.5, 1.0 / 0.7); return 0;
                case IDC_RESET: reset_view(); return 0;
                case IDC_GOTO_START: goto_start(); return 0;
                case IDC_GOTO_END: goto_end(); return 0;
                case IDC_PANLEFT: pan_by(-0.2); return 0;
                case IDC_PANRIGHT: pan_by(0.2); return 0;
                case IDM_HOTKEYS: show_hotkeys(); return 0;
                case IDM_ABOUT: show_about(); return 0;
                case IDM_LANG_RU: g_str = &kRu; save_runtime_settings(); rebuild_ui(); return 0;
                case IDM_LANG_EN: g_str = &kEn; save_runtime_settings(); rebuild_ui(); return 0;
                case IDC_SIDE_POINT_GROUP_LIST:
                    if (HIWORD(wp) == LBN_SELCHANGE) {
                        const int index = side_selected_point_group();
                        if (index >= 0 && index < static_cast<int>(g.point_groups.size())) {
                            g.active_point_group = index;
                            g.marker_color = g.point_groups[static_cast<std::size_t>(index)].color;
                            sync_point_display_from_active_group();
                            save_runtime_settings();
                            if (g.settings_wnd) refresh_settings_controls();
                            load_side_point_group_controls();
                            refresh_side_panel_controls();
                            set_status();
                            InvalidateRect(hwnd, nullptr, FALSE);
                        }
                    }
                    return 0;
                case IDC_SIDE_POINT_GROUP_VISIBLE: {
                    const int index = side_selected_point_group();
                    if (index >= 0 && index < static_cast<int>(g.point_groups.size())) {
                        const SettingsSnapshot before = capture_settings_snapshot();
                        toggle_checked_state(g.side_point_group_visible);
                        const bool checked = is_toggle_checked(g.side_point_group_visible);
                        g.point_groups[static_cast<std::size_t>(index)].visible = checked;
                        record_settings_change(before);
                        save_runtime_settings();
                        if (g.settings_wnd) refresh_settings_controls();
                        load_side_point_group_controls();
                        refresh_side_panel_controls();
                        set_status();
                        InvalidateRect(hwnd, nullptr, FALSE);
                    }
                    return 0;
                }
                case IDC_SIDE_POINT_GROUP_NEW: {
                    const SettingsSnapshot before = capture_settings_snapshot();
                    create_point_group(g.marker_color);
                    record_settings_change(before);
                    save_runtime_settings();
                    if (g.settings_wnd) refresh_settings_controls();
                    refresh_side_panel_controls();
                    set_status();
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
                case IDC_SIDE_POINT_GROUP_DELETE: {
                    const int index = side_selected_point_group();
                    if (index >= 0 && index < static_cast<int>(g.point_groups.size())) {
                        const SettingsSnapshot before = capture_settings_snapshot();
                        erase_point_group(static_cast<std::size_t>(index));
                        if (PointGroup* group = active_point_group()) g.marker_color = group->color;
                        record_settings_change(before);
                        save_runtime_settings();
                        if (g.settings_wnd) refresh_settings_controls();
                        refresh_side_panel_controls();
                        set_status();
                        InvalidateRect(hwnd, nullptr, FALSE);
                    }
                    return 0;
                }
                case IDC_SIDE_POINT_GROUP_RENAME: {
                    const int index = side_selected_point_group();
                    if (index >= 0 && index < static_cast<int>(g.point_groups.size()) && g.side_point_group_name) {
                        const SettingsSnapshot before = capture_settings_snapshot();
                        wchar_t buf[256]{};
                        GetWindowTextW(g.side_point_group_name, buf, 256);
                        std::wstring name = buf;
                        if (name.empty()) {
                            name = (g_str == &kEn) ? (L"Group " + std::to_wstring(index + 1)) : (L"Группа " + std::to_wstring(index + 1));
                        }
                        g.point_groups[static_cast<std::size_t>(index)].name = name;
                        record_settings_change(before);
                        if (g.settings_wnd) refresh_settings_controls();
                        refresh_side_panel_controls();
                        InvalidateRect(hwnd, nullptr, FALSE);
                    }
                    return 0;
                }
                case IDC_SIDE_POINT_COLOR_CURRENT: {
                    CHOOSECOLORW cc = {};
                    cc.lStructSize = sizeof(cc);
                    cc.hwndOwner = hwnd;
                    cc.lpCustColors = g_custom_colors;
                    cc.rgbResult = g.marker_color;
                    cc.Flags = CC_FULLOPEN | CC_RGBINIT;
                    if (ChooseColorW(&cc)) {
                        const SettingsSnapshot before = capture_settings_snapshot();
                        g.marker_color = cc.rgbResult;
                        PointGroup* group = active_point_group();
                        if (group && group->points.empty()) group->color = g.marker_color;
                        record_settings_change(before);
                        save_runtime_settings();
                        if (g.settings_wnd) refresh_settings_controls();
                        refresh_side_panel_controls();
                        InvalidateRect(hwnd, nullptr, FALSE);
                    }
                    return 0;
                }
                case IDC_SIDE_POINT_GROUP_COLOR: {
                    const int index = side_selected_point_group();
                    if (index < 0 || index >= static_cast<int>(g.point_groups.size())) return 0;
                    CHOOSECOLORW cc = {};
                    cc.lStructSize = sizeof(cc);
                    cc.hwndOwner = hwnd;
                    cc.lpCustColors = g_custom_colors;
                    cc.rgbResult = g.point_groups[static_cast<std::size_t>(index)].color;
                    cc.Flags = CC_FULLOPEN | CC_RGBINIT;
                    if (ChooseColorW(&cc)) {
                        const SettingsSnapshot before = capture_settings_snapshot();
                        g.point_groups[static_cast<std::size_t>(index)].color = cc.rgbResult;
                        g.active_point_group = index;
                        g.marker_color = cc.rgbResult;
                        record_settings_change(before);
                        save_runtime_settings();
                        if (g.settings_wnd) refresh_settings_controls();
                        refresh_side_panel_controls();
                        InvalidateRect(hwnd, nullptr, FALSE);
                    }
                    return 0;
                }
                case IDC_SIDE_PT_NUM:
                case IDC_SIDE_PT_X:
                case IDC_SIDE_PT_Y:
                case IDC_SIDE_PT_DX:
                case IDC_SIDE_PT_DY:
                case IDC_SIDE_PT_INVDT:
                case IDC_SIDE_PT_DIST: {
                    const SettingsSnapshot before = capture_settings_snapshot();
                    PointDisplay* display = active_point_display();
                    if (!display) return 0;
                    toggle_checked_state(GetDlgItem(hwnd, id));
                    auto checked = [&](int ctl_id) {
                        return is_toggle_checked(GetDlgItem(hwnd, ctl_id));
                    };
                    display->number = checked(IDC_SIDE_PT_NUM);
                    display->x = checked(IDC_SIDE_PT_X);
                    display->y = checked(IDC_SIDE_PT_Y);
                    display->dx = checked(IDC_SIDE_PT_DX);
                    display->dy = checked(IDC_SIDE_PT_DY);
                    display->inv_dt = checked(IDC_SIDE_PT_INVDT);
                    display->dist = checked(IDC_SIDE_PT_DIST);
                    g.pdisp = *display;
                    record_settings_change(before);
                    save_runtime_settings();
                    if (g.settings_wnd) refresh_settings_controls();
                    sync_point_display_from_active_group();
                    refresh_side_panel_controls();
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
                case IDC_SIDE_PT_SNAP: {
                    const SettingsSnapshot before = capture_settings_snapshot();
                    toggle_checked_state(GetDlgItem(hwnd, id));
                    g.snap_to_data = is_toggle_checked(GetDlgItem(hwnd, id));
                    record_settings_change(before);
                    save_runtime_settings();
                    if (g.settings_wnd) refresh_settings_controls();
                    refresh_side_panel_controls();
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
                case IDC_SIDE_FILTER_ENABLE: {
                    if (HIWORD(wp) == BN_CLICKED || HIWORD(wp) == BN_DOUBLECLICKED) {
                        const SettingsSnapshot before = capture_settings_snapshot();
                        toggle_checked_state(GetDlgItem(hwnd, id));
                        g.noise_threshold_enabled = is_toggle_checked(GetDlgItem(hwnd, id));
                        normalize_filter_bounds();
                        record_settings_change(before);
                        recompute_transforms_from_state();
                        save_runtime_settings();
                        refresh_side_panel_controls();
                        set_status();
                        InvalidateRect(hwnd, nullptr, TRUE);
                    }
                    return 0;
                }
                case IDC_SIDE_FILTER_MODE: {
                    if (HIWORD(wp) == CBN_DROPDOWN) {
                        expand_combo_dropdown(GetDlgItem(hwnd, id));
                        return 0;
                    }
                    if (HIWORD(wp) == CBN_SELCHANGE && !g.updating_noise_threshold_edits) {
                        HWND combo = GetDlgItem(hwnd, id);
                        const int sel = static_cast<int>(SendMessageW(combo, CB_GETCURSEL, 0, 0));
                        if (sel != CB_ERR) {
                            const SettingsSnapshot before = capture_settings_snapshot();
                            const int value = static_cast<int>(SendMessageW(combo, CB_GETITEMDATA, sel, 0));
                            g.noise_threshold_mode = std::clamp(value,
                                                                static_cast<int>(FilterModeLowPass),
                                                                static_cast<int>(FilterModeBandStop));
                            normalize_filter_bounds();
                            record_settings_change(before);
                            recompute_transforms_from_state();
                            save_runtime_settings();
                            refresh_side_panel_controls();
                            set_status();
                            InvalidateRect(hwnd, nullptr, TRUE);
                        }
                    }
                    return 0;
                }
                case IDC_SIDE_FILTER_TOPOLOGY: {
                    if (HIWORD(wp) == CBN_DROPDOWN) {
                        expand_combo_dropdown(GetDlgItem(hwnd, id));
                        return 0;
                    }
                    if (HIWORD(wp) == CBN_SELCHANGE && !g.updating_noise_threshold_edits) {
                        HWND combo = GetDlgItem(hwnd, id);
                        const int sel = static_cast<int>(SendMessageW(combo, CB_GETCURSEL, 0, 0));
                        if (sel != CB_ERR) {
                            const SettingsSnapshot before = capture_settings_snapshot();
                            const int value = static_cast<int>(SendMessageW(combo, CB_GETITEMDATA, sel, 0));
                            g.noise_threshold_topology = std::clamp(value,
                                                                    static_cast<int>(FilterTopologyButterworth),
                                                                    static_cast<int>(FilterTopologyLinkwitzRiley));
                            record_settings_change(before);
                            recompute_transforms_from_state();
                            save_runtime_settings();
                            refresh_side_panel_controls();
                            set_status();
                            InvalidateRect(hwnd, nullptr, TRUE);
                        }
                    }
                    return 0;
                }
                default: break;
            }
            if (id >= IDC_CHAN_BASE && id < IDC_CHAN_BASE + static_cast<int>(g.visible.size())) {
                const int ci = id - IDC_CHAN_BASE;
                const SettingsSnapshot before = capture_settings_snapshot();
                g.side_selected_channel = ci;
                toggle_checked_state(g.checks[ci]);
                g.visible[ci] = is_toggle_checked(g.checks[ci]);
                invalidate_plot_analysis_cache();
                record_settings_change(before);
                load_side_transform_controls();
                InvalidateRect(hwnd, nullptr, TRUE);
            } else if (id >= IDC_CHAN_LABEL_BASE &&
                       id < IDC_CHAN_LABEL_BASE + static_cast<int>(g.channel_labels.size())) {
                const int ci = id - IDC_CHAN_LABEL_BASE;
                g.side_selected_channel = ci;
                load_side_transform_controls();
                if (HIWORD(wp) == STN_DBLCLK) {
                    start_channel_rename(ci);
                } else {
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
            }
            return 0;
        }
        case WM_SETCURSOR: {
            if (reinterpret_cast<HWND>(wp) == hwnd) {
                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(hwnd, &pt);
                RECT p = plot_rect();
                bool in_plot = (pt.x >= p.left && pt.x <= p.right && pt.y >= p.top && pt.y <= p.bottom);
                const bool selecting_fft_here =
                    !g.freq_mode && in_plot &&
                    (GetKeyState(VK_SHIFT) & 0x8000) != 0 &&
                    !g.measure_mode && !g.pending_line && !g.pending_marker;
                if (g.dragging || g.fft_selecting || g.measure_mode || g.pending_line || g.pending_marker || selecting_fft_here) {
                    SetCursor(LoadCursor(nullptr, IDC_CROSS));
                    return TRUE;
                }
                if (in_plot) {
                    SetCursor(LoadCursor(nullptr, IDC_HAND));
                    return TRUE;
                }
            }
            break;
        }
        case WM_MOUSEWHEEL: {
            if (g.gap_details_visible) {
                hide_gap_details_card();
            }
            POINT pt = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
            ScreenToClient(hwnd, &pt);
            if (side_panel_hit_test(pt) && g.side_scroll_max > 0) {
                const int direction = GET_WHEEL_DELTA_WPARAM(wp) > 0 ? -48 : 48;
                scroll_side_panel(direction);
                return 0;
            }
            if (!has_data()) return 0;
            const RECT p = plot_rect();
            bool in_plot = (pt.x >= p.left && pt.x <= p.right && pt.y >= p.top && pt.y <= p.bottom);
            bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
            bool up = GET_WHEEL_DELTA_WPARAM(wp) > 0;

            if (shift) {
                pan_by(up ? -0.1 : 0.1);
                return 0;
            }
            if (ctrl) {
                if (g.freq_mode) {
                    if (in_plot) {
                        double frac = static_cast<double>(p.bottom - pt.y) / (p.bottom - p.top);
                        zoom_y_amp_at(frac, up ? 0.85 : 1.0 / 0.85);
                    } else {
                        zoom_y_amp_at(0.5, up ? 0.85 : 1.0 / 0.85);
                    }
                } else {
                    if (in_plot) {
                        double frac = static_cast<double>(p.bottom - pt.y) / (p.bottom - p.top);
                        zoom_y_at(frac, up ? 0.85 : 1.0 / 0.85);
                    } else {
                        zoom_y_at(0.5, up ? 0.85 : 1.0 / 0.85);
                    }
                }
                return 0;
            }
            if (alt) {
                pan_y_by(up ? -0.1 : 0.1);
                return 0;
            }
            double frac = 0.5;
            if (pt.x >= p.left && pt.x <= p.right)
                frac = static_cast<double>(pt.x - p.left) / (p.right - p.left);
            zoom_at(frac, up ? 0.8 : 1.25);
            return 0;
        }
        case WM_LBUTTONDOWN: {
            finish_channel_rename_if_click_outside(hwnd);
            if (g.gap_details_visible) {
                hide_gap_details_card();
            }
            const int mx = GET_X_LPARAM(lp), my = GET_Y_LPARAM(lp);
            const RECT p = plot_rect();
            if (!has_data()) {
                if (mx >= p.left && mx <= p.right && my >= p.top && my <= p.bottom) {
                    open_file();
                }
                return 0;
            }

            // --- Legend click handling (toggle / solo) ---
            if (mx >= g_legend_box.left && mx < g_legend_box.right &&
                my >= g_legend_box.top && my < g_legend_box.bottom) {
                for (const auto& li : g_legend_items) {
                    if (mx >= li.rect.left && mx < li.rect.right &&
                        my >= li.rect.top && my < li.rect.bottom) {
                        const int ci = li.channel;
                        if ((GetKeyState(VK_CONTROL) & 0x8000) != 0) {
                            // Solo: show only this channel
                            const SettingsSnapshot before = capture_settings_snapshot();
                            for (std::size_t j = 0; j < g.visible.size(); ++j) {
                                g.visible[j] = (static_cast<int>(j) == ci);
                                if (j < g.checks.size())
                                    set_toggle_checked(g.checks[j], g.visible[j] != 0);
                            }
                            invalidate_plot_analysis_cache();
                            record_settings_change(before);
                        } else {
                            // Toggle
                            const SettingsSnapshot before = capture_settings_snapshot();
                            g.visible[ci] = !g.visible[ci];
                            if (ci < static_cast<int>(g.checks.size()))
                                set_toggle_checked(g.checks[ci], g.visible[ci] != 0);
                            invalidate_plot_analysis_cache();
                            record_settings_change(before);
                        }
                        InvalidateRect(hwnd, nullptr, TRUE);
                        return 0;
                    }
                }
            }

            if (!g.freq_mode && g.show_gap_markers) {
                const int gap_index = hit_test_gap_marker(mx, my);
                if (gap_index >= 0) {
                    g.gap_click_pending = false;
                    g.gap_click_index = -1;
                    if (prepare_plot_drag(mx, my)) {
                        g.gap_click_pending = true;
                        g.gap_click_index = gap_index;
                        SetCapture(hwnd);
                    }
                    return 0;
                }
            }

            if (g.side_panel_visible && g.side_panel_tab == 0) {
                for (std::size_t i = 0; i < g.checks.size(); ++i) {
                    HWND c = g.checks[i];
                    if (!c || !IsWindowVisible(c)) continue;
                    RECT cr;
                    GetWindowRect(c, &cr);
                    MapWindowPoints(nullptr, g.main, reinterpret_cast<LPPOINT>(&cr), 2);
                    RECT sr = {cr.left - 18, cr.top + (cr.bottom - cr.top - 12) / 2, cr.left - 6, cr.top + (cr.bottom - cr.top - 12) / 2 + 12};
                    if (mx >= sr.left && mx < sr.right && my >= sr.top && my < sr.bottom) {
                        CHOOSECOLORW cc = {};
                        cc.lStructSize = sizeof(cc);
                        cc.hwndOwner = hwnd;
                        cc.lpCustColors = g_custom_colors;
                        cc.rgbResult = channel_color(i);
                        cc.Flags = CC_FULLOPEN | CC_RGBINIT;
                        if (ChooseColorW(&cc)) {
                            const SettingsSnapshot before = capture_settings_snapshot();
                            if (i >= g_channel_colors.size()) g_channel_colors.resize(g.ds.channel_count());
                            g_channel_colors[i] = cc.rgbResult;
                            record_settings_change(before);
                            InvalidateRect(hwnd, nullptr, FALSE);
                        }
                        return 0;
                    }
                }
            }

            if (mx < p.left || mx > p.right || my < p.top || my > p.bottom) return 0;
            const bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            if (!g.freq_mode && shift && !g.pending_line && !g.pending_marker && !g.measure_mode) {
                const int pw = p.right - p.left;
                if (pw > 0) {
                    const int clamped_x = std::clamp(mx, static_cast<int>(p.left), static_cast<int>(p.right));
                    const double frac = static_cast<double>(clamped_x - p.left) / pw;
                    const double tt = g.win_start + frac * (g.win_end - g.win_start);
                    if (fft_window_contains_time(tt)) {
                        clear_fft_window();
                        set_status();
                        InvalidateRect(hwnd, nullptr, FALSE);
                        return 0;
                    }
                    g.fft_selecting = true;
                    g.fft_select_anchor_x = clamped_x;
                    g.fft_select_current_x = clamped_x;
                    g.fft_select_anchor_t = tt;
                    g.fft_select_current_t = tt;
                    SetCapture(hwnd);
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
            }
            if (g.pending_line) {
                double dx, dy;
                if (px_to_data(mx, my, dx, dy)) {
                    GuideLine gl;
                    gl.vertical = (g.pending_line == 1);
                    gl.freq = g.freq_mode;
                    if (gl.vertical && g.snap_to_data) { double sx = dx, sy = dy; snap_to_nearest(sx, sy); dx = sx; }
                    gl.value = gl.vertical ? dx : dy;
                    g.guides.push_back(gl);
                    UndoAction ua; ua.type = UndoAction::ADD_LINE; ua.line = gl;
                    push_undo(ua);
                }
                // Режим множественного добавления: остаётся активным до Esc
                set_status();
                sync_menu();
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
            if (g.pending_marker) {
                double dx, dy;
                if (px_to_data(mx, my, dx, dy)) {
                    App::Marker mk;
                    int snapped_channel = -1;
                    bool snapped = false;
                    if (g.snap_to_data) snapped = snap_to_nearest_target(dx, dy, &snapped_channel);
                    mk.x = dx;
                    mk.y = dy;
                    mk.freq = g.freq_mode;
                    mk.snapped = snapped;
                    mk.channel = snapped ? snapped_channel : -1;
                    wchar_t buf[16];
                    swprintf(buf, 16, L"M%zu", g.markers.size() + 1);
                    mk.label = buf;
                    g.markers.push_back(mk);
                    g.active_marker = static_cast<int>(g.markers.size()) - 1;
                    UndoAction ua; ua.type = UndoAction::ADD_MARKER; ua.marker = mk;
                    push_undo(ua);
                }
                set_status();
                sync_menu();
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
            if (g.measure_mode) {
                double dx, dy;
                if (px_to_data(mx, my, dx, dy)) {
                    const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                    if (g.snap_to_data) snap_to_nearest(dx, dy);
                    bool created_group = false;
                    const int group_index = ensure_point_group_for_measurement(ctrl, &created_group);
                    if (group_index < 0 || group_index >= static_cast<int>(g.point_groups.size())) return 0;
                    g.point_groups[static_cast<std::size_t>(group_index)].points.push_back({dx, dy});
                    UndoAction ua;
                    ua.type = UndoAction::ADD_POINT;
                    ua.point = {dx, dy};
                    ua.point_group_index = group_index;
                    ua.point_group_created = created_group;
                    ua.point_group_state = g.point_groups[static_cast<std::size_t>(group_index)];
                    ua.point_group_state.points.clear();
                    push_undo(ua);
                    if (g.settings_wnd) populate_point_group_list(g.settings_wnd);
                    refresh_side_panel_controls();
                    set_status();
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                return 0;
            }
            if (!prepare_plot_drag(mx, my)) return 0;
            g.gap_click_pending = false;
            g.gap_click_index = -1;
            g.dragging = true;
            SetCapture(hwnd);
            return 0;
        }
        case WM_RBUTTONDOWN:
            if (g.gap_details_visible) {
                hide_gap_details_card();
            }
            if (has_fft_window()) {
                clear_fft_window();
                set_status();
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
            if (has_measure_points()) {
                UndoAction ua;
                ua.type = UndoAction::CLEAR_POINTS;
                ua.saved_point_groups = g.point_groups;
                ua.saved_active_point_group = g.active_point_group;
                push_undo(ua);
                clear_measure_point_groups();
                if (g.settings_wnd) populate_point_group_list(g.settings_wnd);
                set_status();
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        case WM_MOUSEMOVE: {
            if (g.fft_selecting) {
                const RECT p = plot_rect();
                const int pw = p.right - p.left;
                if (pw > 0) {
                    const int clamped_x = std::clamp(GET_X_LPARAM(lp), static_cast<int>(p.left), static_cast<int>(p.right));
                    const double frac = static_cast<double>(clamped_x - p.left) / pw;
                    g.fft_select_current_x = clamped_x;
                    g.fft_select_current_t = g.win_start + frac * (g.win_end - g.win_start);
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                return 0;
            }
            const int hovered_marker = hit_test_marker(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
            if (hovered_marker >= 0 && hovered_marker != g.active_marker) {
                g.active_marker = hovered_marker;
                set_status();
                RECT rc; GetClientRect(hwnd, &rc);
                RECT sr = {0, rc.bottom - kBottomBar, rc.right, rc.bottom};
                InvalidateRect(hwnd, &sr, FALSE);
            }
            if (g.gap_click_pending && !g.dragging) {
                const int dx = GET_X_LPARAM(lp) - g.drag_x;
                const int dy = GET_Y_LPARAM(lp) - g.drag_y;
                if (std::abs(dx) >= 4 || std::abs(dy) >= 4) {
                    g.gap_click_pending = false;
                    g.gap_click_index = -1;
                    g.dragging = true;
                } else {
                    return 0;
                }
            }
            if (!g.dragging) return 0;
            double *lo, *hi, minb, maxb, minw;
            if (!active_axis(lo, hi, minb, maxb, minw)) return 0;
            const RECT p = plot_rect();
            const int pw = p.right - p.left;
            const double span = g.drag_hi - g.drag_lo;
            const double d = static_cast<double>(GET_X_LPARAM(lp) - g.drag_x) / pw * span;
            *lo = g.drag_lo - d;
            *hi = g.drag_hi - d;
            clamp_range(*lo, *hi, minb, maxb, minw);
            // Vertical panning
            if (g.vertical_pan) {
                const int ph = p.bottom - p.top;
                if (ph > 0) {
                    const double dy = static_cast<double>(GET_Y_LPARAM(lp) - g.drag_y) / ph * (g.drag_y_hi - g.drag_y_lo);
                    if (g.freq_mode) {
                        double new_ytop = g.drag_y_hi + dy;
                        if (new_ytop < 1e-12) new_ytop = 1e-12;
                        g.y_amp_max = new_ytop;
                        g.auto_y_amp = false;
                    } else {
                        double new_lo = g.drag_y_lo + dy;
                        double new_hi = g.drag_y_hi + dy;
                        g.y_lock_min = new_lo;
                        g.y_lock_max = new_hi;
                        g.auto_y = false;
                        if (g.autoy) { SendMessageW(g.autoy, BM_SETCHECK, BST_UNCHECKED, 0); InvalidateRect(g.autoy, nullptr, FALSE); }
                    }
                }
            }
            set_status();
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        case WM_LBUTTONUP:
            if (g.fft_selecting) {
                g.fft_selecting = false;
                if (GetCapture() == hwnd) ReleaseCapture();
                if (std::abs(g.fft_select_current_x - g.fft_select_anchor_x) >= 4) {
                    set_fft_window(g.fft_select_anchor_t, g.fft_select_current_t);
                }
                set_status();
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
            if (g.gap_click_pending) {
                const int pending_gap_index = g.gap_click_index;
                g.gap_click_pending = false;
                g.gap_click_index = -1;
                if (GetCapture() == hwnd) ReleaseCapture();
                if (!g.freq_mode && g.show_gap_markers) {
                    const int released_gap_index = hit_test_gap_marker(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
                    if (released_gap_index >= 0 && released_gap_index == pending_gap_index) {
                        const auto& gap = g.visible_gap_markers[static_cast<std::size_t>(released_gap_index)];
                        show_gap_details_card(gap.duration, gap.estimated_missing_samples);
                    }
                }
                return 0;
            }
            if (g.dragging) { g.dragging = false; ReleaseCapture(); }
            return 0;
        case WM_KEYDOWN:
            if (g.gap_details_visible && wp != VK_ESCAPE) {
                hide_gap_details_card();
            }
            if (wp == VK_ESCAPE) {
                if (g.gap_details_visible) {
                    hide_gap_details_card();
                    return 0;
                }
                if (g.pending_line || g.pending_marker) {
                    g.pending_line = 0;
                    g.pending_marker = false;
                    set_status();
                    sync_menu();
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
                if (g.fft_selecting) {
                    g.fft_selecting = false;
                    if (GetCapture() == hwnd) ReleaseCapture();
                    set_status();
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
                if (has_fft_window()) {
                    clear_fft_window();
                    set_status();
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return 0;
                }
            }
            break;
        case WM_APP_ASYNC_SCAN_DONE: {
            std::unique_ptr<AsyncScanResult> result(reinterpret_cast<AsyncScanResult*>(lp));
            if (!result) return 0;
            if (result->token != g.async_load_token || g.async_load_stage != AsyncLoadStage::ScanningRange) return 0;
            g.async_load_stage = AsyncLoadStage::None;
            g.async_load_cancel_flag.reset();
            hide_loading();
            if (result->cancelled) {
                g.last_error.clear();
                return 0;
            }
            if (!result->ok) {
                g.last_error = result->error;
                MessageBoxW(hwnd, to_w(g.last_error).c_str(), g_str->msg_read_err, MB_ICONERROR | MB_OK);
                return 0;
            }
            g.cached_scan_path = result->path;
            g.cached_scan_start = result->range_start;
            g.cached_scan_end = result->range_end;
            g.cached_scan_valid = true;
            if (!prompt_and_start_light_mode_load(result->path, result->range_start, result->range_end) &&
                !g.last_error.empty()) {
                MessageBoxW(hwnd, to_w(g.last_error).c_str(), g_str->msg_read_err, MB_ICONERROR | MB_OK);
            }
            return 0;
        }
        case WM_APP_ASYNC_LOAD_DONE: {
            std::unique_ptr<AsyncLoadResult> result(reinterpret_cast<AsyncLoadResult*>(lp));
            if (!result) return 0;
            if (result->token != g.async_load_token || g.async_load_stage != AsyncLoadStage::LoadingFile) return 0;
            g.async_load_stage = AsyncLoadStage::None;
            g.async_load_cancel_flag.reset();
            hide_loading();
            if (result->cancelled) {
                g.last_error.clear();
                return 0;
            }
            if (!result->ok) {
                g.last_error = result->error;
                MessageBoxW(hwnd, to_w(g.last_error).c_str(), g_str->msg_read_err, MB_ICONERROR | MB_OK);
                return 0;
            }
            apply_loaded_dataset(std::move(result->ds), result->path, result->hide_channels,
                                 result->requested_time_window, result->cached_global_gap_step,
                                 result->cached_global_gap_step_ready);
            return 0;
        }
        case WM_DROPFILES: {
            handle_file_drop(hwnd, reinterpret_cast<HDROP>(wp));
            return 0;
        }
        case WM_DESTROY:
            request_async_load_cancel();
            hide_loading();
            flush_runtime_settings_save();
            save_app_settings();
            stop_play();
            KillTimer(hwnd, kRuntimeSettingsSaveTimerId);
            KillTimer(hwnd, 2);
            release_backbuffer();
            if (g.ui_font && g.ui_font != reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT)))
                DeleteObject(g.ui_font);
            if (g.menu_font) DeleteObject(g.menu_font);
            if (g.bold_font) DeleteObject(g.bold_font);
            if (g.title_font) DeleteObject(g.title_font);
            if (g.axis_font) DeleteObject(g.axis_font);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

HACCEL make_accelerators() {
    ensure_hotkeys_initialized();
    std::vector<ACCEL> acc;
    acc.reserve(g.hotkeys.size());
    for (const auto& hk : g.hotkeys) {
        if (hk.key == 0) continue;
        ACCEL a = {};
        a.fVirt = hk.fvirt;
        a.key = hk.key;
        a.cmd = static_cast<WORD>(hk.command);
        acc.push_back(a);
    }
    if (acc.empty()) return nullptr;
    return CreateAcceleratorTableW(acc.data(), static_cast<int>(acc.size()));
}

void rebuild_accelerators() {
    HACCEL fresh = make_accelerators();
    if (g.accel) DestroyAcceleratorTable(g.accel);
    g.accel = fresh;
}

bool should_bypass_accelerators() {
    HWND focus = GetFocus();
    if (!focus) return false;
    if (g.channel_edit && (focus == g.channel_edit || IsChild(g.channel_edit, focus) != FALSE)) return true;
    if (g.settings_wnd && (focus == g.settings_wnd || IsChild(g.settings_wnd, focus) != FALSE)) return true;
    wchar_t cls[64]{};
    for (HWND h = focus; h; h = GetParent(h)) {
        GetClassNameW(h, cls, 64);
        if (lstrcmpiW(cls, L"EDIT") == 0 ||
            lstrcmpiW(cls, L"COMBOBOX") == 0 ||
            lstrcmpiW(cls, L"ComboBoxEx32") == 0 ||
            wcsstr(cls, L"RICHEDIT") == cls) {
            return true;
        }
        if (h == g.main) break;
    }
    return false;
}

}  // namespace

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, PWSTR cmd, int show) {
    Gdiplus::GdiplusStartupInput gdi_in;
    Gdiplus::GdiplusStartup(&g_gdiplus_token, &gdi_in, nullptr);

    INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_BAR_CLASSES};
    InitCommonControlsEx(&icc);
    load_app_settings();
    load_runtime_settings();
    load_program_logo();
    HICON class_icon = g_program_logo_icon ? g_program_logo_icon : LoadIcon(nullptr, IDI_APPLICATION);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = L"LvmViewerWnd";
    wc.hIcon = class_icon;
    wc.hIconSm = class_icon;
    RegisterClassExW(&wc);

    // Settings panel + welcome screen window classes.
    WNDCLASSEXW sc = {};
    sc.cbSize = sizeof(sc);
    sc.lpfnWndProc = SettingsProc;
    sc.hInstance = inst;
    sc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    sc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    sc.lpszClassName = L"LvmPtSettings";
    sc.hIcon = class_icon;
    sc.hIconSm = class_icon;
    RegisterClassExW(&sc);

    WNDCLASSEXW wcw = {};
    wcw.cbSize = sizeof(wcw);
    wcw.lpfnWndProc = WelcomeProc;
    wcw.hInstance = inst;
    wcw.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcw.hbrBackground = nullptr;
    wcw.lpszClassName = L"LvmWelcome";
    wcw.hIcon = class_icon;
    wcw.hIconSm = class_icon;
    RegisterClassExW(&wcw);

    g.main = CreateWindowExW(0, wc.lpszClassName, g_str->app_title,
                             WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, 1180, 720,
                             nullptr, nullptr, inst, nullptr);
    if (!g.main) return 1;
    SendMessageW(g.main, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(class_icon));
    SendMessageW(g.main, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(class_icon));
    if (g.runtime_settings_save_pending) {
        save_runtime_settings();
    }

    ShowWindow(g.main, show);
    UpdateWindow(g.main);
    DragAcceptFiles(g.main, TRUE);

    if (cmd && *cmd) {
        std::wstring path = cmd;
        if (!path.empty() && path.front() == L'"') path = path.substr(1, path.find_last_of(L'"') - 1);
        if (!load_path_interactive(path) && !g.last_error.empty())
            MessageBoxW(g.main, to_w(g.last_error).c_str(), g_str->msg_read_err, MB_ICONERROR | MB_OK);
    } else {
        show_welcome(inst);   // start screen when launched without a file
    }

    ensure_hotkeys_initialized();
    rebuild_accelerators();
    MSG m;
    while (GetMessage(&m, nullptr, 0, 0) > 0) {
        if (should_bypass_accelerators() || !g.accel || !TranslateAcceleratorW(g.main, g.accel, &m)) {
            TranslateMessage(&m);
            DispatchMessage(&m);
        }
    }
    if (g.accel) DestroyAcceleratorTable(g.accel);
    unload_program_logo();
    Gdiplus::GdiplusShutdown(g_gdiplus_token);
    return 0;
}
