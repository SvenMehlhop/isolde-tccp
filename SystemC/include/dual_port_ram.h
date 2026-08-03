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

template <int SIZE>
SC_MODULE(Dual_port_ram)
{
    // Port A: Write port
    sc_in<bool> clk_a;
    sc_in<bool> we_a;          // Write Enable
    sc_in<sc_uint<64>> addr_a; // Address
    sc_in<Contract> data_in_a; // Input data (contract)

    // Port B: Read port
    sc_in<bool> clk_b;
    sc_in<sc_uint<64>> addr_b;   // Address
    sc_out<Contract> data_out_b; // Output data

    // RAM storage
    Contract ram[SIZE];

    SC_CTOR(Dual_port_ram)
    {
        // Initialize RAM
        for (int i = 0; i < SIZE; i++)
        {
            ram[i] = Contract();
        }

        SC_METHOD(write_port);
        sensitive << clk_a.pos();
        dont_initialize();

        SC_METHOD(read_port);
        sensitive << clk_b.pos();
        dont_initialize();
    }

    void write_port()
    {
        if (we_a.read())
        {
            ram[addr_a.read()] = data_in_a.read();
            //std::cout << "Dual_port_ram Write to address " << addr_a.read()                      << ": " << data_in_a.read() << std::endl;
        }
    }

    void read_port()
    {
        data_out_b.write(ram[addr_b.read()]);
        //std::cout << "Dual_port_ram Read from address " << addr_b.read()                  << ": " << ram[addr_b.read()] << std::endl;
    }
};
