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
*/#pragma once

#include "types.hpp"
#include "debug.hpp"
#include <string>   
#include <iostream>
#include <fstream>
#include <vector>
#include <regex>

#include <cmath>

inline uint64_t parseHexOrDec(const std::string& str) {
    if (str.size() > 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        return std::stoull(str, nullptr, 16);
    }
    return std::stoull(str, nullptr, 10);
}

inline double parseTimeNs(const std::string& val_str, const std::string& unit) {
    uint64_t val = parseHexOrDec(val_str);
    double multiplier = 1.0;
    if      (unit == "s")  multiplier = 1e9;
    else if (unit == "ms") multiplier = 1e6;
    else if (unit == "us") multiplier = 1e3;
    else if (unit == "ns") multiplier = 1.0;
    else if (unit == "ps") multiplier = 1e-3;
    return static_cast<double>(val) * multiplier;
}

inline uint64_t parseTimeOrCycles(const std::string& val_str, const std::string& unit, double period_ns) {
    uint64_t val = parseHexOrDec(val_str);
    if (!unit.empty() && period_ns > 0) {
        double time_ns = parseTimeNs(val_str, unit);
        return static_cast<uint64_t>(std::round(time_ns / period_ns));
    }
    return val;
}

inline int getObserverId(const std::vector<Observer>& observers, const std::string& name) {
    for (const auto& obs : observers) {
        if (obs.name == name) return obs.id;
    }
    return -1;
}

inline void loadContractFile(const std::string& filename, const ProjectConfig& proj_cfg, std::vector<Contract>& contracts, int& id_counter, const std::string& specific_dump_path = "") {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening contract file '" << filename << "'\n";
        return;
    }
    std::cout << "Loading contract file: " << filename << "\n";
    std::string line;

    bool is_asm = (filename.size() >= 2 && filename.compare(filename.size() - 2, 2, ".s") == 0) ||
                  (filename.size() >= 2 && filename.compare(filename.size() - 2, 2, ".S") == 0) ||
                  (filename.size() >= 4 && filename.compare(filename.size() - 4, 4, ".asm") == 0);

    std::regex rx_contract(R"(CONTRACT\(\s*["']([^"']+)["']\s*\))");

    std::regex rx_periodic(R"(Event (0x[0-9a-fA-F]+|[0-9]+) occurs every (0x[0-9a-fA-F]+|[0-9]+)(s|ms|us|ns|ps)?,\s*(0x[0-9a-fA-F]+|[0-9]+)(s|ms|us|ns|ps)? at observer (\w+))");
    std::regex rx_reaction(R"(Whenever (0x[0-9a-fA-F]+|[0-9]+) occurs then (0x[0-9a-fA-F]+|[0-9]+) occurs within (0x[0-9a-fA-F]+|[0-9]+)(s|ms|us|ns|ps)?,\s*(0x[0-9a-fA-F]+|[0-9]+)(s|ms|us|ns|ps)? at observer (\w+))");
    std::regex rx_aging(R"(Whenever (0x[0-9a-fA-F]+|[0-9]+) occurs then (0x[0-9a-fA-F]+|[0-9]+) has occurred (0x[0-9a-fA-F]+|[0-9]+)(s|ms|us|ns|ps)?,\s*(0x[0-9a-fA-F]+|[0-9]+)(s|ms|us|ns|ps)? before at observer (\w+))");
    std::regex rx_tsbc_time(R"(Value is within (0x[0-9a-fA-F]+|[0-9]+),\s*(0x[0-9a-fA-F]+|[0-9]+) at timepoint (0x[0-9a-fA-F]+|[0-9]+)(s|ms|us|ns|ps)?,\s*(0x[0-9a-fA-F]+|[0-9]+)(s|ms|us|ns|ps)? at observer (\w+))");
    std::regex rx_tsbc_pc(R"(Value is within (0x[0-9a-fA-F]+|[0-9]+),\s*(0x[0-9a-fA-F]+|[0-9]+) from PC (0x[0-9a-fA-F]+|[0-9]+) until (0x[0-9a-fA-F]+|[0-9]+) at observer (\w+))");

    while (std::getline(file, line)) {
        std::string trimmed = line;
        trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
        if (trimmed.empty()) continue;

        if (is_asm) {
            std::smatch contract_match;
            if (std::regex_search(trimmed, contract_match, rx_contract)) {
                trimmed = contract_match[1].str();
            } else {
                continue;
            }
        } else {
            if (trimmed.rfind("//", 0) == 0) continue;
        }

        std::regex rx_obs(R"(at observer\s+(\w+))");
        std::smatch obs_match;
        int current_obs_id = -1;
        if (std::regex_search(trimmed, obs_match, rx_obs)) {
            std::string obs_name = obs_match[1].str();
            current_obs_id = getObserverId(proj_cfg.observers, obs_name);
        }

        // Resolve label names to their PCs using proj_cfg.labels specifically for this observer/dump
        for (const auto& label : proj_cfg.labels) {
            bool matches_obs = (current_obs_id == -1 || label.obs_id == current_obs_id);
            bool matches_dump = (specific_dump_path.empty() || label.dump_path == specific_dump_path);
            if (matches_obs && matches_dump) {
                std::regex rx_lbl("\\b" + label.name + "\\b");
                std::ostringstream ss;
                ss << "0x" << std::hex << label.value;
                trimmed = std::regex_replace(trimmed, rx_lbl, ss.str());
            }
        }

        std::smatch match;
        Contract c;
        c.full_text = trimmed;
        c.id = id_counter;

        if (std::regex_search(trimmed, match, rx_periodic)) {
            c.type = 0;
            c.val_a = parseHexOrDec(match[1].str());
            c.val_b = 0;
            c.val_interval_start    = parseTimeOrCycles(match[2].str(), match[3].str(), proj_cfg.period_ns);
            c.val_interval_stop     = parseTimeOrCycles(match[4].str(), match[5].str(), proj_cfg.period_ns);
            c.val_interval_start_ns = parseTimeNs(match[2].str(), match[3].str());
            c.val_interval_stop_ns  = parseTimeNs(match[4].str(), match[5].str());
            c.obs_id = getObserverId(proj_cfg.observers, match[6].str());
        } else if (std::regex_search(trimmed, match, rx_reaction)) {
            c.type = 1;
            c.val_a = parseHexOrDec(match[1].str());
            c.val_b = parseHexOrDec(match[2].str());
            c.val_interval_start    = parseTimeOrCycles(match[3].str(), match[4].str(), proj_cfg.period_ns);
            c.val_interval_stop     = parseTimeOrCycles(match[5].str(), match[6].str(), proj_cfg.period_ns);
            c.val_interval_start_ns = parseTimeNs(match[3].str(), match[4].str());
            c.val_interval_stop_ns  = parseTimeNs(match[5].str(), match[6].str());
            c.obs_id = getObserverId(proj_cfg.observers, match[7].str());
        } else if (std::regex_search(trimmed, match, rx_aging)) {
            c.type = 2;
            c.val_a = parseHexOrDec(match[1].str());
            c.val_b = parseHexOrDec(match[2].str());
            c.val_interval_start    = parseTimeOrCycles(match[3].str(), match[4].str(), proj_cfg.period_ns);
            c.val_interval_stop     = parseTimeOrCycles(match[5].str(), match[6].str(), proj_cfg.period_ns);
            c.val_interval_start_ns = parseTimeNs(match[3].str(), match[4].str());
            c.val_interval_stop_ns  = parseTimeNs(match[5].str(), match[6].str());
            c.obs_id = getObserverId(proj_cfg.observers, match[7].str());
        } else if (std::regex_search(trimmed, match, rx_tsbc_time)) {
            c.type = 3;
            c.val_interval_start = parseHexOrDec(match[1].str());
            c.val_interval_stop  = parseHexOrDec(match[2].str());
            c.val_a    = parseTimeOrCycles(match[3].str(), match[4].str(), proj_cfg.period_ns);
            c.val_b    = parseTimeOrCycles(match[5].str(), match[6].str(), proj_cfg.period_ns);
            c.val_a_ns = parseTimeNs(match[3].str(), match[4].str());
            c.val_b_ns = parseTimeNs(match[5].str(), match[6].str());
            c.obs_id = getObserverId(proj_cfg.observers, match[7].str());
        } else if (std::regex_search(trimmed, match, rx_tsbc_pc)) {
            c.type = 4;
            c.val_a = parseHexOrDec(match[1].str());
            c.val_b = parseHexOrDec(match[2].str());
            c.val_interval_start = parseHexOrDec(match[3].str());
            c.val_interval_stop  = parseHexOrDec(match[4].str());
            c.obs_id = getObserverId(proj_cfg.observers, match[5].str());
        } else {
            std::cerr << "Warning: Could not parse contract: " << trimmed << "\n";
            continue;
        }

        contracts.push_back(c);
        id_counter++;
    }

    for (const auto& c : contracts) {
        DEBUG_COUT << "  Parsed: ID=" << c.id 
                   << ", Type=" << c.type 
                   << ", ObsID=" << c.obs_id;
                  
        if (c.type == 0 || c.type == 1 || c.type == 2 || c.type == 4) {
            DEBUG_COUT << ", val_a=0x" << std::hex << c.val_a << std::dec 
                       << ", val_b=0x" << std::hex << c.val_b << std::dec;
        } else {
            DEBUG_COUT << ", val_a=" << c.val_a 
                       << ", val_b=" << c.val_b;
        }
        
        DEBUG_COUT << ", val_interval_start=" << c.val_interval_start 
                   << ", val_interval_stop=" << c.val_interval_stop << "\n";
    }
}
