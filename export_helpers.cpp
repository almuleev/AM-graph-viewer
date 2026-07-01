#include "export_helpers.hpp"

#include <algorithm>

const wchar_t* export_prompt_title_text(bool english) {
    return english ? L"Export data" : L"Экспорт данных";
}

const wchar_t* export_prompt_intro_text(bool english) {
    return english
        ? L"Choose what should be exported."
        : L"Выберите, что нужно выгрузить.";
}

const wchar_t* export_range_title_text(bool english) {
    return english ? L"Export range" : L"Область экспорта";
}

std::wstring export_scope_title_text(ExportDataScope scope, bool english) {
    switch (scope) {
        case ExportDataScope::CurrentView:
            return english ? L"Current view" : L"Текущий вид";
        case ExportDataScope::LoadedFragment:
            return english ? L"Loaded fragment" : L"Загруженный фрагмент";
        case ExportDataScope::RawData:
            return english ? L"Raw data without formulas" : L"Сырые данные без формул";
    }
    return L"";
}

std::wstring export_scope_detail_text(ExportDataScope scope, bool english) {
    switch (scope) {
        case ExportDataScope::CurrentView:
            return english
                ? L"Uses the selected range if present, otherwise the visible graph area."
                : L"Берёт выделенный участок, а если его нет - видимую область графика.";
        case ExportDataScope::LoadedFragment:
            return english
                ? L"The whole loaded fragment in the current mode."
                : L"Весь загруженный фрагмент в текущем режиме.";
        case ExportDataScope::RawData:
            return english
                ? L"Original time-domain data, all channels, no formulas."
                : L"Исходные временные данные, все каналы, без формул.";
    }
    return L"";
}

std::wstring export_scope_choice_text(ExportDataScope scope, bool english) {
    return export_scope_title_text(scope, english) + L"\r\n" +
           export_scope_detail_text(scope, english);
}

std::wstring export_range_title_text(ExportRangeMode range, bool english) {
    switch (range) {
        case ExportRangeMode::Selected:
            return english ? L"Selected range" : L"Выделенная область";
        case ExportRangeMode::Visible:
            return english ? L"Visible range" : L"Видимая область";
        case ExportRangeMode::Whole:
            return english ? L"Whole file" : L"Весь файл";
    }
    return L"";
}

std::wstring export_range_detail_text(ExportRangeMode range, bool english) {
    switch (range) {
        case ExportRangeMode::Selected:
            return english
                ? L"Uses the selected time window if one exists."
                : L"Использует выделенный фрагмент, если он есть.";
        case ExportRangeMode::Visible:
            return english
                ? L"Uses only the part currently visible on screen."
                : L"Берёт только ту часть, которая сейчас видна на экране.";
        case ExportRangeMode::Whole:
            return english
                ? L"Exports the entire loaded data set."
                : L"Выгружает весь загруженный набор данных.";
    }
    return L"";
}

std::wstring export_range_choice_text(ExportRangeMode range, bool english) {
    return export_range_title_text(range, english) + L"\r\n" +
           export_range_detail_text(range, english);
}

const wchar_t* export_prompt_continue_text(bool english) {
    return english ? L"Continue" : L"Продолжить";
}

const wchar_t* export_prompt_cancel_text(bool english) {
    return english ? L"Cancel" : L"Отмена";
}

const wchar_t* export_apply_processing_text(bool english) {
    return english
        ? L"Apply formulas and filter to exported data"
        : L"Применить формулы и фильтр к экспортируемым данным";
}

const wchar_t* export_processing_settings_text(bool english) {
    return english
        ? L"Save formulas and filter as settings only"
        : L"Сохранить формулы и фильтр только как настройки";
}

const wchar_t* export_apply_processing_note_text(bool english) {
    return english
        ? L"When off, data are exported raw and formulas/filter are saved as settings only."
        : L"Если выключено, данные выгружаются без изменений, а формулы/фильтр сохраняются только как настройки.";
}

const wchar_t* export_include_channel_names_text(bool english) {
    return english ? L"Channel names" : L"Названия каналов";
}

const wchar_t* export_include_points_text(bool english) {
    return english ? L"Points" : L"Точки";
}

const wchar_t* export_include_markers_text(bool english) {
    return english ? L"Markers" : L"Маркеры";
}

const wchar_t* export_include_guides_text(bool english) {
    return english ? L"Guide lines" : L"Направляющие";
}

const wchar_t* export_include_formulas_text(bool english) {
    return english ? L"Formulas" : L"Формулы";
}

const wchar_t* export_include_filter_text(bool english) {
    return english ? L"Filter settings" : L"Настройки фильтра";
}

const wchar_t* export_include_graph_settings_text(bool english) {
    return english ? L"Graph settings" : L"Настройки графика";
}

std::wstring export_default_name(const std::wstring& stem,
                                ExportDataScope scope,
                                bool csv,
                                bool freq_mode) {
    return export_default_name(stem, scope, csv ? L".csv" : L".txt", freq_mode);
}

std::wstring export_default_name(const std::wstring& stem,
                                ExportDataScope scope,
                                const wchar_t* ext,
                                bool freq_mode) {
    std::wstring out = stem;
    switch (scope) {
        case ExportDataScope::CurrentView:
            out += freq_mode ? L"_view_freq" : L"_view_time";
            break;
        case ExportDataScope::LoadedFragment:
            out += freq_mode ? L"_fragment_freq" : L"_fragment_time";
            break;
        case ExportDataScope::RawData:
            out += L"_raw_time";
            break;
    }
    out += ext ? ext : L"";
    return out;
}

std::wstring export_default_name(const std::wstring& stem,
                                 ExportRangeMode range,
                                 const wchar_t* ext,
                                 bool freq_mode) {
    std::wstring out = stem;
    switch (range) {
        case ExportRangeMode::Selected:
            out += freq_mode ? L"_selected_freq" : L"_selected_time";
            break;
        case ExportRangeMode::Visible:
            out += freq_mode ? L"_visible_freq" : L"_visible_time";
            break;
        case ExportRangeMode::Whole:
            out += freq_mode ? L"_whole_freq" : L"_whole_time";
            break;
    }
    out += ext ? ext : L"";
    return out;
}

std::pair<std::size_t, std::size_t> export_range_bounds(const std::vector<double>& values,
                                                        double start,
                                                        double end) {
    if (start > end) std::swap(start, end);
    const auto first = std::lower_bound(values.begin(), values.end(), start);
    const auto last = std::upper_bound(values.begin(), values.end(), end);
    return {
        static_cast<std::size_t>(first - values.begin()),
        static_cast<std::size_t>(last - values.begin())
    };
}

std::wstring export_status_prefix(const wchar_t* format_name,
                                  ExportDataScope scope,
                                  bool english) {
    const std::wstring scope_text = export_scope_title_text(scope, english);
    if (english) {
        return std::wstring(L"Exported ") + format_name + L" (" + scope_text + L"): ";
    }
    return std::wstring(L"Выгружен ") + format_name + L" (" + scope_text + L"): ";
}

std::wstring export_status_prefix(const wchar_t* format_name,
                                  ExportRangeMode range,
                                  bool english) {
    const std::wstring scope_text = export_range_title_text(range, english);
    if (english) {
        return std::wstring(L"Exported ") + format_name + L" (" + scope_text + L"): ";
    }
    return std::wstring(L"Выгружен ") + format_name + L" (" + scope_text + L"): ";
}

const wchar_t* export_error_text(bool english, bool csv) {
    if (csv) return english ? L"Failed to export CSV." : L"Не удалось выгрузить CSV.";
    return english ? L"Failed to export TXT." : L"Не удалось выгрузить TXT.";
}
