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

template <int TRACEIFs, int CONTRACT_NUM>
SC_MODULE(Report_interface)
{
    sc_in<bool> in_fifo_overflow[TRACEIFs];
    sc_in<bool> in_violation[CONTRACT_NUM];
    sc_in<bool> clk;

    sc_out<bool> out_fifo_overflow;
    sc_out<bool> out_violation;
    sc_out<sc_uint<64>> out_violationMonitorID;

    sc_uint<4> cycle_counter = 0;
    bool fifo_overflow_flag = false;
    bool violation_flag = false;
    sc_uint<64> violationMonitorID_store = 0;

    void update_logic()
    {
        // Reset every 10 cycles
        if (cycle_counter >= 10)
        {
            cycle_counter = 0;
            fifo_overflow_flag = false;
            violation_flag = false;
            violationMonitorID_store = 0;
        }
        else
        {
            cycle_counter++;
        }

        for (int i = 0; i < TRACEIFs; i++)
        {
            if (in_fifo_overflow[i].read())
            {
                std::cout << "Fifo[" << i << "] Overflow!" << endl;
                fifo_overflow_flag = true;
                cycle_counter = 0;
            }
        }
        for (int i = 0; i < CONTRACT_NUM; i++)
        {
            if (in_violation[i].read())
            {
                violation_flag = true;
                violationMonitorID_store = i;
                // std::cout << "Violation of monitor: " << violationMonitorID_store << endl;
                cycle_counter = 0;
            }
        }

        // std::cout << fifo_overflow_flag << std::endl;
        out_fifo_overflow.write(fifo_overflow_flag);
        out_violation.write(violation_flag);
        out_violationMonitorID.write(violationMonitorID_store);
    }

    SC_CTOR(Report_interface)
    {
        SC_METHOD(update_logic);
        sensitive << clk.pos();
        dont_initialize();
    }
};
