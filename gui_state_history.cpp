// ---- undo / redo system ------------------------------------------------
struct SettingsSnapshot {
    std::vector<char> visible;
    std::vector<std::wstring> channel_labels;
    std::vector<COLORREF> channel_colors;
    std::wstring global_formula;
    std::vector<std::wstring> channel_formulas;
    bool snap_to_data = true;
    COLORREF marker_color = RGB(0, 120, 215);
    std::vector<PointGroup> point_groups;
    int active_point_group = -1;
    std::vector<GuideLine> guides;
    std::vector<App::Marker> markers;
    int active_marker = -1;
    int pending_line = 0;
    bool pending_marker = false;
    bool measure_mode = false;
    bool auto_y = true;
    double y_lock_min = -1.0;
    double y_lock_max = 1.0;
    bool auto_y_amp = true;
    double y_amp_max = 1.0;
    bool noise_threshold_enabled = false;
    double noise_threshold_min = -std::numeric_limits<double>::infinity();
    double noise_threshold_max = std::numeric_limits<double>::infinity();
    int noise_threshold_mode = FilterModeBandPass;
    int noise_threshold_topology = FilterTopologyButterworth;
};

struct UndoAction {
    enum Type { NONE, ADD_POINT, ADD_LINE, ADD_MARKER, CLEAR_POINTS, CLEAR_LINES, CLEAR_MARKERS, SETTINGS_CHANGE } type = NONE;
    std::pair<double, double> point;
    int point_group_index = -1;
    bool point_group_created = false;
    PointGroup point_group_state;
    GuideLine line;
    App::Marker marker;
    std::vector<PointGroup> saved_point_groups;
    int saved_active_point_group = -1;
    std::vector<GuideLine> saved_lines;
    std::vector<App::Marker> saved_markers;
    SettingsSnapshot before_settings;
    SettingsSnapshot after_settings;
};
std::vector<UndoAction> g_undo;
std::vector<UndoAction> g_redo;
WNDPROC g_channel_edit_proc = nullptr;
void populate_point_group_list(HWND hwnd);
void refresh_side_panel_controls();
void refresh_settings_controls();
void apply_side_panel_visibility();
void set_side_panel_tab(int tab);
int side_panel_width();
void layout();
void update_side_panel_scrollbar(int viewport_top, int content_height);
bool side_panel_hit_test(const POINT& pt);
void scroll_side_panel(int delta);
void load_side_transform_controls();
std::wstring format_edit_number(double value);
std::wstring format_optional_edit_number(double value);
COLORREF mix_color(COLORREF a, COLORREF b, int weight_b);
void ensure_channel_formula_vectors();
void invalidate_formula_runtime();
void invalidate_formula_runtime_channel(std::size_t channel_index);
void invalidate_transformed_channel_cache();
void invalidate_filtered_channel_cache();
void normalize_filter_bounds();
double current_filter_sample_step();
double current_filter_nyquist();
double clamp_filter_cutoff(double hz, double nyquist);
double filter_slider_to_frequency(int pos, double nyquist);
int frequency_to_filter_slider(double hz, double nyquist);
std::wstring filter_frequency_text(double hz);
double transformed_channel_sample(std::size_t channel_index, std::size_t row_index);
double rendered_channel_sample(std::size_t channel_index, std::size_t row_index);
void ensure_filtered_channel_cache(std::size_t channel_index);
void ensure_transformed_channel_cache(std::size_t channel_index);
void load_channel_formulas_from_ini();
void finish_channel_rename(bool apply);
void save_runtime_settings();
void set_status();
void sync_menu();
void clear_spectrum_cache_state();
void invalidate_plot_analysis_cache();
void compute_spectrum();
void compute_spectrum_from_current_source();
bool ensure_current_spectrum();
double visible_spectrum_ymax();
SettingsSnapshot capture_settings_snapshot();
void ensure_channel_formula_storage();
void ensure_channel_formulas_loaded();
bool settings_snapshot_differs(const SettingsSnapshot& a, const SettingsSnapshot& b);
void apply_settings_snapshot(const SettingsSnapshot& snapshot);
bool record_settings_change(const SettingsSnapshot& before);
bool is_toggle_checked(HWND hwnd);
void set_toggle_checked(HWND hwnd, bool checked);
void toggle_checked_state(HWND hwnd);
std::wstring channel_display_label(std::size_t ci);
std::wstring normalize_axis_label_text(const std::wstring& text, const wchar_t* fallback);
int hit_test_gap_marker(int x, int y);
void hide_gap_details_card();
void show_gap_details_card(double duration, long long estimated_missing_samples);
void draw_gap_details_card(HDC dc, const RECT& plot);
void flush_runtime_settings_save();
void save_runtime_settings_now();
void apply_export_metadata_from_comments(const std::vector<std::string>& comments);

const wchar_t* gap_markers_toggle_text() {
    return (g_str == &kEn) ? L"Show gap markers" : L"Показывать разрывы";
}

const wchar_t* filter_toggle_text() {
    return (g_str == &kEn) ? L"Signal filter" : L"Фильтр сигнала";
}

const wchar_t* filter_low_cutoff_text() {
    return (g_str == &kEn) ? L"Low cutoff:" : L"Нижняя граница:";
}

const wchar_t* filter_high_cutoff_text() {
    return (g_str == &kEn) ? L"High cutoff:" : L"Верхняя граница:";
}

const wchar_t* filter_section_title_text() {
    return (g_str == &kEn) ? L"Filter" : L"Фильтр";
}

const wchar_t* filter_mode_label_text() {
    return (g_str == &kEn) ? L"Mode:" : L"Режим:";
}

const wchar_t* filter_topology_label_text() {
    return (g_str == &kEn) ? L"Topology:" : L"Топология:";
}

const wchar_t* filter_mode_lowpass_text() {
    return (g_str == &kEn) ? L"Low-pass" : L"НЧ";
}

const wchar_t* filter_mode_highpass_text() {
    return (g_str == &kEn) ? L"High-pass" : L"ВЧ";
}

const wchar_t* filter_mode_bandpass_text() {
    return (g_str == &kEn) ? L"Band-pass" : L"Полосовой";
}

const wchar_t* filter_mode_bandstop_text() {
    return (g_str == &kEn) ? L"Band-stop" : L"Режекторный";
}

const wchar_t* filter_topology_butterworth_text() {
    return (g_str == &kEn) ? L"Butterworth" : L"Баттерворт";
}

const wchar_t* filter_topology_bessel_text() {
    return (g_str == &kEn) ? L"Bessel" : L"Бессель";
}

const wchar_t* filter_topology_chebyshev_text() {
    return (g_str == &kEn) ? L"Chebyshev" : L"Чебышёв";
}

const wchar_t* filter_topology_linkwitz_text() {
    return (g_str == &kEn) ? L"Linkwitz-Riley" : L"Линквиц-Райли";
}

const wchar_t* side_global_formula_label_text() {
    return (g_str == &kEn) ? L"Global coefficient for all charts:" : L"Общий коэффициент для всех графиков:";
}

const wchar_t* side_global_formula_apply_text() {
    return (g_str == &kEn) ? L"Apply to all charts" : L"Применить ко всем графикам";
}

const wchar_t* side_channel_formula_label_text() {
    return (g_str == &kEn) ? L"Coefficient for the selected channel:" : L"Коэффициент выбранного канала:";
}

const wchar_t* point_group_list_title() {
    return g_str == &kEn ? L"Point groups" : L"Группы точек";
}

const wchar_t* point_current_color_button_text() {
    return g_str == &kEn ? L"Colour for new points…" : L"Цвет новых точек…";
}

const wchar_t* point_selected_group_color_button_text() {
    return g_str == &kEn ? L"Selected group colour…" : L"Цвет выбранной группы…";
}

const wchar_t* point_group_visible_text() {
    return g_str == &kEn ? L"Show selected group" : L"Показывать выбранную группу";
}

const wchar_t* point_group_new_button_text() {
    return g_str == &kEn ? L"Start new group" : L"Новая группа";
}

const wchar_t* point_group_empty_text() {
    return g_str == &kEn ? L"No point groups yet" : L"Групп точек пока нет";
}

const wchar_t* side_panel_button_text() {
    return (g_str == &kEn) ? L"Panel" : L"Панель";
}

const wchar_t* side_tab_channels_text() {
    return (g_str == &kEn) ? L"Channels" : L"Каналы";
}

const wchar_t* side_tab_points_text() {
    return (g_str == &kEn) ? L"Points" : L"Точки";
}

const wchar_t* side_tab_filter_text() {
    return (g_str == &kEn) ? L"Filter" : L"Фильтр";
}

const wchar_t* side_channel_color_button_text() {
    return (g_str == &kEn) ? L"Channel colour…" : L"Цвет канала…";
}

const wchar_t* side_channel_hint_text() {
    return filter_section_title_text();
}

const wchar_t* side_formula_apply_selected_text() {
    return (g_str == &kEn) ? L"Apply to selected" : L"К выбранному";
}

const wchar_t* side_formula_apply_visible_text() {
    return (g_str == &kEn) ? L"Apply to visible" : L"К видимым";
}

const wchar_t* side_formula_reset_selected_text() {
    return (g_str == &kEn) ? L"Reset selected" : L"Сбросить канал";
}

const wchar_t* side_formula_reset_all_text() {
    return (g_str == &kEn) ? L"Reset all channels" : L"Сбросить все каналы";
}

const wchar_t* side_point_group_delete_text() {
    return (g_str == &kEn) ? L"Delete group" : L"Удалить группу";
}

const wchar_t* side_point_group_rename_text() {
    return (g_str == &kEn) ? L"Rename" : L"Переименовать";
}

const wchar_t* side_pt_num_text() {
    return (g_str == &kEn) ? L"Point #" : L"Номер";
}

const wchar_t* side_pt_x_text() {
    return (g_str == &kEn) ? L"X" : L"X";
}

const wchar_t* side_pt_y_text() {
    return (g_str == &kEn) ? L"Y" : L"Y";
}

const wchar_t* side_pt_dx_text() {
    return (g_str == &kEn) ? L"Δx" : L"Δx";
}

const wchar_t* side_pt_dy_text() {
    return (g_str == &kEn) ? L"Δy" : L"Δy";
}

const wchar_t* side_pt_invdt_text() {
    return (g_str == &kEn) ? L"1/Δt" : L"1/Δt";
}

const wchar_t* side_pt_dist_text() {
    return (g_str == &kEn) ? L"d" : L"d";
}

const wchar_t* side_pt_snap_text() {
    return (g_str == &kEn) ? L"Snap" : L"Привязка";
}

const wchar_t* axis_x_label_text() {
    return (g_str == &kEn) ? L"X label:" : L"Буква X:";
}

const wchar_t* axis_y_label_text() {
    return (g_str == &kEn) ? L"Y label:" : L"Буква Y:";
}

#if 0
std::wstring default_channel_formula_text() {
    return L"x";
}

std::wstring normalize_formula_text(const std::wstring& text) {
    std::wstring out;
    out.reserve(text.size());
    for (wchar_t ch : text) {
        if (ch == L',') out.push_back(L'.');
        else out.push_back(ch);
    }
    std::size_t first = 0;
    while (first < out.size() && iswspace(out[first])) ++first;
    std::size_t last = out.size();
    while (last > first && iswspace(out[last - 1])) --last;
    out = out.substr(first, last - first);
    if (out.empty()) out = default_channel_formula_text();
    return out;
}

int formula_precedence(FormulaOp op) {
    switch (op) {
        case FormulaOp::Neg: return 3;
        case FormulaOp::Mul:
        case FormulaOp::Div: return 2;
        case FormulaOp::Add:
        case FormulaOp::Sub: return 1;
        default: return 0;
    }
}

bool formula_is_right_associative(FormulaOp op) {
    return op == FormulaOp::Neg;
}

bool formula_is_function(FormulaOp op) {
    return op == FormulaOp::FuncAbs || op == FormulaOp::FuncSqrt || op == FormulaOp::FuncSin ||
           op == FormulaOp::FuncCos || op == FormulaOp::FuncTan || op == FormulaOp::FuncLog ||
           op == FormulaOp::FuncExp;
}

bool formula_is_operator(FormulaOp op) {
    return op == FormulaOp::Add || op == FormulaOp::Sub || op == FormulaOp::Mul ||
           op == FormulaOp::Div || op == FormulaOp::Neg;
}

bool formula_function_from_name(const std::wstring& name, FormulaOp& out) {
    if (name == L"abs") { out = FormulaOp::FuncAbs; return true; }
    if (name == L"sqrt") { out = FormulaOp::FuncSqrt; return true; }
    if (name == L"sin") { out = FormulaOp::FuncSin; return true; }
    if (name == L"cos") { out = FormulaOp::FuncCos; return true; }
    if (name == L"tan") { out = FormulaOp::FuncTan; return true; }
    if (name == L"log") { out = FormulaOp::FuncLog; return true; }
    if (name == L"exp") { out = FormulaOp::FuncExp; return true; }
    return false;
}

bool compile_formula_rpn(const std::wstring& raw_text, std::vector<FormulaToken>& out, std::wstring& error) {
    const std::wstring text = normalize_formula_text(raw_text);
    struct StackEntry {
        FormulaOp op = FormulaOp::Add;
        bool is_lparen = false;
    };
    out.clear();
    std::vector<StackEntry> ops;
    bool expect_value = true;

    auto push_operator = [&](FormulaOp op) {
        while (!ops.empty() && !ops.back().is_lparen && formula_is_operator(ops.back().op)) {
            const int top_prec = formula_precedence(ops.back().op);
            const int cur_prec = formula_precedence(op);
            if (top_prec > cur_prec || (top_prec == cur_prec && !formula_is_right_associative(op))) {
                out.push_back({ops.back().op, 0.0});
                ops.pop_back();
            } else {
                break;
            }
        }
        ops.push_back({op, false});
    };

    std::size_t i = 0;
    while (i < text.size()) {
        const wchar_t ch = text[i];
        if (iswspace(ch)) { ++i; continue; }

        if ((ch >= L'0' && ch <= L'9') || ch == L'.') {
            const wchar_t* begin = text.c_str() + i;
            wchar_t* end = nullptr;
            double value = wcstod(begin, &end);
            if (begin == end) {
                error = (g_str == &kEn) ? L"Invalid number in coefficient." : L"Некорректное число в коэффициенте.";
                return false;
            }
            i = static_cast<std::size_t>(end - text.c_str());
            out.push_back({FormulaOp::Number, value});
            expect_value = false;
            continue;
        }

        if (iswalpha(ch)) {
            std::size_t j = i;
            while (j < text.size() && (iswalpha(text[j]) || iswdigit(text[j]) || text[j] == L'_')) ++j;
            std::wstring name = text.substr(i, j - i);
            for (wchar_t& c : name) c = static_cast<wchar_t>(towlower(c));
            if (name == L"x") {
                out.push_back({FormulaOp::Variable, 0.0});
                expect_value = false;
            } else {
                FormulaOp fn = FormulaOp::FuncAbs;
                if (!formula_function_from_name(name, fn)) {
                    error = ((g_str == &kEn) ? L"Unknown function: " : L"Неизвестная функция: ") + name;
                    return false;
                }
                ops.push_back({fn, false});
                expect_value = true;
            }
            i = j;
            continue;
        }

        if (ch == L'(') {
            ops.push_back({FormulaOp::Add, true});
            ++i;
            expect_value = true;
            continue;
        }
        if (ch == L')') {
            bool found_lparen = false;
            while (!ops.empty()) {
                if (ops.back().is_lparen) {
                    found_lparen = true;
                    ops.pop_back();
                    break;
                }
                out.push_back({ops.back().op, 0.0});
                ops.pop_back();
            }
            if (!found_lparen) {
                error = (g_str == &kEn) ? L"Mismatched parentheses." : L"Несогласованные скобки.";
                return false;
            }
            if (!ops.empty() && formula_is_function(ops.back().op)) {
                out.push_back({ops.back().op, 0.0});
                ops.pop_back();
            }
            ++i;
            expect_value = false;
            continue;
        }

        FormulaOp op = FormulaOp::Add;
        if (ch == L'+') op = FormulaOp::Add;
        else if (ch == L'-') op = expect_value ? FormulaOp::Neg : FormulaOp::Sub;
        else if (ch == L'*') op = FormulaOp::Mul;
        else if (ch == L'/') op = FormulaOp::Div;
        else {
            error = ((g_str == &kEn) ? L"Unsupported character in coefficient: " : L"Недопустимый символ в коэффициенте: ") + std::wstring(1, ch);
            return false;
        }
        if (expect_value && op != FormulaOp::Neg) {
            error = (g_str == &kEn) ? L"Operator position is invalid." : L"Некорректное положение оператора.";
            return false;
        }
        push_operator(op);
        ++i;
        expect_value = true;
    }

    if (expect_value) {
        error = (g_str == &kEn) ? L"Incomplete coefficient." : L"Коэффициент не завершён.";
        return false;
    }

    while (!ops.empty()) {
        if (ops.back().is_lparen) {
            error = (g_str == &kEn) ? L"Mismatched parentheses." : L"Несогласованные скобки.";
            return false;
        }
        out.push_back({ops.back().op, 0.0});
        ops.pop_back();
    }
    return !out.empty();
}

bool formula_rpn_is_identity(const std::vector<FormulaToken>& rpn) {
    return rpn.size() == 1 && rpn[0].op == FormulaOp::Variable;
}

AffineFormulaInfo analyze_formula_rpn_affine(const std::vector<FormulaToken>& rpn) {
    if (rpn.empty()) return {};

    std::vector<AffineFormulaInfo> stack;
    stack.reserve(rpn.size());
    auto is_constant = [](const AffineFormulaInfo& value) {
        return value.valid && value.mul == 0.0;
    };
    auto push = [&](AffineFormulaInfo value) -> bool {
        if (!value.valid) return false;
        stack.push_back(value);
        return true;
    };
    auto pop1 = [&]() -> AffineFormulaInfo {
        if (stack.empty()) return {};
        AffineFormulaInfo value = stack.back();
        stack.pop_back();
        return value;
    };
    auto pop2 = [&](AffineFormulaInfo& a, AffineFormulaInfo& b) -> bool {
        if (stack.size() < 2) return false;
        b = stack.back();
        stack.pop_back();
        a = stack.back();
        stack.pop_back();
        return true;
    };
    auto push_constant_fn = [&](const AffineFormulaInfo& operand, double (*fn)(double)) -> bool {
        return is_constant(operand) && push({true, 0.0, fn(operand.add)});
    };

    for (const auto& token : rpn) {
        switch (token.op) {
            case FormulaOp::Number:
                if (!push({true, 0.0, token.value})) return {};
                break;
            case FormulaOp::Variable:
                if (!push({true, 1.0, 0.0})) return {};
                break;
            case FormulaOp::Add: {
                AffineFormulaInfo a, b;
                if (!pop2(a, b) || !push({a.valid && b.valid, a.mul + b.mul, a.add + b.add})) return {};
                break;
            }
            case FormulaOp::Sub: {
                AffineFormulaInfo a, b;
                if (!pop2(a, b) || !push({a.valid && b.valid, a.mul - b.mul, a.add - b.add})) return {};
                break;
            }
            case FormulaOp::Neg: {
                AffineFormulaInfo v = pop1();
                if (!push({v.valid, -v.mul, -v.add})) return {};
                break;
            }
            case FormulaOp::Mul: {
                AffineFormulaInfo a, b;
                if (!pop2(a, b)) return {};
                if (is_constant(a) && b.valid) {
                    if (!push({true, a.add * b.mul, a.add * b.add})) return {};
                } else if (is_constant(b) && a.valid) {
                    if (!push({true, b.add * a.mul, b.add * a.add})) return {};
                } else {
                    return {};
                }
                break;
            }
            case FormulaOp::Div: {
                AffineFormulaInfo a, b;
                if (!pop2(a, b) || !a.valid || !is_constant(b)) return {};
                const double denom = b.add;
                const double inv = (denom == 0.0) ? std::numeric_limits<double>::quiet_NaN() : (1.0 / denom);
                if (!push({true, a.mul * inv, a.add * inv})) return {};
                break;
            }
            case FormulaOp::FuncAbs: {
                AffineFormulaInfo v = pop1();
                if (!push_constant_fn(v, [](double x) { return std::fabs(x); })) return {};
                break;
            }
            case FormulaOp::FuncSqrt: {
                AffineFormulaInfo v = pop1();
                if (!push_constant_fn(v, [](double x) {
                    return x < 0.0 ? std::numeric_limits<double>::quiet_NaN() : std::sqrt(x);
                })) return {};
                break;
            }
            case FormulaOp::FuncSin: {
                AffineFormulaInfo v = pop1();
                if (!push_constant_fn(v, [](double x) { return std::sin(x); })) return {};
                break;
            }
            case FormulaOp::FuncCos: {
                AffineFormulaInfo v = pop1();
                if (!push_constant_fn(v, [](double x) { return std::cos(x); })) return {};
                break;
            }
            case FormulaOp::FuncTan: {
                AffineFormulaInfo v = pop1();
                if (!push_constant_fn(v, [](double x) { return std::tan(x); })) return {};
                break;
            }
            case FormulaOp::FuncLog: {
                AffineFormulaInfo v = pop1();
                if (!push_constant_fn(v, [](double x) {
                    return x <= 0.0 ? std::numeric_limits<double>::quiet_NaN() : std::log(x);
                })) return {};
                break;
            }
            case FormulaOp::FuncExp: {
                AffineFormulaInfo v = pop1();
                if (!push_constant_fn(v, [](double x) { return std::exp(x); })) return {};
                break;
            }
        }
    }

    return stack.size() == 1 ? stack.back() : AffineFormulaInfo{};
}

double eval_formula_rpn(const std::vector<FormulaToken>& rpn, double x) {
    if (rpn.empty()) return std::numeric_limits<double>::quiet_NaN();
    if (formula_rpn_is_identity(rpn)) return x;

    constexpr std::size_t kInlineStackCap = 32;
    std::array<double, kInlineStackCap> inline_stack{};
    std::vector<double> heap_stack;
    double* stack = inline_stack.data();
    std::size_t capacity = kInlineStackCap;
    if (rpn.size() > kInlineStackCap) {
        heap_stack.resize(rpn.size());
        stack = heap_stack.data();
        capacity = heap_stack.size();
    }

    std::size_t sp = 0;
    auto push = [&](double v) -> bool {
        if (sp >= capacity) return false;
        stack[sp++] = v;
        return true;
    };
    auto pop1 = [&]() -> double {
        if (sp == 0) return std::numeric_limits<double>::quiet_NaN();
        return stack[--sp];
    };
    auto pop2 = [&](double& a, double& b) -> bool {
        if (sp < 2) return false;
        b = stack[--sp];
        a = stack[--sp];
        return true;
    };

    for (const auto& token : rpn) {
        switch (token.op) {
            case FormulaOp::Number: if (!push(token.value)) return std::numeric_limits<double>::quiet_NaN(); break;
            case FormulaOp::Variable: if (!push(x)) return std::numeric_limits<double>::quiet_NaN(); break;
            case FormulaOp::Add: {
                double a, b;
                if (!pop2(a, b)) return std::numeric_limits<double>::quiet_NaN();
                if (!push(a + b)) return std::numeric_limits<double>::quiet_NaN();
                break;
            }
            case FormulaOp::Sub: {
                double a, b;
                if (!pop2(a, b)) return std::numeric_limits<double>::quiet_NaN();
                if (!push(a - b)) return std::numeric_limits<double>::quiet_NaN();
                break;
            }
            case FormulaOp::Mul: {
                double a, b;
                if (!pop2(a, b)) return std::numeric_limits<double>::quiet_NaN();
                if (!push(a * b)) return std::numeric_limits<double>::quiet_NaN();
                break;
            }
            case FormulaOp::Div: {
                double a, b;
                if (!pop2(a, b)) return std::numeric_limits<double>::quiet_NaN();
                if (!push(b == 0.0 ? std::numeric_limits<double>::quiet_NaN() : (a / b))) {
                    return std::numeric_limits<double>::quiet_NaN();
                }
                break;
            }
            case FormulaOp::Neg: {
                double v = pop1();
                if (!std::isfinite(v) && !std::isnan(v)) return std::numeric_limits<double>::quiet_NaN();
                if (!push(-v)) return std::numeric_limits<double>::quiet_NaN();
                break;
            }
            case FormulaOp::FuncAbs: {
                double v = pop1(); if (!push(std::fabs(v))) return std::numeric_limits<double>::quiet_NaN(); break;
            }
            case FormulaOp::FuncSqrt: {
                double v = pop1(); if (!push(v < 0.0 ? std::numeric_limits<double>::quiet_NaN() : std::sqrt(v))) return std::numeric_limits<double>::quiet_NaN(); break;
            }
            case FormulaOp::FuncSin: { double v = pop1(); if (!push(std::sin(v))) return std::numeric_limits<double>::quiet_NaN(); break; }
            case FormulaOp::FuncCos: { double v = pop1(); if (!push(std::cos(v))) return std::numeric_limits<double>::quiet_NaN(); break; }
            case FormulaOp::FuncTan: { double v = pop1(); if (!push(std::tan(v))) return std::numeric_limits<double>::quiet_NaN(); break; }
            case FormulaOp::FuncLog: {
                double v = pop1(); if (!push(v <= 0.0 ? std::numeric_limits<double>::quiet_NaN() : std::log(v))) return std::numeric_limits<double>::quiet_NaN(); break;
            }
            case FormulaOp::FuncExp: { double v = pop1(); if (!push(std::exp(v))) return std::numeric_limits<double>::quiet_NaN(); break; }
        }
    }
    return sp == 1 ? stack[0] : std::numeric_limits<double>::quiet_NaN();
}
#endif

void normalize_active_point_group() {
    if (g.point_groups.empty()) {
        g.active_point_group = -1;
        return;
    }
    if (g.active_point_group >= 0 &&
        g.active_point_group < static_cast<int>(g.point_groups.size())) {
        return;
    }
    g.active_point_group = static_cast<int>(g.point_groups.size()) - 1;
}

PointGroup* active_point_group() {
    normalize_active_point_group();
    if (g.active_point_group < 0) return nullptr;
    return &g.point_groups[static_cast<std::size_t>(g.active_point_group)];
}

const PointGroup* active_point_group_readonly() {
    normalize_active_point_group();
    if (g.active_point_group < 0) return nullptr;
    return &g.point_groups[static_cast<std::size_t>(g.active_point_group)];
}

std::size_t total_measure_point_count() {
    std::size_t total = 0;
    for (const auto& group : g.point_groups) total += group.points.size();
    return total;
}

bool has_measure_points() {
    return total_measure_point_count() != 0;
}

void clear_measure_point_groups() {
    g.point_groups.clear();
    g.active_point_group = -1;
}

PointDisplay* active_point_display() {
    if (PointGroup* group = active_point_group()) return &group->display;
    return nullptr;
}

const PointDisplay* active_point_display_readonly() {
    if (const PointGroup* group = active_point_group_readonly()) return &group->display;
    return nullptr;
}

void sync_point_display_from_active_group() {
    if (const PointDisplay* display = active_point_display_readonly()) {
        g.pdisp = *display;
    }
}

void erase_point_group(std::size_t index) {
    if (index >= g.point_groups.size()) return;
    g.point_groups.erase(g.point_groups.begin() + static_cast<std::ptrdiff_t>(index));
    if (g.point_groups.empty()) {
        g.active_point_group = -1;
        return;
    }
    if (g.active_point_group > static_cast<int>(index)) {
        --g.active_point_group;
    } else if (g.active_point_group == static_cast<int>(index)) {
        g.active_point_group = static_cast<int>(std::min<std::size_t>(index, g.point_groups.size() - 1));
    }
    sync_point_display_from_active_group();
}

int create_point_group(COLORREF color) {
    PointGroup group;
    const std::size_t index = g.point_groups.size();
    if (g_str == &kEn) group.name = L"Group " + std::to_wstring(index + 1);
    else group.name = L"Группа " + std::to_wstring(index + 1);
    group.color = color;
    group.visible = true;
    if (const PointDisplay* active_display = active_point_display_readonly()) {
        group.display = *active_display;
    } else {
        group.display = g.pdisp;
    }
    g.point_groups.push_back(group);
    g.active_point_group = static_cast<int>(g.point_groups.size()) - 1;
    sync_point_display_from_active_group();
    return g.active_point_group;
}

int ensure_point_group_for_measurement(bool force_new_group, bool* created_group = nullptr) {
    bool created = false;
    normalize_active_point_group();
    PointGroup* group = active_point_group();
    if (!group) {
        create_point_group(g.marker_color);
        created = true;
        group = active_point_group();
    } else {
        const bool active_has_points = !group->points.empty();
        if (force_new_group || (active_has_points && group->color != g.marker_color)) {
            create_point_group(g.marker_color);
            created = true;
            group = active_point_group();
        } else if (!active_has_points) {
            group->color = g.marker_color;
        }
    }
    if (group) group->visible = true;
    if (created_group) *created_group = created;
    return g.active_point_group;
}

std::wstring point_group_list_label(std::size_t index, const PointGroup& group) {
    wchar_t buf[160];
    const std::wstring base_name = group.name.empty()
        ? ((g_str == &kEn) ? (L"Group " + std::to_wstring(index + 1)) : (L"Группа " + std::to_wstring(index + 1)))
        : group.name;
    const wchar_t* status = L"";
    if (!group.visible) {
        status = (g_str == &kEn) ? L" [hidden]" : L" [скрыта]";
    } else if (static_cast<int>(index) == g.active_point_group) {
        status = (g_str == &kEn) ? L" [active]" : L" [активна]";
    }
    if (g_str == &kEn) {
        swprintf(buf, 160, L"%ls — %zu pts%s", base_name.c_str(), group.points.size(), status);
    } else {
        swprintf(buf, 160, L"%ls — %zu тчк%s", base_name.c_str(), group.points.size(), status);
    }
    return buf;
}

std::wstring measure_points_status_text() {
    const std::size_t total_points = total_measure_point_count();
    if (!total_points) return L"";
    std::size_t visible_groups = 0;
    for (const auto& group : g.point_groups) {
        if (group.visible && !group.points.empty()) ++visible_groups;
    }
    wchar_t buf[160];
    if (g_str == &kEn) {
        swprintf(buf, 160, L"   |   Points: %zu in %zu groups (%zu visible)",
            total_points, g.point_groups.size(), visible_groups);
    } else {
        swprintf(buf, 160, L"   |   Точки: %zu в %zu группах (%zu видимых)",
            total_points, g.point_groups.size(), visible_groups);
    }
    return buf;
}

void push_undo(const UndoAction& a) {
    g_undo.push_back(a);
    g_redo.clear(); // new action clears redo stack
}

SettingsSnapshot capture_settings_snapshot() {
    ensure_channel_formula_vectors();
    SettingsSnapshot snapshot;
    snapshot.visible = g.visible;
    snapshot.channel_labels = g.channel_labels;
    snapshot.channel_colors = g_channel_colors;
    snapshot.global_formula = g.global_formula;
    snapshot.channel_formulas = g.channel_formulas;
    snapshot.snap_to_data = g.snap_to_data;
    snapshot.noise_threshold_enabled = g.noise_threshold_enabled;
    snapshot.noise_threshold_min = g.noise_threshold_min;
    snapshot.noise_threshold_max = g.noise_threshold_max;
    snapshot.noise_threshold_mode = g.noise_threshold_mode;
    snapshot.noise_threshold_topology = g.noise_threshold_topology;
    snapshot.marker_color = g.marker_color;
    snapshot.point_groups = g.point_groups;
    snapshot.active_point_group = g.active_point_group;
    snapshot.guides = g.guides;
    snapshot.markers = g.markers;
    snapshot.active_marker = g.active_marker;
    snapshot.pending_line = g.pending_line;
    snapshot.pending_marker = g.pending_marker;
    snapshot.measure_mode = g.measure_mode;
    snapshot.auto_y = g.auto_y;
    snapshot.y_lock_min = g.y_lock_min;
    snapshot.y_lock_max = g.y_lock_max;
    snapshot.auto_y_amp = g.auto_y_amp;
    snapshot.y_amp_max = g.y_amp_max;
    return snapshot;
}

bool settings_snapshot_differs(const SettingsSnapshot& a, const SettingsSnapshot& b) {
    auto same_point_groups = [&](const std::vector<PointGroup>& lhs, const std::vector<PointGroup>& rhs) {
        if (lhs.size() != rhs.size()) return false;
        for (std::size_t i = 0; i < lhs.size(); ++i) {
            if (lhs[i].name != rhs[i].name ||
                lhs[i].color != rhs[i].color ||
                lhs[i].visible != rhs[i].visible ||
                lhs[i].display.number != rhs[i].display.number ||
                lhs[i].display.x != rhs[i].display.x ||
                lhs[i].display.y != rhs[i].display.y ||
                lhs[i].display.dx != rhs[i].display.dx ||
                lhs[i].display.dy != rhs[i].display.dy ||
                lhs[i].display.inv_dt != rhs[i].display.inv_dt ||
                lhs[i].display.dist != rhs[i].display.dist ||
                lhs[i].points != rhs[i].points) {
                return false;
            }
        }
        return true;
    };
    auto same_guides = [&](const std::vector<GuideLine>& lhs, const std::vector<GuideLine>& rhs) {
        if (lhs.size() != rhs.size()) return false;
        for (std::size_t i = 0; i < lhs.size(); ++i) {
            if (lhs[i].vertical != rhs[i].vertical ||
                lhs[i].value != rhs[i].value ||
                lhs[i].freq != rhs[i].freq) {
                return false;
            }
        }
        return true;
    };
    auto same_markers = [&](const std::vector<App::Marker>& lhs, const std::vector<App::Marker>& rhs) {
        if (lhs.size() != rhs.size()) return false;
        for (std::size_t i = 0; i < lhs.size(); ++i) {
            if (lhs[i].x != rhs[i].x ||
                lhs[i].y != rhs[i].y ||
                lhs[i].label != rhs[i].label ||
                lhs[i].freq != rhs[i].freq ||
                lhs[i].snapped != rhs[i].snapped ||
                lhs[i].channel != rhs[i].channel) {
                return false;
            }
        }
        return true;
    };
    return a.visible != b.visible ||
           a.channel_labels != b.channel_labels ||
           a.channel_colors != b.channel_colors ||
           a.global_formula != b.global_formula ||
           a.channel_formulas != b.channel_formulas ||
           a.snap_to_data != b.snap_to_data ||
           a.noise_threshold_enabled != b.noise_threshold_enabled ||
           a.noise_threshold_min != b.noise_threshold_min ||
           a.noise_threshold_max != b.noise_threshold_max ||
           a.noise_threshold_mode != b.noise_threshold_mode ||
           a.noise_threshold_topology != b.noise_threshold_topology ||
           a.marker_color != b.marker_color ||
           !same_point_groups(a.point_groups, b.point_groups) ||
           a.active_point_group != b.active_point_group ||
           !same_guides(a.guides, b.guides) ||
           !same_markers(a.markers, b.markers) ||
           a.active_marker != b.active_marker ||
           a.pending_line != b.pending_line ||
           a.pending_marker != b.pending_marker ||
           a.measure_mode != b.measure_mode ||
           a.auto_y != b.auto_y ||
           a.y_lock_min != b.y_lock_min ||
           a.y_lock_max != b.y_lock_max ||
           a.auto_y_amp != b.auto_y_amp ||
           a.y_amp_max != b.y_amp_max;
}

void sync_channel_controls_from_state() {
    for (std::size_t i = 0; i < g.checks.size() && i < g.visible.size(); ++i) {
        if (g.checks[i]) {
            set_toggle_checked(g.checks[i], g.visible[i] != 0);
        }
    }
    for (std::size_t i = 0; i < g.check_labels.size() && i < g.ds.channel_count(); ++i) {
        if (g.check_labels[i]) {
            const std::wstring label = channel_display_label(i);
            SetWindowTextW(g.check_labels[i], label.c_str());
        }
    }
    if (g.measure) {
        SendMessageW(g.measure, BM_SETCHECK, g.measure_mode ? BST_CHECKED : BST_UNCHECKED, 0);
    }
    if (g.autoy) {
        SendMessageW(g.autoy, BM_SETCHECK, g.auto_y ? BST_CHECKED : BST_UNCHECKED, 0);
    }
}

void rebuild_formula_cache_from_state() {
    g.global_formula_rpn.clear();
    g.channel_formula_rpn.assign(g.channel_formulas.size(), {});
    invalidate_formula_runtime();
    ensure_channel_formula_vectors();
}

void recompute_transforms_from_state() {
    invalidate_plot_analysis_cache();
    invalidate_filtered_channel_cache();
    clear_spectrum_cache_state();
    if (g.freq_mode) compute_spectrum();
    sync_menu();
}

void apply_settings_snapshot(const SettingsSnapshot& snapshot) {
    if (g.channel_edit) finish_channel_rename(false);
    g.visible = snapshot.visible;
    g.channel_labels = snapshot.channel_labels;
    g_channel_colors = snapshot.channel_colors;
    g.global_formula = snapshot.global_formula;
    g.channel_formulas = snapshot.channel_formulas;
    rebuild_formula_cache_from_state();
    g.snap_to_data = snapshot.snap_to_data;
    g.noise_threshold_enabled = snapshot.noise_threshold_enabled;
    g.noise_threshold_min = snapshot.noise_threshold_min;
    g.noise_threshold_max = snapshot.noise_threshold_max;
    g.noise_threshold_mode = snapshot.noise_threshold_mode;
    g.noise_threshold_topology = snapshot.noise_threshold_topology;
    normalize_filter_bounds();
    g.marker_color = snapshot.marker_color;
    g.point_groups = snapshot.point_groups;
    g.active_point_group = snapshot.active_point_group;
    sync_point_display_from_active_group();
    g.guides = snapshot.guides;
    g.markers = snapshot.markers;
    g.active_marker = snapshot.active_marker;
    g.pending_line = snapshot.pending_line;
    g.pending_marker = snapshot.pending_marker;
    g.measure_mode = snapshot.measure_mode;
    g.auto_y = snapshot.auto_y;
    g.y_lock_min = snapshot.y_lock_min;
    g.y_lock_max = snapshot.y_lock_max;
    g.auto_y_amp = snapshot.auto_y_amp;
    g.y_amp_max = snapshot.y_amp_max;
    sync_channel_controls_from_state();
    recompute_transforms_from_state();
    if (g.settings_wnd) refresh_settings_controls();
    refresh_side_panel_controls();
    set_status();
    InvalidateRect(g.main, nullptr, TRUE);
    save_runtime_settings();
}

bool record_settings_change(const SettingsSnapshot& before) {
    const SettingsSnapshot after = capture_settings_snapshot();
    if (!settings_snapshot_differs(before, after)) return false;
    UndoAction action;
    action.type = UndoAction::SETTINGS_CHANGE;
    action.before_settings = before;
    action.after_settings = after;
    push_undo(action);
    return true;
}

void pop_undo() {
    if (g_undo.empty()) return;
    UndoAction a = g_undo.back();
    g_undo.pop_back();
    switch (a.type) {
        case UndoAction::ADD_POINT:
            if (a.point_group_index >= 0 &&
                a.point_group_index < static_cast<int>(g.point_groups.size())) {
                auto& pts = g.point_groups[static_cast<std::size_t>(a.point_group_index)].points;
                if (!pts.empty()) {
                    g_redo.push_back(a);
                    pts.pop_back();
                    if (a.point_group_created && pts.empty()) {
                        erase_point_group(static_cast<std::size_t>(a.point_group_index));
                    } else {
                        g.active_point_group = a.point_group_index;
                    }
                    if (PointGroup* group = active_point_group()) g.marker_color = group->color;
                }
            }
            break;
        case UndoAction::ADD_LINE: {
            auto it = std::find_if(g.guides.begin(), g.guides.end(), [&](const GuideLine& gl) {
                return gl.vertical == a.line.vertical && gl.value == a.line.value && gl.freq == a.line.freq;
            });
            if (it != g.guides.end()) {
                g_redo.push_back(a);
                g.guides.erase(it);
            }
            break;
        }
        case UndoAction::ADD_MARKER: {
            auto it = std::find_if(g.markers.begin(), g.markers.end(), [&](const App::Marker& m) {
                return m.x == a.marker.x && m.freq == a.marker.freq && m.label == a.marker.label;
            });
            if (it != g.markers.end()) {
                g_redo.push_back(a);
                g.markers.erase(it);
            }
            break;
        }
        case UndoAction::CLEAR_POINTS:
            g_redo.push_back(a);
            g.point_groups = a.saved_point_groups;
            g.active_point_group = a.saved_active_point_group;
            if (PointGroup* group = active_point_group()) g.marker_color = group->color;
            break;
        case UndoAction::CLEAR_LINES:
            g_redo.push_back(a);
            g.guides = a.saved_lines;
            break;
        case UndoAction::CLEAR_MARKERS:
            g_redo.push_back(a);
            g.markers = a.saved_markers;
            break;
        case UndoAction::SETTINGS_CHANGE:
            g_redo.push_back(a);
            apply_settings_snapshot(a.before_settings);
            break;
        default: break;
    }
    sync_point_display_from_active_group();
    if (g.settings_wnd) populate_point_group_list(g.settings_wnd);
    refresh_side_panel_controls();
}
void pop_redo() {
    if (g_redo.empty()) return;
    UndoAction a = g_redo.back();
    g_redo.pop_back();
    switch (a.type) {
        case UndoAction::ADD_POINT:
            if (a.point_group_created) {
                const std::size_t insert_at = (a.point_group_index >= 0 &&
                                               a.point_group_index <= static_cast<int>(g.point_groups.size()))
                    ? static_cast<std::size_t>(a.point_group_index)
                    : g.point_groups.size();
                PointGroup group = a.point_group_state;
                group.points.clear();
                g.point_groups.insert(g.point_groups.begin() + static_cast<std::ptrdiff_t>(insert_at), group);
            }
            if (a.point_group_index >= 0 &&
                a.point_group_index < static_cast<int>(g.point_groups.size())) {
                g.point_groups[static_cast<std::size_t>(a.point_group_index)].points.push_back(a.point);
                g.active_point_group = a.point_group_index;
                g.marker_color = g.point_groups[static_cast<std::size_t>(a.point_group_index)].color;
                g_undo.push_back(a);
            }
            break;
        case UndoAction::ADD_LINE:
            g.guides.push_back(a.line);
            g_undo.push_back(a);
            break;
        case UndoAction::ADD_MARKER:
            g.markers.push_back(a.marker);
            g_undo.push_back(a);
            break;
        case UndoAction::CLEAR_POINTS:
            g_undo.push_back(a);
            clear_measure_point_groups();
            break;
        case UndoAction::CLEAR_LINES:
            g_undo.push_back(a);
            g.guides.clear();
            break;
        case UndoAction::CLEAR_MARKERS:
            g_undo.push_back(a);
            g.markers.clear();
            break;
        case UndoAction::SETTINGS_CHANGE:
            g_undo.push_back(a);
            apply_settings_snapshot(a.after_settings);
            break;
        default: break;
    }
    sync_point_display_from_active_group();
    if (g.settings_wnd) populate_point_group_list(g.settings_wnd);
    refresh_side_panel_controls();
}

