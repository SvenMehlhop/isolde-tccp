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

SC_MODULE(HW_observerPlatonHW)
{
    sc_in<bool>           clk{"clk"};
    sc_in<bool>           new_val;
    sc_in<sc_uint<64>>    iaddr;
    sc_in<sc_uint<64>>    values;

    sc_out<sc_uint<64>>   location;
    sc_out<sc_uint<64>>   data_value;
    sc_out<bool>          output_ready;

    void observe()
    {
        if (new_val.read())
        {
            location     = iaddr.read();
            data_value   = values.read();
            output_ready = true;
        }
        else
        {
            output_ready = false;
        }
    }

    SC_CTOR(HW_observerPlatonHW)
    {
        SC_METHOD(observe);
        sensitive << clk.pos();
        dont_initialize();
    }
};
