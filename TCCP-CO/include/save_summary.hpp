/*
    Copyright 2024, 2026 OFFIS e.V.
    This activity has received funding from the Key Digital Technologies Joint Undertaking (KDT JU)
    under grant agreement No 877056.
    The JU receives support from the European Union’s Horizon 2020 research
    and innovation programme and Spain, Italy, Austria, Germany, Finland, Switzerland.
    https://www.offis.de/en/offis/project/isolde.html

    OFFIS e.V. licenses this file
    to you under the Apache License, Version 2.0 (the
    "License"); you may not use this file except in compliance
    with the License.  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing,
    software distributed under the License is distributed on an
    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
    KIND, either express or implied.  See the License for the
    specific language governing permissions and limitations
    under the License.
    Contributors:
        Sven Mehlhop - initial implementation
*/
#pragma once

#include "types.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <cmath>


static std::string toHexStr(uint64_t v) {
    std::ostringstream ss;
    ss << "0x" << std::hex << v;
    return ss.str();
}

static std::string toDecStr(uint64_t v) {
    return std::to_string(v);
}

static std::string findLabel(uint64_t val, const std::vector<Label>& labels) {
    for (const auto& l : labels) {
        if (l.value == val) return l.name;
    }
    return "";
}

static std::string formatPeriod(double period_ns) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << period_ns;
    std::string s = ss.str();
    if (s.find('.') != std::string::npos) {
        s.erase(s.find_last_not_of('0') + 1);
        if (s.back() == '.') s.pop_back();
    }
    return s + "ns";
}

static std::string fmtFloat(double v, int precision = 2) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(precision) << v;
    std::string s = ss.str();
    if (s.find('.') != std::string::npos) {
        s.erase(s.find_last_not_of('0') + 1);
        if (s.back() == '.') s.pop_back();
    }
    return s;
}

static bool isEventType(int type) { return type == 0 || type == 1 || type == 2; }

static bool hasTimeInterval(int type) { return type <= 3; }
static bool hasTimepoints(int type)   { return type == 3; }

static std::string fmtEventReadable(uint64_t val, int type, bool is_val_b,
                                    const std::vector<Label>& labels) {
    if (!isEventType(type)) {
        return toDecStr(val);
    }
    if (type == 0 && is_val_b) {
        return "0 (noLabel)";
    }
    std::string lbl = findLabel(val, labels);
    std::ostringstream ss;
    ss << "0x" << std::hex << val;
    ss << " " << (lbl.empty() ? "(noLabel)" : lbl);
    return ss.str();
}

static std::string fmtTimepointReadable(double orig_ns, uint64_t cycles) {
    return fmtFloat(orig_ns) + "ns/" + std::to_string(cycles) + "cycles";
}

static std::string fmtIntervalReadable(uint64_t cycles_val, double orig_ns,
                                       int type) {
    if (hasTimeInterval(type) && orig_ns > 0.0) {
        return fmtFloat(orig_ns) + "ns/" + std::to_string(cycles_val) + "cycles";
    }
    return toDecStr(cycles_val);
}
static std::string fmtEventHex(uint64_t val, int type,
                                const std::vector<Label>& labels) {
    if (!isEventType(type)) {
        return toHexStr(val);
    }
    std::string lbl = findLabel(val, labels);
    return toHexStr(val) + " " + (lbl.empty() ? "(noLabel)" : lbl);
}

static std::string fmtTimepointHex(uint64_t cycles_val) {
    return toHexStr(cycles_val);
}

static std::string fmtIntervalHex(uint64_t val, int type,
                                  const std::string& label) {
    if (type == 4 && !label.empty()) {
        return label + " (" + toHexStr(val) + ")";
    }
    return toHexStr(val);
}

inline void saveSummary(const std::vector<Contract>& contracts,
                        const ProjectConfig& proj_cfg,
                        const std::string& out_dir,
                        const std::string& filename = "summary.md") {
    std::filesystem::create_directories(out_dir);
    std::string filepath = out_dir + "/" + filename;
    std::ofstream out(filepath);
    if (!out.is_open()) {
        std::cerr << "Error creating summary file: " << filepath << "\n";
        return;
    }

    const std::vector<Label>& labels = proj_cfg.labels;
    const double period_ns = proj_cfg.period_ns;

    out << "# General Information\n";
    out << "freq=" << proj_cfg.freq_str << "\n";
    out << "period=" << formatPeriod(period_ns) << "\n";
    out << "# legend\n";

    out << "## Observer\n";
    for (const auto& obs : proj_cfg.observers) {
        out << "ID(" << obs.name << ")=" << obs.id << "\n";
    }
    out << "\n";

    out << "## Label\n";
    for (const auto& l : labels) {
        out << "0x" << std::hex << l.value << std::dec << "=" << l.name << "\n";
    }
    out << "\n";

    out << "## Contract type\n";
    out << "0: Periodic\n";
    out << "1: Reaction\n";
    out << "2: Aging\n";
    out << "3: TSBC time\n";
    out << "4: TSBC PC\n";
    out << "\n";

    auto writeTableHeader = [&]() {
        out << "| ID  | Monitor | Observer | Event/ Val a      | Event/ Val b      | ValInt Start | ValInt Stop  |\n";
        out << "|:---:|:-------:|:--------:|:-----------------:|:-----------------:|:------------:|:------------:|\n";
    };

    out << "# Table Readable\n";
    writeTableHeader();
    for (const auto& c : contracts) {
        std::string val_a_cell, val_b_cell;

        if (hasTimepoints(c.type)) {
            val_a_cell = fmtTimepointReadable(c.val_a_ns, c.val_a);
            val_b_cell = fmtTimepointReadable(c.val_b_ns, c.val_b);
        } else {
            val_a_cell = fmtEventReadable(c.val_a, c.type, false, labels);
            val_b_cell = fmtEventReadable(c.val_b, c.type, true,  labels);
        }

        out << "| " << c.id
            << " | " << c.type
            << " | " << c.obs_id
            << " | " << val_a_cell
            << " | " << val_b_cell
            << " | " << fmtIntervalReadable(c.val_interval_start, c.val_interval_start_ns, c.type)
            << " | " << fmtIntervalReadable(c.val_interval_stop,  c.val_interval_stop_ns,  c.type)
            << " |\n";
    }
    out << "\n";

    out << "# Table HEX\n";
    writeTableHeader();
    for (const auto& c : contracts) {
        std::string val_a_cell, val_b_cell;

        if (hasTimepoints(c.type)) {
            val_a_cell = fmtTimepointHex(c.val_a);
            val_b_cell = fmtTimepointHex(c.val_b);
        } else {
            val_a_cell = fmtEventHex(c.val_a, c.type, labels);
            val_b_cell = fmtEventHex(c.val_b, c.type, labels);
        }

        out << "| " << c.id
            << " | " << c.type
            << " | " << c.obs_id
            << " | " << val_a_cell
            << " | " << val_b_cell
            << " | " << fmtIntervalHex(c.val_interval_start, c.type, c.val_interval_start_label)
            << " | " << fmtIntervalHex(c.val_interval_stop,  c.type, c.val_interval_stop_label)
            << " |\n";
    }

    std::cout << "Summary saved to: " << filepath << "\n";
}
