#pragma once

#include <windows.h>

void release_backbuffer();
bool ensure_backbuffer(HDC hdc, int width, int height);
void on_paint(HDC hdc);
