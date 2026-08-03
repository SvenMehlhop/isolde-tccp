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
#include "debug.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <filesystem>

inline void saveContracts(const std::vector<Contract>& contracts, const std::string& out_dir, const std::string& filename = "contracts.csv") {
    std::filesystem::create_directories(out_dir);
    std::string filepath = out_dir + "/" + filename;
    std::ofstream out(filepath);
    if (!out.is_open()) {
        std::cerr << "Error creating output file: " << filepath << "\n";
        return;
    }
    
    out << "# id, type, obs_id, val_a, val_b, val_interval_start, val_interval_stop\n";
    for (const auto& c : contracts) {
        out << std::dec
            << c.id << ", " << c.type << ", " << c.obs_id << ", "
            << "0x" << std::hex << c.val_a << ", "
            << "0x" << c.val_b << ", "
            << "0x" << c.val_interval_start << ", "
            << "0x" << c.val_interval_stop << "\n";
    }
    std::cout << "Contracts saved to: " << filepath << "\n";
}
