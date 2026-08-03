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
#include <vector>
#include <cstdint>

struct MainConfig {
    std::string input;
    std::string output;
};

struct ObserverSource {
    std::string contract;
    std::string dump;
};

struct Observer {
    int id;
    std::string name;
    std::vector<ObserverSource> sources;
};

struct Label {
    uint64_t value;
    std::string name;
    int obs_id;
    std::string dump_path;
};

struct ProjectConfig {
    std::vector<std::string> contracts;
    std::string output;
    std::string freq_str;
    std::string unit;
    double period_ns;
    std::vector<Observer> observers;
    std::vector<Label> labels;
};

struct Contract {
    std::string full_text;
    int id;
    int type;
    int obs_id;
    uint64_t val_a;
    uint64_t val_b;
    uint64_t val_interval_start;  
    uint64_t val_interval_stop;   

    double val_a_ns            = 0.0;  
    double val_b_ns            = 0.0;  
    double val_interval_start_ns = 0.0;
    double val_interval_stop_ns  = 0.0;

    std::string val_interval_start_label;
    std::string val_interval_stop_label;
};
