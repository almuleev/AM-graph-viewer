#pragma once

#include <string>
#include <vector>

#include <windows.h>

struct SettingsSnapshot;
struct UndoAction;

void push_undo(const UndoAction& a);
void pop_undo();
void pop_redo();
bool settings_snapshot_differs(const SettingsSnapshot& a, const SettingsSnapshot& b);
void apply_settings_snapshot(const SettingsSnapshot& snapshot);
bool record_settings_change(const SettingsSnapshot& before);
