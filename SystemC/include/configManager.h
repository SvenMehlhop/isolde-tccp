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

#include "contract_struct.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <systemc.h>
#include <vector>

struct ClockConfig
{
  double period;
  double duty_cycle;
  double start_time;
  bool initial_state;

  ClockConfig(double p = 1.0, double d = 0.5, double s = 0.0, bool v = true)
      : period(p), duty_cycle(d), start_time(s), initial_state(v) {}
};

class ConfigManager
{
public:
  //Defaults
  bool iti_active = true;
  bool tip_active = true;
  bool cam_active = true;
  bool ros_active = false;
  bool platon_hw_active = false;
  std::string iti_dasm_file = "../../tip-test/trace_hart_0.dasm";
  std::string iti_trace_file = "../../tip-test/iti.trace";
  std::string tip_trace_file = "../../Dump/tipDump/tip_port_0_signals_dump.txt";
  std::string ros_trace_file = "../../Dump/evalLibrary/time_model.json";
  std::string contract_file = "contracts.csv";
  std::vector<Contract> contracts;

  ClockConfig clock_tccp{1.0, 0.5, 120.0, true};
  ClockConfig clock_dpram{1.0, 0.5, 10.0, true};
  ClockConfig clock_iti{2.0, 0.5, 950.0, true};
  ClockConfig clock_tip{2.0, 0.5, 950.0, true};
  ClockConfig clock_cam{20.0, 0.5, 950.0, true};
  ClockConfig clock_ros{1000000, 0.5, 950.0, false};
  ClockConfig clock_platon_hw{1000000, 0.5, 950.0, false};

  ConfigManager(const std::string &filename = "config.txt")
  {
    std::ifstream file(filename);
    if (!file.is_open())
    {
      SC_REPORT_WARNING(
          "ConfigManager",
          ("Could not open " + filename + ". Using defaults.").c_str());
      return;
    }

    std::string line;
    while (std::getline(file, line))
    {
      size_t commentPos = line.find('#');
      if (commentPos != std::string::npos)
      {
        line = line.substr(0, commentPos);
      }

      line.erase(0, line.find_first_not_of(" \t\r\n"));
      line.erase(line.find_last_not_of(" \t\r\n") + 1);

      if (line.empty())
        continue;

      if (line.find("ITI_ACTIVE=") == 0)
      {
        iti_active = (line.substr(11) == "true");
      }
      else if (line.find("TIP_ACTIVE=") == 0)
      {
        tip_active = (line.substr(11) == "true");
      }
      else if (line.find("CAM_ACTIVE=") == 0)
      {
        cam_active = (line.substr(11) == "true");
      }
      else if (line.find("ROS_ACTIVE=") == 0)
      {
        ros_active = (line.substr(11) == "true");
      }
      else if (line.find("PLATON_HW_ACTIVE=") == 0)
      {
        platon_hw_active = (line.substr(17) == "true");
      }
      else if (line.find("ITI_DASM_FILE=") == 0)
      {
        iti_dasm_file = line.substr(14);
      }
      else if (line.find("ITI_TRACE_FILE=") == 0)
      {
        iti_trace_file = line.substr(15);
      }
      else if (line.find("TIP_TRACE_FILE=") == 0)
      {
        tip_trace_file = line.substr(15);
      }
      else if (line.find("ROS_TRACE_FILE=") == 0)
      {
        ros_trace_file = line.substr(15);
      }
      else if (line.find("CONTRACT_FILE=") == 0)
      {
        contract_file = line.substr(14);
      }
      else if (line.find("CLOCK_TCCP=") == 0)
      {
        parseClock(line.substr(11), clock_tccp);
      }
      else if (line.find("CLOCK_DPRAM=") == 0)
      {
        parseClock(line.substr(12), clock_dpram);
      }
      else if (line.find("CLOCK_ITI=") == 0)
      {
        parseClock(line.substr(10), clock_iti);
      }
      else if (line.find("CLOCK_TIP=") == 0)
      {
        parseClock(line.substr(10), clock_tip);
      }
      else if (line.find("CLOCK_CAM=") == 0)
      {
        parseClock(line.substr(10), clock_cam);
      }
      else if (line.find("CLOCK_ROS=") == 0)
      {
        parseClock(line.substr(10), clock_ros);
      }
      else if (line.find("CLOCK_PLATON_HW=") == 0)
      {
        parseClock(line.substr(16), clock_platon_hw);
      }
      else if (line.find("CONTRACT:") == 0)
      {
        parseContract(line.substr(9));
      }
    }
    file.close();

    if (!contract_file.empty())
    {
      loadContractsFromCSV(contract_file);
    }
    SC_REPORT_INFO("ConfigManager",
                   ("Configuration loaded from " + filename).c_str());
  }

private:
  void parseContract(const std::string &data)
  {
    std::stringstream ss(data);
    std::string segment;
    std::vector<std::string> parts;

    while (std::getline(ss, segment, ','))
    {
      segment.erase(0, segment.find_first_not_of(" \t"));
      segment.erase(segment.find_last_not_of(" \t") + 1);
      parts.push_back(segment);
    }

    if (parts.size() >= 7)
    {
      try
      {
        uint64_t id = std::stoull(parts[0], nullptr, 0);
        uint64_t type = std::stoull(parts[1], nullptr, 0);
        uint64_t obs_id = std::stoull(parts[2], nullptr, 0);
        uint64_t val_a = std::stoull(parts[3], nullptr, 0);
        uint64_t val_b = std::stoull(parts[4], nullptr, 0);
        uint64_t val_interval_start = std::stoull(parts[5], nullptr, 0);
        uint64_t val_interval_stop = std::stoull(parts[6], nullptr, 0);

        contracts.push_back(Contract(id, type, obs_id, val_a, val_b,
                                     val_interval_start, val_interval_stop,
                                     true));
      }
      catch (const std::exception &e)
      {
        SC_REPORT_ERROR("ConfigManager",
                        ("Error parsing contract: " + data).c_str());
      }
    }
    else
    {
      SC_REPORT_ERROR(
          "ConfigManager",
          ("Invalid contract line (expected 8 parts): " + data).c_str());
    }
  }

  void loadContractsFromCSV(const std::string &path)
  {
    std::ifstream file(path);
    if (!file.is_open())
    {
      SC_REPORT_WARNING("ConfigManager",
                        ("Could not open contract file " + path).c_str());
      return;
    }

    std::string line;
    while (std::getline(file, line))
    {
      size_t commentPos = line.find('#');
      if (commentPos != std::string::npos)
        line = line.substr(0, commentPos);
      line.erase(0, line.find_first_not_of(" \t\r\n"));
      line.erase(line.find_last_not_of(" \t\r\n") + 1);
      if (line.empty())
        continue;

      parseContract(line);
    }
    file.close();
  }

  void parseClock(const std::string &data, ClockConfig &config)
  {
    std::stringstream ss(data);
    std::string seg;
    std::vector<std::string> parts;
    while (std::getline(ss, seg, ','))
    {
      parts.push_back(seg);
    }
    if (parts.size() >= 4)
    {
      config.period = std::stod(parts[0]);
      config.duty_cycle = std::stod(parts[1]);
      config.start_time = std::stod(parts[2]);
      config.initial_state = (parts[3] == "true");
    }
  }
};
