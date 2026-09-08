#include "gui_frf.hpp"
#include "gui_analysis_source.hpp"
#include "gui_state.hpp"
#include "gui_text.hpp"
#include "gui_processing.hpp"
#include "gui_render.hpp"
#include "gui_status.hpp"
#include "gui_menu.hpp"
#include "gui_controls.hpp"
#include "gui_side_panel.hpp"
#include "gui_ids.hpp"
#include "gui_export.hpp"

namespace gui {
lvm::FrfWorker g_frf_worker;
void show_frf_channel_picker(bool supports);
bool handle_frf_picker_command(int id);
namespace {
enum {
    InputLabel = 7100, Input, OutputLabel, Output, Processing,
    LowLabel, Low, HighLabel, High, ApplyRange, Method, Source,
    Calculate, Csv, Png, Hint, EstimatorLabel, Estimator, LengthLabel, Length,
    SmoothingLabel, Smoothing
};
constexpr double smoothing_choices[] = {0, 1.0/24.0, 1.0/12.0, 1.0/6.0, 1.0/3.0};
int smoothing_choice(double octaves) {
    for (int i=0;i<5;++i)
        if (std::fabs(octaves-smoothing_choices[i])<1e-12) return i;
    return 0;
}
HWND control(int id) { return g.frf_panel ? GetDlgItem(g.frf_panel, id) : nullptr; }
const wchar_t* tr(const wchar_t* en, const wchar_t* ru) { return g_str == &kEn ? en : ru; }
void label(int id, const wchar_t* value) { if (HWND h = control(id)) SetWindowTextW(h, value); }
bool read_segment_length() {
    wchar_t text[64]{};
    GetWindowTextW(control(Length), text, 64);
    double value=0;
    if (!parse_wide_double_text(text,value) || !std::isfinite(value) || value<0 || value>100000000 ||
        std::floor(value)!=value || (value!=0 && (value<4 || std::fmod(value,2)!=0))) {
        MessageBoxW(g.frf_panel, tr(L"Use 0 (Auto), or an even L >= 4.", L"Укажите 0 (Auto) или чётное L ≥ 4."), L"FRF", MB_OK);
        return false;
    }
    g.frf.options.segment_length=static_cast<std::size_t>(value);
    return true;
}
LRESULT CALLBACK FrfPanelProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CREATE: {
            g.frf_panel = hwnd;
            HINSTANCE inst = reinterpret_cast<CREATESTRUCTW*>(lp)->hInstance;
            auto make = [&](int id, const wchar_t* cls, DWORD style, int x, int y, int w, int h) {
                HWND c = CreateWindowExW(0, cls, L"", WS_CHILD | WS_VISIBLE | style, x, y, w, h,
                    hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), inst, nullptr);
                SendMessageW(c, WM_SETFONT, reinterpret_cast<WPARAM>(g.ui_font), TRUE);
            };
            make(InputLabel, L"STATIC", SS_LEFT, 12, 4, 278, 18);
            make(Input, L"BUTTON", BS_OWNERDRAW | WS_TABSTOP, 12, 22, 278, 24);
            make(OutputLabel, L"STATIC", SS_LEFT, 12, 50, 278, 18);
            make(Output, L"BUTTON", BS_OWNERDRAW | WS_TABSTOP, 12, 68, 278, 24);
            make(Processing, L"BUTTON", BS_OWNERDRAW | WS_TABSTOP, 12, 96, 278, 24);
            make(EstimatorLabel, L"STATIC", SS_LEFT, 12, 124, 130, 18);
            make(LengthLabel, L"STATIC", SS_LEFT, 156, 124, 134, 18);
            make(Estimator, L"COMBOBOX", CBS_DROPDOWNLIST | WS_TABSTOP, 12, 144, 130, 200);
            make(Length, L"EDIT", WS_BORDER | ES_NUMBER | WS_TABSTOP, 156, 144, 134, 24);
            make(Method, L"STATIC", SS_LEFT, 12, 174, 278, 48);
            make(SmoothingLabel, L"STATIC", SS_LEFT, 12, 224, 130, 18);
            make(Smoothing, L"COMBOBOX", CBS_DROPDOWNLIST | WS_TABSTOP, 156, 222, 134, 200);
            make(Source, L"STATIC", SS_LEFT, 12, 250, 278, 32);
            make(LowLabel, L"STATIC", SS_LEFT, 12, 284, 130, 18);
            make(HighLabel, L"STATIC", SS_LEFT, 156, 284, 134, 18);
            make(Low, L"EDIT", WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 12, 304, 130, 24);
            make(High, L"EDIT", WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 156, 304, 134, 24);
            make(ApplyRange, L"BUTTON", BS_OWNERDRAW | WS_TABSTOP, 12, 332, 278, 24);
            make(Calculate, L"BUTTON", BS_OWNERDRAW | WS_TABSTOP, 12, 362, 122, 24);
            make(Csv, L"BUTTON", BS_OWNERDRAW | WS_TABSTOP, 140, 362, 72, 24);
            make(Png, L"BUTTON", BS_OWNERDRAW | WS_TABSTOP, 218, 362, 72, 24);
            make(Hint, L"STATIC", SS_LEFT, 12, 392, 278, 42);
            auto make_picker = [&](int id, const wchar_t* cls, DWORD style, int x, int y, int w, int h) {
                HWND c=CreateWindowExW(0,cls,L"",WS_CHILD|style,x,y,w,h,hwnd,
                    reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),inst,nullptr);
                SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(g.ui_font),TRUE);
            };
            make_picker(7300,L"STATIC",SS_LEFT,12,98,278,18);
            make_picker(7306,L"STATIC",SS_LEFT,12,118,278,32);
            make_picker(7307,L"STATIC",SS_LEFT,12,152,134,18);
            make_picker(7308,L"STATIC",SS_LEFT,156,152,134,18);
            make_picker(7301,L"LISTBOX",WS_BORDER|WS_VSCROLL|LBS_EXTENDEDSEL|LBS_NOTIFY|WS_TABSTOP,12,172,134,140);
            make_picker(7302,L"LISTBOX",WS_BORDER|WS_VSCROLL|LBS_EXTENDEDSEL|LBS_NOTIFY|WS_TABSTOP,156,172,134,140);
            make_picker(7303,L"BUTTON",BS_PUSHBUTTON|WS_TABSTOP,12,324,100,26);
            make_picker(7304,L"BUTTON",BS_DEFPUSHBUTTON|WS_TABSTOP,124,324,80,26);
            make_picker(7305,L"BUTTON",BS_PUSHBUTTON|WS_TABSTOP,212,324,78,26);
            SendMessageW(control(Estimator), CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"H1 (Welch)"));
            SendMessageW(control(Estimator), CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Direct Y/X"));
            for (const wchar_t* choice : {L"Off",L"1/24 octave",L"1/12 octave",L"1/6 octave",L"1/3 octave"})
                SendMessageW(control(Smoothing),CB_ADDSTRING,0,reinterpret_cast<LPARAM>(choice));
            refresh_frf_controls(true);
            return 0;
        }
        case WM_COMMAND: {
            const int id = LOWORD(wp), code = HIWORD(wp);
            if (id==Estimator && code==CBN_SELCHANGE) {
                if (!read_segment_length()) {
                    SendMessageW(control(Estimator),CB_SETCURSEL,g.frf.options.estimator==lvm::FrfEstimator::H1 ? 0 : 1,0);
                    return 0;
                }
                g.frf.options.estimator=SendMessageW(control(Estimator),CB_GETCURSEL,0,0)==1 ?
                    lvm::FrfEstimator::Direct : lvm::FrfEstimator::H1;
                compute_frf_from_current_source();
                return 0;
            }
            if (id==Smoothing && code==CBN_SELCHANGE) {
                const int choice=static_cast<int>(SendMessageW(control(Smoothing),CB_GETCURSEL,0,0));
                if (choice>=0 && choice<5)
                    g.frf.display_smoothing_octaves=smoothing_choices[choice];
                refresh_frf_controls(); set_status(); invalidate_plot();
                return 0;
            }
            if ((id==Input || id==Output) && code==BN_CLICKED) {
                show_frf_channel_picker(id==Input);
                return 0;
            }
            if (handle_frf_picker_command(id)) return 0;
            if (code != BN_CLICKED && code != BN_DOUBLECLICKED) return 0;
            if (id == Processing) {
                g.frf.apply_processing = !g.frf.apply_processing;
                invalidate_frf();
                compute_frf_from_current_source();
            } else if (id == Calculate) {
                if (read_segment_length()) compute_frf_from_current_source();
            } else if (id == ApplyRange) {
                wchar_t low[80]{}, high[80]{};
                GetWindowTextW(control(Low), low, 80); GetWindowTextW(control(High), high, 80);
                double a = 0, b = 0;
                if (!parse_wide_double_text(low, a) || !parse_wide_double_text(high, b) ||
                    !set_frf_frequency_range(a, b)) {
                    MessageBoxW(hwnd, tr(L"Use 0 < F min < F max within the calculated frequency range.",
                        L"Укажите 0 < F min < F max в пределах рассчитанных частот."), L"FRF", MB_OK | MB_ICONINFORMATION);
                }
            } else if (id == Csv) {
                save_as_dialog();
            } else if (id == Png) {
                save_png_dialog();
            }
            return 0;
        }
        case WM_DRAWITEM: {
            auto* dis = reinterpret_cast<DRAWITEMSTRUCT*>(lp);
            if (!dis || !dis->hwndItem) break;
            wchar_t text[128]{}; GetWindowTextW(dis->hwndItem, text, 128);
            if (dis->CtlID == Processing) {
                draw_themed_check_control(dis->hDC, dis->rcItem, text,
                    g.frf.apply_processing,
                    (dis->itemState & ODS_SELECTED) != 0, true, false, false);
            } else {
                draw_themed_button(dis->hDC, dis->rcItem, text,
                    (dis->itemState & ODS_SELECTED) != 0, false, false);
            }
            return TRUE;
        }
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN: {
            HDC dc = reinterpret_cast<HDC>(wp);
            SetTextColor(dc, g_theme->text_primary); SetBkMode(dc, TRANSPARENT);
            return reinterpret_cast<LRESULT>(g_panel_brush);
        }
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX: {
            HDC dc = reinterpret_cast<HDC>(wp);
            SetTextColor(dc, g_theme->text_primary); SetBkColor(dc, g_theme->bg_plot);
            return reinterpret_cast<LRESULT>(g_input_brush);
        }
        case WM_ERASEBKGND: {
            RECT r; GetClientRect(hwnd, &r);
            FillRect(reinterpret_cast<HDC>(wp), &r, g_panel_brush);
            return 1;
        }
        case WM_DESTROY: g.frf_panel = nullptr; return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
}

std::wstring frf_error_text(lvm::FrfError e) {
    if (e==lvm::FrfError::InvalidChannels) return tr(
        L"Select at least one Support and Response without shared channels.",
        L"Выберите хотя бы одну опору и отклик без общих каналов.");
    if (g_str == &kEn) return to_w(lvm::frf_error_text(e));
    switch (e) {
        case lvm::FrfError::None: return L"";
        case lvm::FrfError::InvalidChannels: break;
        case lvm::FrfError::FrequencyData: return L"Для FRF нужны исходные сигналы во времени.";
        case lvm::FrfError::TooShort: return L"В выбранном участке меньше L отсчётов. Уменьшите L или расширьте участок.";
        case lvm::FrfError::InvalidTime: return L"Временные метки должны быть конечными и строго возрастающими.";
        case lvm::FrfError::MissingValues: return L"В выбранных каналах есть пропущенные или бесконечные значения.";
        case lvm::FrfError::InvalidOptions: return L"Некорректные параметры FRF.";
        case lvm::FrfError::WeakReference: return L"Во входном канале нет достаточного переменного сигнала.";
        case lvm::FrfError::Overflow: return L"Превышены численные ограничения или недостаточно памяти для FRF.";
    }
    return L"Не удалось рассчитать FRF.";
}

namespace {
bool valid_selection(std::vector<int> references,std::vector<int> responses) {
    const int count=static_cast<int>(g.ds.channel_count());
    if (references.empty() || responses.empty()) return false;
    for (auto* list:{&references,&responses}) {
        std::sort(list->begin(),list->end());
        if (list->front()<0 || list->back()>=count || std::adjacent_find(list->begin(),list->end())!=list->end()) return false;
    }
    for (int channel:references) if (std::binary_search(responses.begin(),responses.end(),channel)) return false;
    return true;
}
}

namespace {
enum { PickerTitle=7300, PickerSupports=7301, PickerResponses, PickerSwap, PickerApply, PickerCancel,
       PickerHelp, PickerSupportsLabel, PickerResponsesLabel };

std::vector<int> picker_selection(HWND list) {
    const int count=static_cast<int>(SendMessageW(list,LB_GETSELCOUNT,0,0));
    if (count<=0) return {};
    std::vector<int> rows(static_cast<std::size_t>(count));
    SendMessageW(list,LB_GETSELITEMS,count,reinterpret_cast<LPARAM>(rows.data()));
    std::vector<int> result; result.reserve(rows.size());
    for (int row:rows) result.push_back(static_cast<int>(SendMessageW(list,LB_GETITEMDATA,row,0)));
    return result;
}
void picker_set_selection(HWND list,const std::vector<int>& channels) {
    SendMessageW(list,LB_SETSEL,FALSE,-1);
    for (int channel:channels) SendMessageW(list,LB_SETSEL,TRUE,channel);
}
void picker_populate(HWND list,const std::vector<int>& channels) {
    SendMessageW(list,LB_RESETCONTENT,0,0);
    for (std::size_t c=0;c<g.ds.channel_count();++c) {
        const std::wstring item=std::to_wstring(c+1)+L": "+channel_display_label(static_cast<int>(c));
        const int row=static_cast<int>(SendMessageW(list,LB_ADDSTRING,0,reinterpret_cast<LPARAM>(item.c_str())));
        SendMessageW(list,LB_SETITEMDATA,row,c);
    }
    picker_set_selection(list,channels);
}
}

void show_frf_channel_picker(bool supports) {
    if (!g.frf_panel) return;
    for (int id : {Processing,EstimatorLabel,Estimator,LengthLabel,Length,Method,SmoothingLabel,Smoothing,
                   Source,LowLabel,Low,HighLabel,High,ApplyRange,Calculate,Csv,Png,Hint})
        ShowWindow(control(id),SW_HIDE);
    label(PickerTitle,tr(L"Channel roles",L"Роли каналов"));
    label(PickerHelp,tr(L"Choose both roles. A channel can belong to one role only.",
        L"Выберите обе роли. Канал может относиться только к одной роли."));
    label(PickerSupportsLabel,tr(L"Supports / Reference",L"Опоры"));
    label(PickerResponsesLabel,tr(L"Responses",L"Отклики"));
    label(PickerSwap,tr(L"Swap",L"Поменять")); label(PickerApply,tr(L"Apply",L"Применить")); label(PickerCancel,tr(L"Cancel",L"Отмена"));
    picker_populate(control(PickerSupports),g.frf.inputs);
    picker_populate(control(PickerResponses),g.frf.outputs);
    for (int id : {PickerTitle,PickerHelp,PickerSupportsLabel,PickerResponsesLabel,PickerSupports,PickerResponses,PickerSwap,PickerApply,PickerCancel})
        ShowWindow(control(id),SW_SHOW);
    SetFocus(control(supports ? PickerSupports : PickerResponses));
}

bool handle_frf_picker_command(int id) {
    if (!g.frf_panel || !(GetWindowLongPtrW(control(PickerSupports),GWL_STYLE)&WS_VISIBLE)) return false;
    if (id==PickerSwap) {
        const auto left=picker_selection(control(PickerSupports)), right=picker_selection(control(PickerResponses));
        picker_set_selection(control(PickerSupports),right); picker_set_selection(control(PickerResponses),left);
        return true;
    }
    if (id!=PickerApply && id!=PickerCancel) return false;
    if (id==PickerApply) {
        const auto supports=picker_selection(control(PickerSupports));
        const auto responses=picker_selection(control(PickerResponses));
        if (!valid_selection(supports,responses)) {
            MessageBoxW(g.frf_panel,tr(L"Choose at least one support and one response; channel roles cannot overlap.",
                L"Выберите хотя бы одну опору и отклик; роли каналов не должны пересекаться."),L"FRF",MB_OK|MB_ICONINFORMATION);
            return true;
        }
        set_frf_channels(supports,responses);
    }
    for (int picker : {PickerTitle,PickerHelp,PickerSupportsLabel,PickerResponsesLabel,PickerSupports,PickerResponses,PickerSwap,PickerApply,PickerCancel})
        ShowWindow(control(picker),SW_HIDE);
    for (int regular : {Processing,EstimatorLabel,Estimator,LengthLabel,Length,Method,SmoothingLabel,Smoothing,
                        Source,LowLabel,Low,HighLabel,High,ApplyRange,Calculate,Csv,Png,Hint})
        ShowWindow(control(regular),SW_SHOW);
    refresh_frf_controls();
    return true;
}

bool set_frf_channels(std::vector<int> references,std::vector<int> responses) {
    if (!valid_selection(references,responses)) return false;
    std::sort(references.begin(),references.end()); std::sort(responses.begin(),responses.end());
    if (references==g.frf.inputs && responses==g.frf.outputs) return true;
    g.frf.inputs=std::move(references); g.frf.outputs=std::move(responses);
    invalidate_frf();
    if (g.mode==AnalysisMode::FRF) compute_frf_from_current_source();
    refresh_frf_controls();
    return true;
}
std::wstring frf_curve_label(std::size_t response) {
    if (response>=g.frf.output_names.size()) return L"";
    return g.frf.output_names[response]+L" / "+g.frf.input_name;
}

void invalidate_frf(bool reset_view) {
    g_frf_worker.cancel();
    ++g.frf.generation;
    g.frf.pending = false; g.frf.attempted = false;
    g.frf.result = {};
    if (reset_view) { g.frf.view_initialized = false; g.frf.auto_y = true; }
}

void compute_frf_from_current_source() {
    invalidate_frf();
    g.frf.attempted = true;
    double a = 0, b = 0; bool selected = false;
    lvm::FrfBatchResult failure;
    failure.options=g.frf.options;
    if (g.ds.frequency_axis) failure.error = lvm::FrfError::FrequencyData;
    else if (!valid_selection(g.frf.inputs,g.frf.outputs)) failure.error=lvm::FrfError::InvalidChannels;
    else if (!current_fft_source_window(a, b, selected)) failure.error = lvm::FrfError::TooShort;
    if (failure.error != lvm::FrfError::None) { apply_frf_result(std::move(failure)); return; }
    g.frf.source_start = a; g.frf.source_end = b; g.frf.from_selection = selected;
    g.frf.input_name.clear(); g.frf.output_names.clear();
    for (int c:g.frf.inputs) {
        if (!g.frf.input_name.empty()) g.frf.input_name+=L", ";
        g.frf.input_name+=channel_display_label(c);
    }
    if (g.frf.inputs.size()>1) g.frf.input_name=L"AVG("+g.frf.input_name+L")";
    for (int c:g.frf.outputs) g.frf.output_names.push_back(channel_display_label(c));
    try {
        lvm::Dataset pair;
        std::vector<std::size_t> channels(g.frf.inputs.begin(),g.frf.inputs.end());
        channels.insert(channels.end(),g.frf.outputs.begin(),g.frf.outputs.end());
        build_time_window_dataset(g.ds, a, b, pair, &channels, g.frf.apply_processing);
        lvm::FrfBatchInput input; input.time=std::move(pair.time);
        for (std::size_t i=0;i<pair.channels.size();++i) {
            auto& destination=i<g.frf.inputs.size() ? input.references : input.responses;
            destination.push_back(std::move(pair.channels[i]));
        }
        g.frf.processing_description = L"raw";
        if (g.frf.apply_processing) {
            g.frf.processing_description = L"global=" + g.global_formula +
                L"; filter=" + std::to_wstring(g.noise_threshold_enabled) +
                L"; mode=" + std::to_wstring(g.noise_threshold_mode) +
                L"; topology=" + std::to_wstring(g.noise_threshold_topology) +
                L"; low=" + format_optional_edit_number(g.noise_threshold_min) +
                L"; high=" + format_optional_edit_number(g.noise_threshold_max);
            for (auto c:channels) g.frf.processing_description+=L"; channel["+std::to_wstring(c)+L"]="+g.channel_formulas[c];
        }
        if (g.main) {
            g.frf.pending = true;
            g_frf_worker.submit(std::move(input), g.frf.options, g.frf.generation);
            refresh_frf_controls(); set_status(); invalidate_plot();
        } else {
            apply_frf_result(lvm::analyze_frf_batch(std::move(input), g.frf.options));
        }
    } catch (...) {
        failure.error = lvm::FrfError::Overflow;
        apply_frf_result(std::move(failure));
    }
}

bool ensure_current_frf() {
    double start = 0, end = 0; bool selected = false;
    if (g.frf.attempted && !g.frf.pending &&
        (g.frf.result.options.estimator!=g.frf.options.estimator ||
         g.frf.result.options.segment_length!=g.frf.options.segment_length))
        invalidate_frf();
    if (g.frf.attempted && current_fft_source_window(start, end, selected) &&
        (start != g.frf.source_start || end != g.frf.source_end || selected != g.frf.from_selection))
        invalidate_frf();
    if (!g.frf.attempted) compute_frf_from_current_source();
    return g.frf.result.ok && !g.frf.pending;
}

void apply_frf_result(lvm::FrfBatchResult result) {
    g.frf.result = std::move(result); g.frf.pending = false;
    if (g.frf.result.ok) {
        const auto& f = g.frf.result.common().frequencies;
        const double lo = std::log10(f[1]), hi = std::log10(f.back());
        if (!g.frf.view_initialized || g.frf.log_end <= lo || g.frf.log_start >= hi) {
            g.frf.log_start = lo; g.frf.log_end = hi;
        } else {
            g.frf.log_start = std::max(lo, g.frf.log_start);
            g.frf.log_end = std::min(hi, g.frf.log_end);
        }
        g.frf.view_initialized = true;
    }
    refresh_frf_controls();
    if (g.mode == AnalysisMode::FRF) { set_status(); if (g.main) invalidate_plot(); }
}

void poll_frf_result() {
    if (auto r = g_frf_worker.take_result())
        if (r->generation == g.frf.generation) apply_frf_result(std::move(r->frf));
}

void on_frf_processing_changed() {
    refresh_frf_controls(true);
    if (!g.frf.apply_processing) return;
    invalidate_frf();
    if (g.mode == AnalysisMode::FRF) compute_frf_from_current_source();
}

std::wstring frf_status_text() {
    if (g.frf.pending) return tr(L"Calculating FRF…", L"Вычисление АЧХ…");
    if (!g.frf.result.ok) return gui::frf_error_text(g.frf.result.error);
    wchar_t buf[256]{};
    const auto& r = g.frf.result.common();
    swprintf(buf, 256, L"FRF: %.6g–%.6g Hz | N=%zu | Fs=%.6g Hz | Hann | %ls",
        std::pow(10.0, g.frf.log_start), std::pow(10.0, g.frf.log_end),
        r.sample_count, 1.0 / r.sample_dt,
        g.frf.apply_processing ? tr(L"Processed", L"С обработкой") : tr(L"Raw", L"Исходные"));
    std::wstring text = buf;
    if (r.gaps_ignored) text = tr(L"Warning: Gaps ignored | ",
        L"Внимание: пропуски проигнорированы (Gaps ignored) | ") + text;
    wchar_t method[160]{};
    swprintf(method,160,L" | %ls L=%zu K=%zu Δf=%.6g Hz overlap=%.0f%%",
        r.options.estimator==lvm::FrfEstimator::H1 ? L"H1" : L"Direct",
        r.segment_length,r.averages,1.0/(r.sample_dt*r.segment_length),
        100.0*r.overlap_samples/r.segment_length);
    text+=method;
    for (std::size_t i=0;i<g.frf.result.responses.size();++i)
        if (!g.frf.result.responses[i].ok) text+=L" | "+g.frf.output_names[i]+L": "+gui::frf_error_text(g.frf.result.responses[i].error);
    if (r.averages<2) text+=tr(L" | Coherence unavailable: K < 2",L" | Coherence недоступна: K < 2");
    return text;
}

void create_frf_panel(HWND parent, HINSTANCE instance) {
    WNDCLASSW wc{};
    wc.hInstance = instance; wc.lpfnWndProc = FrfPanelProc;
    wc.lpszClassName = L"AMGraphFrfPanel"; wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassW(&wc);
    g.frf_panel = CreateWindowExW(0, wc.lpszClassName, L"FRF", WS_CHILD | WS_CLIPCHILDREN,
        0, 0, kRightPanel, 400, parent, nullptr, instance, nullptr);
}

void layout_frf_panel() {
    if (!g.frf_panel) return;
    const bool show = g.mode == AnalysisMode::FRF && g.side_panel_visible && !welcome_visible();
    ShowWindow(g.frf_panel, show ? SW_SHOW : SW_HIDE);
    if (show) {
        RECT r; GetClientRect(g.main, &r);
        MoveWindow(g.frf_panel, r.right - kRightPanel, kTopBar, kRightPanel,
            std::max(1L, r.bottom - kTopBar - kBottomBar), TRUE);
    }
}

void refresh_frf_controls(bool repopulate) {
    if (!g.frf_panel) return;
    label(InputLabel, tr(L"Supports / Reference", L"Опоры"));
    label(OutputLabel, tr(L"Responses", L"Отклики"));
    label(Processing, tr(L"Apply channel processing", L"Применять обработку каналов"));
    label(LowLabel, L"F min, Hz"); label(HighLabel, L"F max, Hz");
    label(ApplyRange, tr(L"Apply frequency range", L"Применить диапазон частот"));
    label(EstimatorLabel, tr(L"Estimator",L"Метод")); label(LengthLabel,L"L (0 = Auto)");
    label(SmoothingLabel, tr(L"Display smoothing",L"Сглаживание графика"));
    const auto& r=g.frf.result.common();
    wchar_t details[192]{};
    if (r.segment_length && !g.frf.pending) {
        swprintf(details,192,L"%ls · Hann · L=%zu · K=%zu\nΔf=%.6g Hz · overlap=%.0f%% (%zu)",
            r.options.estimator==lvm::FrfEstimator::H1 ? L"H1" : L"Direct",
            r.segment_length,r.averages,1.0/(r.sample_dt*r.segment_length),
            100.0*r.overlap_samples/r.segment_length,r.overlap_samples);
        label(Method,details);
    } else label(Method,g.frf.pending ? tr(L"Calculating…",L"Вычисление…") : L"Hann · L/K/Δf: —");
    EnableWindow(control(Length),g.frf.options.estimator==lvm::FrfEstimator::H1);
    label(Calculate, tr(L"Calculate", L"Рассчитать")); label(Csv, L"CSV"); label(Png, L"PNG");
    label(Hint, tr(L"KD = Response / average Support. CSV contains raw values.\nSelect the time interval in Signal mode.",
                   L"КД = отклик / средняя опора. CSV содержит исходные значения.\nУчасток выделяется в режиме времени."));
    if (repopulate) {
        SendMessageW(control(Estimator),CB_SETCURSEL,g.frf.options.estimator==lvm::FrfEstimator::H1 ? 0 : 1,0);
        SendMessageW(control(Smoothing),CB_SETCURSEL,smoothing_choice(g.frf.display_smoothing_octaves),0);
        label(Length,std::to_wstring(g.frf.options.segment_length).c_str());
    }
    const auto button_text=[&](const std::vector<int>& channels,bool reference) {
        const wchar_t* action=reference ? tr(L"Select supports: ",L"Выбрать опоры: ") :
            tr(L"Select responses: ",L"Выбрать отклики: ");
        if (channels.size()==1) return std::wstring(action)+channel_display_label(channels.front())+L" ▾";
        return std::wstring(action)+std::to_wstring(channels.size())+tr(L" channels ▾",L" каналов ▾");
    };
    label(Input,button_text(g.frf.inputs,true).c_str());
    label(Output,button_text(g.frf.outputs,false).c_str());
    if (g.frf.view_initialized) {
        label(Low, format_edit_number(std::pow(10.0, g.frf.log_start)).c_str());
        label(High, format_edit_number(std::pow(10.0, g.frf.log_end)).c_str());
    }
    wchar_t source[192]{};
    swprintf(source, 192, tr(L"%ls: %.6g–%.6g s", L"%ls: %.6g–%.6g с"),
        g.frf.from_selection ? tr(L"Selection", L"Выделение") : tr(L"Time view", L"Видимый участок"),
        g.frf.source_start, g.frf.source_end);
    label(Source, source);
    const bool ready = g.frf.result.ok && !g.frf.pending;
    EnableWindow(control(Csv), ready); EnableWindow(control(Png), ready);
    EnableWindow(control(ApplyRange), ready);
    InvalidateRect(control(Processing), nullptr, TRUE);
}

bool set_frf_frequency_range(double low, double high) {
    if (!g.frf.result.ok || !std::isfinite(low) || !std::isfinite(high) || low <= 0 || high <= low) return false;
    const auto& f = g.frf.result.common().frequencies;
    if (low < f[1] * (1 - 1e-9) || high > f.back() * (1 + 1e-9)) return false;
    g.frf.log_start = std::log10(std::max(low, f[1]));
    g.frf.log_end = std::log10(std::min(high, f.back()));
    refresh_frf_controls(); set_status(); if (g.main) invalidate_plot();
    return true;
}
void reset_frf_view() {
    if (!g.frf.result.ok) return;
    g.frf.auto_y = true;
    set_frf_frequency_range(g.frf.result.common().frequencies[1], g.frf.result.common().frequencies.back());
}
bool frf_command_supported(int id) {
    switch (id) {
        case IDC_PLAY: case IDM_ADD_MARKER: case IDM_CLEAR_MARKERS: case IDM_VISMOOTH:
            return false;
    }
    return true;
}
} // namespace gui
