#pragma once

#include <string>
#include <windows.h>

void request_async_load_cancel();
void show_loading(const std::wstring& msg, bool cancellable = false);
void hide_loading();
void enable_file_drop_support(HWND hwnd);
void handle_file_drop(HWND hwnd, HDROP hDrop);
