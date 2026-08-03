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

SC_MODULE(Tb_fill_ram)
{
    sc_in<bool> clk;
    sc_out<bool> we;
    sc_out<sc_uint<64>> addr;
    sc_out<Contract> data_out;
    std::vector<Contract> contracts;
    int i = 0;
    bool finished = false;

    void process()
    {
        if (i < contracts.size() && !finished)
        {
            we.write(true);
            addr.write(contracts[i].id.to_int());
            Contract contract = contracts[i];
            data_out.write(contract);
            i++;
        }
        else if (!finished)
        {
            std::ostringstream msg;
            msg << "RAM filled with " << contracts.size() << " contracts at " << sc_time_stamp();
            SC_REPORT_INFO("TestBench Fill Ram", msg.str().c_str());
            we.write(false); // Stop writing
            finished = true;
            return;
        }
        
    }

    Tb_fill_ram(sc_module_name nm, const std::vector<Contract>& initial_contracts) 
        : sc_module(nm), contracts(initial_contracts)
    {
        SC_METHOD(process);
        sensitive << clk.pos();
        dont_initialize();
    }

    SC_CTOR(Tb_fill_ram)
    {
        SC_METHOD(process);
        sensitive << clk.pos();
        dont_initialize();
    }
};