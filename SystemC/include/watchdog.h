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
#include "contract_struct.h"

extern bool g_debug_mode;

SC_MODULE(Watchdog)
{
  sc_in<bool> trigger;
  sc_in<sc_uint<64>> location;
  sc_in<sc_uint<64>> timestamp;
  sc_in<sc_uint<64>> data_value;
  sc_in<sc_uint<8>> observer_id;

  sc_in<bool> selected;
  sc_in<Contract> in_contract;

  bool active = false;
  bool initialized = false;
  Contract contract;
  sc_uint<64> diff;
  sc_uint<64> last_timestamp = 0;
  unsigned int watchdog_id;

  sc_out<bool> violation;

  SC_HAS_PROCESS(Watchdog);

  Watchdog(sc_module_name name, unsigned int id)
      : sc_module(name), active(false), watchdog_id(id)
  {
    SC_METHOD(eval);
    sensitive << trigger.pos();
    dont_initialize();

    SC_METHOD(init);
    sensitive << selected.pos();
    dont_initialize();
  }
  void init()
  {
    contract = in_contract.read();
    initialized = contract.exists;
    active = false;
  }

  void eval()
  {
    violation.write(false);
    if (!initialized)
    {
      // SC_REPORT_INFO("Watchdog", "Watchdog triggered before initialization");
      return;
    }
    else if (!active)
    {
      if (observer_id.read() != contract.observer_id)
      {
        return;
      }
      switch (contract.type)
      {
      case 0:
        if (contract.event_val_a == location.read())
        {

          active = true;
          last_timestamp = timestamp.read();
        }
        break;

      case 1:
        if (contract.event_val_a == location.read())
        {

          active = true;
          last_timestamp = timestamp.read();
        }

        break;
      case 2:
        if (contract.event_val_b == location.read())
        {

          active = true;
          last_timestamp = timestamp.read();
        }
        break;
      case 3:
        if (timestamp.read() > contract.event_val_a &&
            timestamp.read() < contract.event_val_b)
        {

          active = true;
          if (data_value.read() < contract.val_interval_start ||
              data_value.read() > contract.val_interval_stop)
          {
            violation.write(true);
          }
        }
        break;
      case 4:
        //std::cout << location.read() << "    ";
        if (location.read() == contract.event_val_a)
        {
          active = true;
          if (data_value.read() < contract.val_interval_start ||
              data_value.read() > contract.val_interval_stop)
          {
            violation.write(true);
          }
        }
        break;
      default:
        std::ostringstream msg;
        msg << "Unknown monitor type: " << contract.type;
        SC_REPORT_WARNING("Watchdog", msg.str().c_str());
        break;
      }
    }
    else if (active)
    {
      if (timestamp.read() == 0)
      {
        return;
      }
      diff = timestamp.read() - last_timestamp;
      switch (contract.type)
      {
      case 0:
        if (location.read() == contract.event_val_a &&
                 observer_id.read() == contract.observer_id)
        {
          if (diff < contract.val_interval_start)
          {
            violation.write(true);
            if (g_debug_mode) {
              std::cout << "[WD " << contract.id.to_int() << "] Violation (diff<start): diff=" << diff.to_uint64() << ", start=" << contract.val_interval_start.to_uint64() << ", timestamp=" << timestamp.read().to_uint64() << ", last=" << last_timestamp.to_uint64() << " at " << sc_time_stamp() << std::endl;
            }
          }
          last_timestamp = timestamp.read();
        } if(diff>contract.val_interval_stop){
          violation.write(true);
          if (g_debug_mode) {
            std::cout << "[WD " << contract.id.to_int() << "] Violation (diff>stop): diff=" << diff.to_uint64() << ", stop=" << contract.val_interval_stop.to_uint64() << ", timestamp=" << timestamp.read().to_uint64() << ", last=" << last_timestamp.to_uint64() << " at " << sc_time_stamp() << std::endl;
          }
          active=false;
        }
        
        break;
      case 1:
        if (diff > contract.val_interval_stop)
        {
          violation.write(true);
          active = false;
          if (g_debug_mode) {
            std::cout << "[WD " << watchdog_id << "] VIOLATION (diff > stop): diff=" << diff << " stop=" << contract.val_interval_stop << std::endl;
          }
        }
        if (location.read() == contract.event_val_b &&
            observer_id.read() == contract.observer_id)
        {
          if (diff < contract.val_interval_start)
          {
            violation.write(true);
            if (g_debug_mode) {
              std::cout << "[WD " << watchdog_id << "] VIOLATION (diff < start): diff=" << diff << " start=" << contract.val_interval_start << std::endl;
            }
          }

          active = false;
          if (g_debug_mode) {
            std::cout << "[WD " << watchdog_id << "] Deactivated (matched event_val_b)" << std::endl;
          }
        }
        break;
      case 2:
        if (observer_id.read() == contract.observer_id)
        {

          if (location.read() == contract.event_val_b)
          {
            last_timestamp = timestamp.read();
          }
          else if (location.read() == contract.event_val_a)
          {
            active = false;

            if (diff > contract.val_interval_stop ||
                diff < contract.val_interval_start)
            {
              violation.write(true);
              if (g_debug_mode) {
                std::cout << "[WD " << watchdog_id << "] VIOLATION (diff > stop || diff < start): diff=" << diff << " stop=" << contract.val_interval_stop << " start=" << contract.val_interval_start << std::endl;
              }

              //std::cout << "diff case 2 "<< std::endl;
            }
          }
        }
        break;
      case 3:
        if (observer_id.read() == contract.observer_id)
        {

          if (timestamp.read() < contract.event_val_b)
          {
            if (data_value.read() < contract.val_interval_start ||
                data_value.read() > contract.val_interval_stop)
            {
              if (g_debug_mode) {
                std::cout << "[WD " << watchdog_id << "] VIOLATION (diff > stop || diff < start): diff=" << diff << " stop=" << contract.val_interval_stop << " start=" << contract.val_interval_start << std::endl;
              }
              violation.write(true);
            }
          }
          else if (timestamp.read() > contract.event_val_b)
          {
            //std::cout << " stop at " << timestamp.read();
            active = false;
          }
        }
        break;
      case 4:
        if (observer_id.read() == contract.observer_id)
        {
          if (data_value.read() < contract.val_interval_start ||
              data_value.read() > contract.val_interval_stop)
          {
            if (g_debug_mode) {
              std::cout << "[WD " << watchdog_id << "] VIOLATION (diff > stop || diff < start): diff=" << diff << " stop=" << contract.val_interval_stop << " start=" << contract.val_interval_start << std::endl;
            }
            violation.write(true);
          }
          if (location.read() == contract.event_val_b)
          {
            active = false;
          }
        }
        break;
      default:
        std::ostringstream msg;
        msg << "Unknown internal monitor type: " << contract.type;
        SC_REPORT_WARNING("Watchdog", msg.str().c_str());
        break;
      }
    }
  }
};