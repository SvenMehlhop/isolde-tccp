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
#include <string>
#include <iostream>
#include <algorithm>
#include <vector>
#include <fstream>
#include <regex>
#include "toml.hpp"

#include "types.hpp"

inline MainConfig loadMainConfig(const std::string& filename) {
    MainConfig mc;
    try {
        auto config = toml::parse_file(filename);
        mc.input = config["main"]["input"].value_or("Input");
        mc.output = config["main"]["output"].value_or("Output");
        
        //std::cout << "Main Input: " << mc.input << "\n";
        //std::cout << "Main Output: " << mc.output << "\n";
    } catch (const toml::parse_error& err) {
        std::cerr << "Error parsing config file '" << filename << "': " << err << std::endl;
    }
    return mc;
}

inline ProjectConfig loadProjectConfig(const std::string& filename) {
    ProjectConfig pc;
    try {
        auto config = toml::parse_file(filename);
        
        if (auto arr = config["main"]["contract"].as_array()) {
            for (auto& elem : *arr) {
                std::string c_file = elem.value_or(std::string(""));
                if (!c_file.empty()) {
                    pc.contracts.push_back(c_file);
                }
            }
        } else {
            std::string c_file = config["main"]["contract"].value_or(std::string(""));
            if (!c_file.empty()) {
                pc.contracts.push_back(c_file);
            }
        }

        pc.output = config["main"]["output"].value_or("");
        pc.freq_str = config["main"]["freq"].value_or("");

        if (pc.freq_str.ends_with("Hz")) {
            try {
                size_t pos = 0;
                double value = std::stod(pc.freq_str, &pos);
                pc.unit = pc.freq_str.substr(pos);

                double multiplier = 1.0;
                if (pc.unit == "kHz")      multiplier = 1e3;
                else if (pc.unit == "MHz") multiplier = 1e6;
                else if (pc.unit == "GHz") multiplier = 1e9;
                else if (pc.unit != "Hz") {
                    throw std::invalid_argument("Unknown unit: " + pc.unit);
                }

                double freq_hz = value * multiplier;
                
                pc.period_ns = 1e9 / freq_hz;

            } catch (...) {
                std::cerr << "Invalid frequency format: " << pc.freq_str << std::endl;
                exit(1);
            }
        } else {
            std::cerr << "Frequency must end with 'Hz': " << pc.freq_str << std::endl;
            exit(1);
        }


        if (auto observers = config["observer"].as_array()) {
            for (auto& elem : *observers) {
                if (auto tbl = elem.as_table()) {
                    Observer obs;
                    obs.id = (*tbl)["id"].value_or(0);
                    obs.name = (*tbl)["name"].value_or("");
                    
                    // Parse multiple sources tuples
                    if (auto sources = (*tbl)["sources"].as_array()) {
                        for (auto& s_elem : *sources) {
                            if (auto s_tbl = s_elem.as_table()) {
                                ObserverSource src;
                                src.contract = (*s_tbl)["contract"].value_or("");
                                src.dump = (*s_tbl)["dump"].value_or("");
                                if (!src.contract.empty() || !src.dump.empty()) {
                                    obs.sources.push_back(src);
                                }
                            }
                        }
                    }
                    
                    // Fallback to legacy/direct contract & dump fields
                    std::string single_contract = (*tbl)["contract"].value_or("");
                    std::string single_dump = (*tbl)["dump"].value_or("");
                    if (!single_contract.empty() || !single_dump.empty()) {
                        ObserverSource src;
                        src.contract = single_contract;
                        src.dump = single_dump;
                        obs.sources.push_back(src);
                    }
                    
                    pc.observers.push_back(obs);
                }
            }
        }

        for (const auto& obs : pc.observers) {
            for (const auto& src : obs.sources) {
                if (!src.dump.empty()) {
                    std::string dir = "";
                    size_t last_slash = filename.find_last_of("/\\");
                    if (last_slash != std::string::npos) {
                        dir = filename.substr(0, last_slash + 1);
                    }
                    std::string dump_path = dir + src.dump;
                    
                    std::ifstream dump_file(dump_path);
                    if (dump_file.is_open()) {
                        std::string dump_line;
                        std::regex rx_label(R"(^([0-9a-fA-F]+)\s+<([^>]+)>:)");
                        int loaded_count = 0;
                        while (std::getline(dump_file, dump_line)) {
                            std::smatch match;
                            if (std::regex_search(dump_line, match, rx_label)) {
                                std::string pc_hex = match[1].str();
                                std::string label_name = match[2].str();
                                if (label_name[0] != '.') {
                                    Label l;
                                    l.value = std::stoull(pc_hex, nullptr, 16);
                                    l.name = label_name;
                                    l.obs_id = obs.id;
                                    l.dump_path = dump_path;
                                    pc.labels.push_back(l);
                                    loaded_count++;
                                }
                            }
                        }
                        std::cout << "Loaded " << loaded_count << " labels from dump file: " << dump_path << " (observer ID " << obs.id << ")\n";
                    } else {
                        std::cerr << "Warning: Could not open dump file " << dump_path << " for observer " << obs.name << "\n";
                    }
                }
            }
        }
    } catch (const toml::parse_error& err) {
        std::cerr << "Error parsing project config file '" << filename << "': " << err << std::endl;
    }
    return pc;
}

