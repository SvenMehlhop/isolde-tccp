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
#include <systemc.h>

struct Contract
{
  sc_uint<8> id;                  //  ID
  sc_uint<8> type;                //  Type
  sc_uint<8> observer_id;         //  Observer ID
  sc_uint<64> event_val_a;        // Event Value A
  sc_uint<64> event_val_b;        // Event Value B
  sc_uint<64> val_interval_start; // Val Interval Start
  sc_uint<64> val_interval_stop;  // Val Interval Stop
  bool exists;

  bool operator==(const Contract &other) const
  {
    return id == other.id && type == other.type &&
           observer_id == other.observer_id &&
           event_val_a == other.event_val_a &&
           event_val_b == other.event_val_b &&
           val_interval_start == other.val_interval_start &&
           val_interval_stop == other.val_interval_stop &&
           exists == other.exists;
  }

  Contract(sc_uint<32> id, sc_uint<8> type, sc_uint<32> obs_id,
           sc_uint<64> val_a, sc_uint<64> val_b, sc_uint<64> start,
           sc_uint<64> stop, bool exists = false)
      : id(id), type(type), observer_id(obs_id), event_val_a(val_a),
        event_val_b(val_b), val_interval_start(start), val_interval_stop(stop),
        exists(exists) {}

  Contract()
      : id(0), type(0), observer_id(0), event_val_a(0), event_val_b(0),
        val_interval_start(0), val_interval_stop(0), exists(false) {}

  Contract &operator=(const Contract &other)
  {
    if (this != &other)
    {
      id = other.id;
      type = other.type;
      observer_id = other.observer_id;
      event_val_a = other.event_val_a;
      event_val_b = other.event_val_b;
      val_interval_start = other.val_interval_start;
      val_interval_stop = other.val_interval_stop;
      exists = other.exists;
    }
    return *this;
  }

  friend std::ostream &operator<<(std::ostream &os, const Contract &c)
  {
    os << "Contract: {ID: " << c.id.to_uint()
       << ", Type: " << (int)c.type.to_uint()
       << ", ObserverID: " << c.observer_id.to_uint()
       << ", Event_A: " << c.event_val_a.to_uint()
       << ", Event_B: " << c.event_val_b.to_uint() << ", Interval: ["
       << c.val_interval_start.to_uint() << ", "
       << c.val_interval_stop.to_uint() << "]"
       << ", Exists: " << (c.exists ? "true" : "false") << "}";
    return os;
  }
};
void sc_trace(sc_core::sc_trace_file *tf, const Contract &contract,
              const std::string &name)
{
  sc_trace(tf, contract.id, name + ".monitor_id");
  sc_trace(tf, contract.type, name + ".monitor_type");
  sc_trace(tf, contract.observer_id, name + ".monitor_observer_id");
  sc_trace(tf, contract.event_val_a, name + ".event_val_a");
  sc_trace(tf, contract.event_val_b, name + ".event_val_b");
  sc_trace(tf, contract.val_interval_start, name + ".val_interval_start");
  sc_trace(tf, contract.val_interval_stop, name + ".val_interval_stop");
  sc_trace(tf, contract.exists, name + ".exists");
}
