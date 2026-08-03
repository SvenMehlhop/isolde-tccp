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
#include <deque>
#include <tuple>
#include <vector>
#include <iostream>
#include "contract_struct.h"

template <int CONTRACTS>
SC_MODULE(Contract_fetch)
{
    sc_in<bool> clk;
    sc_in<Contract> contracts_in;

    sc_out<sc_uint<64>> addr;
    sc_out<bool> select_watchdog[CONTRACTS];
    sc_out<Contract> contract_out;
    sc_out<bool> active;

    int current_id = 0;

    void fetch()
    {
        if (current_id < 2)
        {
            addr.write(current_id);
            current_id++;
        }
        else if (current_id < CONTRACTS)
        {
            //std::cout << "Contract_fetch checking for ID: " << current_id - 2 << " with adjacent cid " << contracts_in.read().id << std::endl;
            if (current_id > 2)
            {
                select_watchdog[current_id - 3].write(false);
            }
            if (!contracts_in.read().exists)
            {
                //std::cout << "Contract_fetch no contract for ID: " << current_id - 2 << " with cid " << contracts_in.read().id << std::endl;
                addr.write(current_id);
                current_id++;
            }
            else
            {
                //std::cout << "Contract_fetch fetched contract ID: " << contracts_in.read().id << std::endl;
                //std::cout << contracts_in.read() << std::endl;
                contract_out.write(contracts_in.read());
                select_watchdog[current_id - 2].write(true);
                addr.write(current_id);
                current_id++;
            }
        }
        else
        {
            active.write(true);
        }
    }
    SC_CTOR(Contract_fetch)
    {
        SC_METHOD(fetch);
        sensitive << clk.pos();
        dont_initialize();
    }
};
