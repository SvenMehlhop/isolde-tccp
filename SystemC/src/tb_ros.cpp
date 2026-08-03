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
#include <systemc.h>
#include <fstream>
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>
#include "json.hpp"

#include <map>
#include <deque>

using json = nlohmann::json;

// Map ROS topic types to fixed numerical IDs (addresses) for traceable contracts
static const std::map<std::string, uint64_t> ROS_TYPE_TO_ID = {
    {"map",          100},
    {"icp",          101},
    {"inliers",      102},
    {"doppler_comp", 103},
    {"tau",          104},
    {"publish",      105},
    {"vel_pick",     106},
    {"cluster",      107},
    {"total",        108},
    {"core_total",   108},
    {"vel_refine",   109},
    {"segment",      110},
    {"decode",       111}
};

SC_MODULE(Tb_ros)
{
    sc_in<bool>           clk;
    sc_out<bool>          new_val;
    sc_out<bool>          finished_tb;
    sc_out<sc_uint<64>>   iaddr;

    struct RosEvent
    {
        uint64_t rel_tick;    
        uint64_t type_id;   
        std::string type_name;
        double stamp;
    };

    bool     init      = false;
    bool     finished  = false;
    bool     active    = false;
    uint64_t tick_count = 0;
    size_t   ev_idx    = 0;

    std::string          ros_json_path;
    std::vector<RosEvent> events;
    std::deque<RosEvent>  event_queue;

    std::vector<RosEvent> parseRosJson(const std::string &filename)
    {
        std::vector<RosEvent> result;
        std::ifstream file(filename);
        if (!file.is_open())
        {
            SC_REPORT_ERROR("Tb_ros", ("Cannot open JSON: " + filename).c_str());
            return result;
        }

        json j;
        try
        {
            file >> j;
        }
        catch (const json::parse_error &e)
        {
            SC_REPORT_ERROR("Tb_ros", ("JSON parse error: " + std::string(e.what())).c_str());
            return result;
        }

        double t0 = -1.0;

        // First pass: find global t0 (minimum stamp_sec across all types)
        for (const auto &[type_name, id] : ROS_TYPE_TO_ID)
        {
            if (!j.contains(type_name) || !j[type_name].contains("raw_samples"))
                continue;

            for (const auto &sample : j[type_name]["raw_samples"])
            {
                if (sample.contains("stamp_sec"))
                {
                    double stamp = sample["stamp_sec"].get<double>();
                    if (t0 < 0.0 || stamp < t0)
                        t0 = stamp;
                }
            }
        }

        // Second pass: build event list
        for (const auto &[type_name, id] : ROS_TYPE_TO_ID)
        {
            if (!j.contains(type_name) || !j[type_name].contains("raw_samples"))
                continue;

            for (const auto &sample : j[type_name]["raw_samples"])
            {
                if (!sample.contains("stamp_sec"))
                    continue;

                double stamp = sample["stamp_sec"].get<double>();
                uint64_t rel_tick = static_cast<uint64_t>((stamp - t0) * 10000000.0 + 0.5);

                result.push_back({rel_tick, id, type_name, stamp});
            }
        }

        // Sort by timestamp; events at the same tick are ordered by type/stamp chronologically
        std::sort(result.begin(), result.end(),
                  [](const RosEvent &a, const RosEvent &b) {
                      if (a.rel_tick != b.rel_tick) {
                          return a.rel_tick < b.rel_tick;
                      }
                      if (a.stamp != b.stamp) {
                          return a.stamp < b.stamp;
                      }
                      return a.type_id < b.type_id;
                  });

        std::string msg = "ROS JSON loaded: " + std::to_string(result.size()) +
                          " events from " + filename;
        SC_REPORT_INFO("Tb_ros", msg.c_str());
        return result;
    }

    // -----------------------------------------------------------------------
    // Clock-driven run method (1 tick = 10 us)
    // -----------------------------------------------------------------------
    void run()
    {
        if (!active && !init)
        {
            SC_REPORT_INFO("Tb_ros", "TB ROS not active.");
            finished_tb.write(true);
            finished = true;
            init = true;
            return;
        }

        if (!init && active)
        {
            events = parseRosJson(ros_json_path);
            init   = true;
            if (events.empty())
            {
                SC_REPORT_ERROR("Tb_ros", "No ROS events loaded. Check JSON and observed types.");
                finished_tb.write(true);
                finished = true;
            }
            return;
        }

        if (init && active && !finished)
        {
            new_val.write(false);

            if (tick_count % 10000000 == 0)
            {
                std::cout << "[PROGRESS] Tb_ros: Tick " << tick_count << " of " << 300000000 << " (" << (tick_count * 100.0 / 300000000) << "%) at " << sc_time_stamp() << std::endl;
            }

            while (ev_idx < events.size() && events[ev_idx].rel_tick <= tick_count)
            {
                event_queue.push_back(events[ev_idx]);
                ev_idx++;
            }

            if (!event_queue.empty())
            {
                RosEvent ev = event_queue.front();
                event_queue.pop_front();

                iaddr.write(ev.type_id);
                new_val.write(true);

                if (event_queue.size() > 2)
                {
                    auto warning_msg = "Multiple (" + std::to_string(event_queue.size() + 1) + ") events per tick detected. Distributing to next ticks. " + std::to_string(event_queue.size()) + " events remaining.";
                    SC_REPORT_WARNING("Tb_ros", warning_msg.c_str());
                }
            }

            if ((ev_idx >= events.size() && event_queue.empty()) || tick_count >= 300000000)
            {
                if (!finished)
                {
                    std::ostringstream msg;
                    msg << "ROS simulation complete (limit reached) at " << sc_time_stamp();
                    SC_REPORT_INFO("Tb_ros", msg.str().c_str());
                    finished_tb.write(true);
                    finished = true;
                }
                return;
            }

            tick_count++;
        }
    }

    SC_HAS_PROCESS(Tb_ros);
    Tb_ros(sc_module_name nm, bool is_active, std::string json_path)
        : sc_module(nm), active(is_active), ros_json_path(json_path)
    {
        SC_METHOD(run);
        sensitive << clk.pos();
    }
};
