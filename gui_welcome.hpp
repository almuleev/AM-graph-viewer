#pragma once
#include "gui_platform.hpp"
#include "gui_settings.hpp"

namespace gui {

std::wstring welcome_version_text();

const wchar_t* welcome_actions_title_text();

const wchar_t* welcome_open_button_text();

const wchar_t* welcome_actions_hint_text();

const wchar_t* welcome_author_credit_text();

struct WelcomeLayout {
    RECT bounds{};
    RECT hero{};
    RECT action{};
    bool stacked = false;
    bool compact = false;
};

int rect_width(const RECT& r);

int rect_height(const RECT& r);

WelcomeLayout compute_welcome_layout(HWND hwnd);

void layout_welcome_controls(HWND hwnd);

// ---- welcome / start screen ----------------------------------------------

struct RecentFilesPanelState {
    HWND wnd = nullptr;
    std::array<HWND, kMaxRecentFiles> items{};
    bool visible = false;
};

extern RecentFilesPanelState g_recent_files_panel;

inline constexpr int kRecentPanelItemBase = 6400;

inline constexpr int kRecentPanelPad = 14;

inline constexpr int kRecentPanelItemHeight = 46;

inline constexpr int kRecentPanelItemGap = 8;

inline constexpr int kRecentPanelHeaderHeight = 22;

std::wstring recent_file_panel_item_text(const std::wstring& path);

std::pair<std::wstring, std::wstring> split_recent_file_panel_text(const wchar_t* text);

void draw_recent_file_panel_button(HDC dc, const RECT& r, const wchar_t* txt, bool pressed);

void update_recent_files_panel_items();

void layout_welcome_recent_files_panel(HWND owner);

void hide_welcome_recent_files_panel();

void show_welcome_recent_files_panel(HWND owner);

void toggle_welcome_recent_files_panel(HWND owner);

LRESULT CALLBACK WelcomeRecentPanelProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

void register_recent_files_panel_class(HWND owner);

LRESULT CALLBACK WelcomeProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

void show_welcome(HINSTANCE inst);

void raise_main_window();

} // namespace gui
