#pragma once
#include "gui_platform.hpp"

namespace gui {

extern HWND g_loading_wnd;

extern HWND g_loading_cancel_btn;

extern std::wstring g_loading_text;

extern bool g_loading_cancellable;

LRESULT CALLBACK LoadingProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

void show_loading(const std::wstring& msg, bool cancellable = false);

void hide_loading();

inline constexpr UINT_PTR kDropForwardSubclassId = 0x4C564D01u;

LRESULT CALLBACK drop_forward_subclass_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                                            UINT_PTR, DWORD_PTR);

BOOL CALLBACK enable_file_drop_child_proc(HWND child, LPARAM);

void enable_file_drop_support(HWND hwnd);

void handle_file_drop(HWND hwnd, HDROP hDrop);

} // namespace gui
