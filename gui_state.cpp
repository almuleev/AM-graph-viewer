// State: native viewer implementation.
#include "gui_state.hpp"

namespace gui {

std::wstring g_config_path;

App g;

ULONG_PTR g_gdiplus_token = 0;

bool has_data() { return g.ds.ok && g.ds.rows() > 1 && g.ds.channel_count() > 0; }

} // namespace gui
