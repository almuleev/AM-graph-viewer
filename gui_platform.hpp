#pragma once

// Shared Win32 and C++ dependencies for the native GUI modules.
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
#include "sampling.hpp"
#include "data_io.hpp"
#include "filter_engine.hpp"
#include "spectrum_worker.hpp"
#include "minmax_index.hpp"
#include "export_helpers.hpp"
#include "gap_details.hpp"
#include "formula_engine.hpp"
#include "lvm_parser.hpp"
