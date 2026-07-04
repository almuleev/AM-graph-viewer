void apply_export_metadata_from_comments(const std::vector<std::string>& comments) {
    if (comments.empty() || g.ds.channel_count() == 0) return;

    enum class ExportSection {
        None,
        Export,
        Graph,
        Filter,
        Channels,
        Formulas,
        PointGroups,
        Markers,
        Guides,
    };

    auto trim_copy = [](const std::string& text) {
        const char* ws = " \t\r\n\f\v";
        const std::size_t begin = text.find_first_not_of(ws);
        if (begin == std::string::npos) return std::string();
        const std::size_t end = text.find_last_not_of(ws);
        return text.substr(begin, end - begin + 1);
    };
    auto lower_copy = [](std::string text) {
        for (char& ch : text) {
            if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
        }
        return text;
    };
    auto parse_bool = [&](const std::string& text, bool& out) {
        const std::string value = lower_copy(trim_copy(text));
        if (value == "1" || value == "true" || value == "yes" || value == "on") {
            out = true;
            return true;
        }
        if (value == "0" || value == "false" || value == "no" || value == "off") {
            out = false;
            return true;
        }
        int numeric = 0;
        char tail = '\0';
        if (std::sscanf(value.c_str(), "%d%c", &numeric, &tail) >= 1) {
            out = (numeric != 0);
            return true;
        }
        return false;
    };
    auto parse_int = [&](const std::string& text, int& out) {
        const std::string value = trim_copy(text);
        if (value.empty()) return false;
        char* end = nullptr;
        const long parsed = std::strtol(value.c_str(), &end, 10);
        if (end == value.c_str() || *end != '\0') return false;
        out = static_cast<int>(parsed);
        return true;
    };
    auto parse_double = [&](const std::string& text, double& out) {
        const std::string value = trim_copy(text);
        if (value.empty()) return false;
        char* end = nullptr;
        const double parsed = std::strtod(value.c_str(), &end);
        if (end == value.c_str() || *end != '\0') return false;
        out = parsed;
        return true;
    };
    auto parse_color_triplet = [&](const std::string& text, COLORREF& out) {
        unsigned int r = 0, g = 0, b = 0;
        if (std::sscanf(text.c_str(), "%u,%u,%u", &r, &g, &b) != 3) return false;
        out = RGB(static_cast<BYTE>(std::clamp(r, 0u, 255u)),
                  static_cast<BYTE>(std::clamp(g, 0u, 255u)),
                  static_cast<BYTE>(std::clamp(b, 0u, 255u)));
        return true;
    };
    auto decode_export_text = [&](const std::string& text) {
        if (text.empty()) return std::wstring();
        std::wstring utf8 = to_w(text);
        if (!utf8.empty()) return utf8;
        return to_w_acp(text);
    };
    auto extract_after = [&](const std::string& text, const std::string& prefix) {
        const std::size_t pos = text.find(prefix);
        if (pos == std::string::npos) return std::string();
        return trim_copy(text.substr(pos + prefix.size()));
    };
    auto extract_between = [&](const std::string& text, const std::string& prefix, const std::string& suffix) {
        const std::size_t pos = text.find(prefix);
        if (pos == std::string::npos) return std::string();
        const std::size_t begin = pos + prefix.size();
        if (suffix.empty()) return trim_copy(text.substr(begin));
        const std::size_t end = text.find(suffix, begin);
        if (end == std::string::npos) return trim_copy(text.substr(begin));
        return trim_copy(text.substr(begin, end - begin));
    };
    auto parse_point_display = [&](const std::string& text, PointDisplay& display) {
        bool any = false;
        std::size_t start = 0;
        while (start <= text.size()) {
            const std::size_t comma = text.find(',', start);
            const std::string token = trim_copy(text.substr(start, comma == std::string::npos ? std::string::npos : comma - start));
            if (!token.empty()) {
                const std::size_t eq = token.find('=');
                if (eq != std::string::npos) {
                    const std::string key = lower_copy(trim_copy(token.substr(0, eq)));
                    bool value = false;
                    if (parse_bool(token.substr(eq + 1), value)) {
                        any = true;
                        if (key == "number") display.number = value;
                        else if (key == "x") display.x = value;
                        else if (key == "y") display.y = value;
                        else if (key == "dx") display.dx = value;
                        else if (key == "dy") display.dy = value;
                        else if (key == "inv_dt") display.inv_dt = value;
                        else if (key == "dist") display.dist = value;
                    }
                }
            }
            if (comma == std::string::npos) break;
            start = comma + 1;
        }
        return any;
    };
    auto parse_filter_mode = [&](const std::string& text, int& out) {
        const std::string value = lower_copy(trim_copy(text));
        if (value == "low_pass" || value == "low-pass") { out = FilterModeLowPass; return true; }
        if (value == "high_pass" || value == "high-pass") { out = FilterModeHighPass; return true; }
        if (value == "band_pass" || value == "band-pass" || value == "bandpass") { out = FilterModeBandPass; return true; }
        if (value == "band_stop" || value == "band-stop" || value == "bandstop") { out = FilterModeBandStop; return true; }
        return false;
    };
    auto parse_filter_topology = [&](const std::string& text, int& out) {
        const std::string value = lower_copy(trim_copy(text));
        if (value == "butterworth") { out = FilterTopologyButterworth; return true; }
        if (value == "bessel") { out = FilterTopologyBessel; return true; }
        if (value == "chebyshev") { out = FilterTopologyChebyshev; return true; }
        if (value == "linkwitz_riley" || value == "linkwitz-riley") { out = FilterTopologyLinkwitzRiley; return true; }
        return false;
    };
    auto parse_section_name = [&](const std::string& text) {
        const std::string section = lower_copy(trim_copy(text));
        if (section == "[export]") return ExportSection::Export;
        if (section == "[graph_settings]" || section == "[graph]") return ExportSection::Graph;
        if (section == "[filter_settings]" || section == "[filter]") return ExportSection::Filter;
        if (section == "[channels]") return ExportSection::Channels;
        if (section == "[formulas]") return ExportSection::Formulas;
        if (section == "[point_groups]") return ExportSection::PointGroups;
        if (section == "[markers]") return ExportSection::Markers;
        if (section == "[guides]") return ExportSection::Guides;
        return ExportSection::None;
    };
    auto parse_indexed_line = [&](const std::string& text, const char* prefix, int& out_index, std::string& tail) {
        const std::string line = trim_copy(text);
        if (line.rfind(prefix, 0) != 0) return false;
        const std::size_t lb = line.find('[');
        const std::size_t rb = line.find(']', lb == std::string::npos ? 0 : lb + 1);
        if (lb == std::string::npos || rb == std::string::npos || rb <= lb + 1) return false;
        int one_based = 0;
        if (!parse_int(line.substr(lb + 1, rb - lb - 1), one_based) || one_based <= 0) return false;
        out_index = one_based - 1;
        tail = trim_copy(line.substr(rb + 1));
        return true;
    };

    ExportSection section = ExportSection::None;
    bool imported_formulas = false;
    std::vector<PointGroup> point_groups;
    std::vector<App::Marker> markers;
    std::vector<GuideLine> guides;
    int current_group = -1;

    for (const std::string& raw_comment : comments) {
        const std::string line = trim_copy(raw_comment);
        if (line.empty()) continue;
        if (line.front() == '[' && line.back() == ']') {
            section = parse_section_name(line);
            if (section != ExportSection::PointGroups) {
                current_group = -1;
            }
            continue;
        }
        if (section == ExportSection::None) continue;

        if (section == ExportSection::Export) {
            const std::size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            const std::string key = lower_copy(trim_copy(line.substr(0, eq)));
            const std::string value = trim_copy(line.substr(eq + 1));
            if (key == "partial_fragment") {
                bool partial = false;
                if (parse_bool(value, partial) && partial) {
                    g.current_file_partial = true;
                }
            }
            continue;
        }

        if (section == ExportSection::Graph) {
            const std::size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            const std::string key = lower_copy(trim_copy(line.substr(0, eq)));
            const std::string value = trim_copy(line.substr(eq + 1));
            if (key == "axis_x_label") {
                g.axis_x_label = normalize_axis_label_text(decode_export_text(value), L"X");
            } else if (key == "axis_y_label") {
                g.axis_y_label = normalize_axis_label_text(decode_export_text(value), L"Y");
            } else if (key == "marker_color") {
                parse_color_triplet(value, g.marker_color);
            } else if (key == "smoothing") {
                parse_bool(value, g.visual_smooth);
            } else if (key == "vertical_pan") {
                parse_bool(value, g.vertical_pan);
            } else if (key == "snap_to_data") {
                parse_bool(value, g.snap_to_data);
            } else if (key == "show_gap_markers") {
                parse_bool(value, g.show_gap_markers);
            } else if (key == "light_mode") {
                parse_bool(value, g.light_mode);
            } else if (key == "auto_y") {
                parse_bool(value, g.auto_y);
            } else if (key == "y_lock_min") {
                parse_double(value, g.y_lock_min);
            } else if (key == "y_lock_max") {
                parse_double(value, g.y_lock_max);
            } else if (key == "auto_y_amp") {
                parse_bool(value, g.auto_y_amp);
            } else if (key == "y_amp_max") {
                parse_double(value, g.y_amp_max);
            } else if (key == "play_speed") {
                double speed = g.play_speed;
                if (parse_double(value, speed) && std::isfinite(speed) && speed > 0.0) g.play_speed = speed;
            } else if (key == "point_display") {
                parse_point_display(value, g.pdisp);
            }
            continue;
        }

        if (section == ExportSection::Filter) {
            const std::size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            const std::string key = lower_copy(trim_copy(line.substr(0, eq)));
            const std::string value = trim_copy(line.substr(eq + 1));
            if (key == "enabled") {
                parse_bool(value, g.noise_threshold_enabled);
            } else if (key == "mode") {
                parse_filter_mode(value, g.noise_threshold_mode);
            } else if (key == "topology") {
                parse_filter_topology(value, g.noise_threshold_topology);
            } else if (key == "low_cutoff") {
                parse_double(value, g.noise_threshold_min);
            } else if (key == "high_cutoff") {
                parse_double(value, g.noise_threshold_max);
            }
            continue;
        }

        if (section == ExportSection::Channels) {
            int channel_index = -1;
            std::string tail;
            if (!parse_indexed_line(line, "channel", channel_index, tail)) continue;
            if (channel_index < 0 || static_cast<std::size_t>(channel_index) >= g.ds.channel_count()) continue;
            const std::string name = extract_between(tail, "name=", ", visible=");
            const std::string visible_text = extract_between(tail, "visible=", ", color=");
            const std::string color_text = extract_after(tail, "color=");
            if (!name.empty()) {
                const std::wstring wname = decode_export_text(name);
                g.channel_labels[static_cast<std::size_t>(channel_index)] = wname;
                g.ds.names[static_cast<std::size_t>(channel_index)] = to_utf8(wname);
            }
            bool visible = true;
            if (parse_bool(visible_text, visible)) {
                g.visible[static_cast<std::size_t>(channel_index)] = visible ? 1 : 0;
            }
            COLORREF color = channel_color(static_cast<std::size_t>(channel_index));
            if (parse_color_triplet(color_text, color)) {
                if (static_cast<std::size_t>(channel_index) >= g_channel_colors.size()) {
                    g_channel_colors.resize(g.ds.channel_count());
                }
                g_channel_colors[static_cast<std::size_t>(channel_index)] = color;
            }
            continue;
        }

        if (section == ExportSection::Formulas) {
            const std::size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            const std::string key = lower_copy(trim_copy(line.substr(0, eq)));
            const std::string value = trim_copy(line.substr(eq + 1));
            if (key == "global") {
                g.global_formula = decode_export_text(value);
                imported_formulas = true;
            } else {
                int channel_index = -1;
                std::string tail;
                if (parse_indexed_line(line, "channel", channel_index, tail) && channel_index >= 0 &&
                    static_cast<std::size_t>(channel_index) < g.channel_formulas.size()) {
                    g.channel_formulas[static_cast<std::size_t>(channel_index)] = decode_export_text(value);
                    imported_formulas = true;
                }
            }
            continue;
        }

        if (section == ExportSection::PointGroups) {
            int group_index = -1;
            std::string tail;
            if (parse_indexed_line(line, "group", group_index, tail)) {
                PointGroup group;
                const std::string name = extract_between(tail, "name=", ", visible=");
                const std::string visible_text = extract_between(tail, "visible=", ", color=");
                const std::string color_text = extract_between(tail, "color=", ", display=");
                const std::string display_text = extract_after(tail, "display=");
                group.name = decode_export_text(name);
                bool visible = true;
                if (parse_bool(visible_text, visible)) group.visible = visible;
                parse_color_triplet(color_text, group.color);
                parse_point_display(display_text, group.display);
                point_groups.push_back(std::move(group));
                current_group = static_cast<int>(point_groups.size()) - 1;
                continue;
            }
            if (current_group >= 0) {
                int point_index = -1;
                if (parse_indexed_line(line, "point", point_index, tail)) {
                    double x = 0.0;
                    double y = 0.0;
                    if (parse_double(extract_between(tail, "x=", ", y="), x) &&
                        parse_double(extract_after(tail, "y="), y)) {
                        point_groups[static_cast<std::size_t>(current_group)].points.emplace_back(x, y);
                    }
                }
            }
            continue;
        }

        if (section == ExportSection::Markers) {
            int marker_index = -1;
            std::string tail;
            if (!parse_indexed_line(line, "marker", marker_index, tail)) continue;
            App::Marker marker;
            marker.label = decode_export_text(extract_between(tail, "label=", ", x="));
            parse_double(extract_between(tail, "x=", ", y="), marker.x);
            parse_double(extract_between(tail, "y=", ", mode="), marker.y);
            const std::string mode = lower_copy(extract_between(tail, "mode=", ", snapped="));
            marker.freq = (mode == "frequency");
            bool snapped = false;
            if (parse_bool(extract_between(tail, "snapped=", ", channel="), snapped)) marker.snapped = snapped;
            parse_int(extract_after(tail, "channel="), marker.channel);
            markers.push_back(std::move(marker));
            continue;
        }

        if (section == ExportSection::Guides) {
            int guide_index = -1;
            std::string tail;
            if (!parse_indexed_line(line, "guide", guide_index, tail)) continue;
            GuideLine guide;
            const std::string kind = lower_copy(extract_between(tail, "kind=", ", value="));
            guide.vertical = !(kind == "horizontal");
            parse_double(extract_between(tail, "value=", ", mode="), guide.value);
            const std::string mode = lower_copy(extract_after(tail, "mode="));
            guide.freq = (mode == "frequency");
            guides.push_back(guide);
            continue;
        }
    }

    normalize_filter_bounds();

    if (!point_groups.empty()) {
        g.point_groups = std::move(point_groups);
        g.active_point_group = static_cast<int>(g.point_groups.size()) - 1;
        sync_point_display_from_active_group();
    }
    if (!markers.empty()) {
        g.markers = std::move(markers);
        g.active_marker = -1;
    }
    if (!guides.empty()) {
        g.guides = std::move(guides);
    }
    if (imported_formulas) {
        g.formula_ini_deferred = false;
        rebuild_formula_cache_from_state();
    }
    if (!g.show_gap_markers) hide_gap_details_card();
    recompute_transforms_from_state();
    sync_channel_controls_from_state();
}

std::wstring lvm_current_date_text(const SYSTEMTIME& st);
std::wstring lvm_current_time_text(const SYSTEMTIME& st);
double lvm_export_nominal_delta_x();

bool save_tabular_export(const std::wstring& path, const ExportOptions& opts) {
    std::ofstream out(to_acp(path.c_str()), std::ios::binary);
    if (!out) return false;

    const bool csv = opts.format == ExportFileFormat::Csv;
    const char sep = csv ? ',' : '\t';
    const char* line_end = csv ? "\n" : "\r\n";
    double export_start = 0.0;
    double export_end = 0.0;
    bool actual_selected_range = false;

    if (g.freq_mode) {
        lvm::Spectrum export_spec;
        std::vector<int> spec_channel_indices;
        bool export_all_channels = false;
        if (!build_export_spectrum(opts, export_spec, spec_channel_indices, export_all_channels)) return false;

        if (opts.selected_range == ExportRangeMode::Whole) {
            export_start = export_spec.freqs.empty() ? 0.0 : export_spec.freqs.front();
            export_end = export_spec.freqs.empty() ? 0.0 : export_spec.freqs.back();
        } else {
            export_start = g.freq_start;
            export_end = g.freq_end;
            actual_selected_range = (opts.selected_range == ExportRangeMode::Selected);
        }
        if (!std::isfinite(export_start) || !std::isfinite(export_end)) return false;
        if (export_end < export_start) std::swap(export_start, export_end);
        if (export_end <= export_start) return false;

        std::vector<std::size_t> cols;
        write_export_metadata(out, opts, csv, export_start, export_end, actual_selected_range);
        out << to_acp(g_str->csv_freq);
        for (std::size_t j = 0; j < export_spec.amp.size(); ++j) {
            const int ci = (j < spec_channel_indices.size()) ? spec_channel_indices[j] : -1;
            const bool visible_channel =
                ci >= 0 && static_cast<std::size_t>(ci) < g.visible.size() &&
                g.visible[static_cast<std::size_t>(ci)];
            if (export_all_channels || visible_channel) {
                const std::size_t label_index = (ci >= 0) ? static_cast<std::size_t>(ci) : j;
                out << sep << to_acp(export_channel_label_text(label_index, opts.include_channel_names).c_str());
                cols.push_back(j);
            }
        }
        if (cols.empty()) {
            for (std::size_t j = 0; j < export_spec.amp.size(); ++j) cols.push_back(j);
        }
        out << line_end;
        std::size_t begin = 0;
        std::size_t end = export_spec.freqs.size();
        if (opts.selected_range != ExportRangeMode::Whole) {
            double start = export_start;
            double finish = export_end;
            if (start > finish) std::swap(start, finish);
            const auto bounds = export_range_bounds(export_spec.freqs, start, finish);
            begin = bounds.first;
            end = bounds.second;
        }
        for (std::size_t k = begin; k < end; ++k) {
            out << numfmt(export_spec.freqs[k]);
            for (std::size_t j : cols) {
                if (j < export_spec.amp.size() && k < export_spec.amp[j].size()) {
                    out << sep << numfmt(export_spec.amp[j][k]);
                } else {
                    out << sep;
                }
            }
            out << line_end;
        }
        return true;
    }

    if (opts.selected_range != ExportRangeMode::Whole) {
        // Selected range falls back to the visible view when no explicit window exists.
        if (!export_range_bounds_for_mode(opts.selected_range, export_start, export_end, actual_selected_range)) return false;
    } else {
        export_start = g.ds.time.empty() ? 0.0 : g.ds.time.front();
        export_end = g.ds.time.empty() ? 0.0 : g.ds.time.back();
    }
    write_export_metadata(out, opts, csv, export_start, export_end, actual_selected_range);
    out << to_acp(g_str->csv_time);
    const std::vector<std::size_t> cols = export_channel_indices(opts.include_hidden_channels);
    for (std::size_t c : cols) {
        out << sep << to_acp(export_channel_label_text(c, opts.include_channel_names).c_str());
    }
    out << line_end;

    std::size_t begin = 0;
    std::size_t end = g.ds.rows();
    if (opts.selected_range != ExportRangeMode::Whole && export_end > export_start) {
        const auto bounds = export_range_bounds(g.ds.time, export_start, export_end);
        begin = bounds.first;
        end = bounds.second;
    }
    for (std::size_t r = begin; r < end; ++r) {
        out << numfmt(g.ds.time[r]);
        for (std::size_t c : cols) {
            out << sep << numfmt(export_channel_sample(c, r, opts.apply_processing_to_data));
        }
        out << line_end;
    }
    return true;
}

bool save_lvm_export(const std::wstring& path, const ExportOptions& opts) {
    if (!has_data() || g.freq_mode) return false;

    double export_start = 0.0;
    double export_end = 0.0;
    bool actual_selected_range = false;
    if (!export_range_bounds_for_mode(opts.selected_range, export_start, export_end, actual_selected_range)) return false;

    const auto bounds = export_range_bounds(g.ds.time, export_start, export_end);
    std::size_t begin = bounds.first;
    std::size_t end = bounds.second;
    if (begin >= end) return false;

    const std::vector<std::size_t> cols = export_channel_indices(opts.include_hidden_channels);

    std::ofstream out(to_acp(path.c_str()), std::ios::binary);
    if (!out) return false;

    const char* line_end = "\r\n";
    SYSTEMTIME stamp{};
    GetLocalTime(&stamp);
    const std::wstring date_text = lvm_current_date_text(stamp);
    const std::wstring time_text = lvm_current_time_text(stamp);
    const std::wstring y_unit = g.axis_y_label.empty() ? L"Value" : g.axis_y_label;
    const double delta_x = lvm_export_nominal_delta_x();
    const std::wstring delta_x_text = format_edit_number(delta_x);
    auto repeat_header = [&](const char* key, const std::wstring& value) {
        out << key;
        const std::string encoded = to_acp(value.c_str());
        for (std::size_t i = 0; i < cols.size(); ++i) {
            out << '\t' << encoded;
        }
        out << line_end;
    };
    auto clean_label = [](std::string text) {
        for (char& ch : text) {
            if (ch == '\t' || ch == '\r' || ch == '\n') ch = ' ';
        }
        return text;
    };

    write_export_metadata(out, opts, false, export_start, export_end, actual_selected_range);
    out << "LabVIEW Measurement" << line_end;
    out << "Writer_Version\t0.92" << line_end;
    out << "Reader_Version\t1" << line_end;
    out << "Separator\tTab" << line_end;
    out << "Multi_Headings\tYes" << line_end;
    out << "X_Columns\tMulti" << line_end;
    out << "Time_Pref\tAbsolute" << line_end;
    out << "Operator\t" << to_acp(L"AM Graph Viewer") << line_end;
    out << "Date\t" << to_acp(date_text.c_str()) << line_end;
    out << "Time\t" << to_acp(time_text.c_str()) << line_end;
    out << "***End_of_Header***" << line_end << line_end;

    out << "Channels\t" << cols.size() << line_end;
    repeat_header("Samples", std::to_wstring(end - begin));
    repeat_header("Date", date_text);
    repeat_header("Time", time_text);
    repeat_header("Y_Unit_Label", y_unit);
    repeat_header("X_Dimension", L"Time");
    repeat_header("X0", L"0");
    repeat_header("Delta_X", delta_x_text);
    out << "***End_of_Header***" << line_end;

    std::string labels = "X_Value";
    for (std::size_t i = 0; i < cols.size(); ++i) {
        labels.push_back('\t');
        labels += clean_label(to_acp(export_channel_label_text(cols[i], opts.include_channel_names).c_str()));
        if (i + 1 < cols.size()) {
            labels.push_back('\t');
            labels += "X_Value";
        }
    }
    labels += "\tComment";
    out << labels << line_end;

    for (std::size_t r = begin; r < end; ++r) {
        std::string row = numfmt(g.ds.time[r]);
        for (std::size_t c : cols) {
            row.push_back('\t');
            row += numfmt(g.ds.time[r]);
            row.push_back('\t');
            row += numfmt(export_channel_sample(c, r, opts.apply_processing_to_data));
        }
        row.push_back('\t');
        out << row << line_end;
    }
    return true;
}

bool save_dialog(std::wstring& out_path, const wchar_t* filter, const wchar_t* defext,
                 const std::wstring& defname) {
    wchar_t file[2048] = L"";
    lstrcpynW(file, defname.c_str(), 2048);
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g.main;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = file;
    ofn.nMaxFile = 2048;
    ofn.lpstrDefExt = defext;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    if (!GetSaveFileNameW(&ofn)) return false;
    out_path = file;
    return true;
}

std::wstring file_stem() {
    std::wstring stem = g.file_name;
    std::size_t dot = stem.find_last_of(L'.');
    if (dot != std::wstring::npos) stem = stem.substr(0, dot);
    return stem;
}

void status_msg(const std::wstring& m) { if (g.status) SetWindowTextW(g.status, m.c_str()); }

void save_png_dialog() {
    if (!has_data()) { show_styled_info_prompt(g.main, g_str->msg_nodata, g_str->msg_openfirst, false); return; }
    std::wstring def = file_stem() + (g.freq_mode ? L"_spectrum.png" : L"_plot.png");
    std::wstring path;
    if (!save_dialog(path, g_str->filter_png, L"png", def)) return;
    if (save_png(path)) {
        const wchar_t* b = wcsrchr(path.c_str(), L'\\');
        status_msg(std::wstring(g_str->msg_saved_png) + (b ? b + 1 : path.c_str()));
    } else {
        MessageBoxW(g.main, g_str->msg_savepng_err, g_str->msg_error_title, MB_ICONERROR);
    }
}

const wchar_t* export_file_extension(ExportFileFormat format) {
    switch (format) {
        case ExportFileFormat::Txt: return L"txt";
        case ExportFileFormat::Csv: return L"csv";
        case ExportFileFormat::Lvm: return L"lvm";
    }
    return L"txt";
}

const wchar_t* export_file_filter(ExportFileFormat format) {
    const bool en = (g_str == &kEn);
    switch (format) {
        case ExportFileFormat::Txt:
            return en ? L"TXT file\0*.txt\0All files\0*.*\0"
                      : L"TXT С„Р°Р№Р»\0*.txt\0Р’СЃРµ С„Р°Р№Р»С‹\0*.*\0";
        case ExportFileFormat::Csv:
            return en ? L"CSV file\0*.csv\0All files\0*.*\0"
                      : L"CSV С„Р°Р№Р»\0*.csv\0Р’СЃРµ С„Р°Р№Р»С‹\0*.*\0";
        case ExportFileFormat::Lvm:
            return en ? L"LVM file\0*.lvm\0All files\0*.*\0"
                      : L"LVM С„Р°Р№Р»\0*.lvm\0Р’СЃРµ С„Р°Р№Р»С‹\0*.*\0";
    }
    return en ? L"TXT file\0*.txt\0All files\0*.*\0"
              : L"TXT С„Р°Р№Р»\0*.txt\0Р’СЃРµ С„Р°Р№Р»С‹\0*.*\0";
}

const wchar_t* export_file_name(ExportFileFormat format) {
    switch (format) {
        case ExportFileFormat::Txt: return L"TXT";
        case ExportFileFormat::Csv: return L"CSV";
        case ExportFileFormat::Lvm: return L"LVM";
    }
    return L"TXT";
}

bool save_export_file(const std::wstring& path, const ExportOptions& opts) {
    switch (opts.format) {
        case ExportFileFormat::Txt:
        case ExportFileFormat::Csv:
            return save_tabular_export(path, opts);
        case ExportFileFormat::Lvm:
            return save_lvm_export(path, opts);
    }
    return false;
}

void save_as_dialog() {
    if (!has_data()) { show_styled_info_prompt(g.main, g_str->msg_nodata, g_str->msg_openfirst, false); return; }
    ExportOptions opts;
    if (!prompt_export_options(opts)) return;
    if (opts.format == ExportFileFormat::Lvm && g.freq_mode) {
        MessageBoxW(g.main,
                    g_str == &kEn ? L"LVM export is available only in Time mode."
                                   : L"Р­РєСЃРїРѕСЂС‚ LVM РґРѕСЃС‚СѓРїРµРЅ С‚РѕР»СЊРєРѕ РІ СЂРµР¶РёРјРµ Р’СЂРµРјСЏ.",
                    g_str->msg_error_title, MB_ICONINFORMATION);
        return;
    }

    const wchar_t* ext = export_file_extension(opts.format);
    std::wstring def = export_default_name(file_stem(), opts.selected_range, L".", g.freq_mode);
    def += ext;
    std::wstring path;
    if (!save_dialog(path, export_file_filter(opts.format), ext, def)) return;
    if (save_export_file(path, opts)) {
        const wchar_t* b = wcsrchr(path.c_str(), L'\\');
        status_msg(export_status_prefix(export_file_name(opts.format), opts.selected_range, g_str == &kEn) + (b ? b + 1 : path.c_str()));
    } else {
        MessageBoxW(g.main,
                    g_str == &kEn ? L"Failed to export file." : L"РќРµ СѓРґР°Р»РѕСЃСЊ РІС‹РіСЂСѓР·РёС‚СЊ С„Р°Р№Р».",
                    g_str->msg_error_title, MB_ICONERROR);
    }
}

std::wstring lvm_current_date_text(const SYSTEMTIME& st) {
    wchar_t buf[32]{};
    swprintf(buf, 32, L"%04u/%02u/%02u",
             static_cast<unsigned int>(st.wYear),
             static_cast<unsigned int>(st.wMonth),
             static_cast<unsigned int>(st.wDay));
    return buf;
}

std::wstring lvm_current_time_text(const SYSTEMTIME& st) {
    wchar_t buf[32]{};
    swprintf(buf, 32, L"%02u:%02u:%02u,%03u",
             static_cast<unsigned int>(st.wHour),
             static_cast<unsigned int>(st.wMinute),
             static_cast<unsigned int>(st.wSecond),
             static_cast<unsigned int>(st.wMilliseconds));
    return buf;
}

double lvm_export_nominal_delta_x() {
    if (g.cached_global_gap_step_ready && g.cached_global_gap_step > 0.0) return g.cached_global_gap_step;
    if (g.approx_dt > 0.0) return g.approx_dt;
    if (g.ds.time.size() > 1) {
        const double diff = g.ds.time[1] - g.ds.time[0];
        if (std::isfinite(diff) && diff > 0.0) return diff;
    }
    return 1e-6;
}

