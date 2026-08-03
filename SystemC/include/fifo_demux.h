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

template <int TRACEIFs>
SC_MODULE(Fifo_demux)
{
    sc_in<bool> clk;
    sc_in<bool> active;
    sc_in<sc_uint<64>> location[TRACEIFs];
    sc_in<sc_uint<64>> timestamp[TRACEIFs];
    sc_in<sc_uint<64>> data_value[TRACEIFs];
    sc_in<bool> output_ready[TRACEIFs];

    sc_out<sc_uint<64>> out_location;
    sc_out<sc_uint<64>> out_timestamp;
    sc_out<sc_uint<64>> out_data_value;
    sc_out<sc_uint<8>> out_observer_id; 
    sc_out<bool> out_ready;
    sc_out<bool> read[TRACEIFs];

    int counter = 0;
    bool popped_last_cycle = false;

    SC_CTOR(Fifo_demux)
    {
        SC_METHOD(compare);
        sensitive << clk.pos();
        dont_initialize();
    }

    void compare()
    {

        if (!active.read())
        {
            //std::cout << "Fifo demux inactive" << std::endl;
            popped_last_cycle = false;
            return;
        }

        for (int i = 0; i < TRACEIFs; i++)
        {
            read[i].write(false);
        }

        out_data_value.write(0);
        out_location.write(0);
        out_timestamp.write(0);
        out_ready.write(false);
        out_observer_id.write(0);

        if (popped_last_cycle)
        {
            popped_last_cycle = false;
            return;
        }

        counter++;

        int lowest = 0;
        int lowestIndex = -1;

        for (int i = 0; i < TRACEIFs; i++)
        {
            if (output_ready[i].read())
            {
                if (lowest == 0 || timestamp[i].read() < lowest)
                {
                    lowest = timestamp[i].read();
                    lowestIndex = i;
                }
            }
        }
        if (lowestIndex != -1)
        {
            // Output the data from the selected interface
            out_ready.write(true);
            read[lowestIndex].write(true);
            out_timestamp.write(timestamp[lowestIndex].read());
            out_location.write(location[lowestIndex].read());
            out_data_value.write(data_value[lowestIndex].read());
            out_observer_id.write(lowestIndex);
            
            popped_last_cycle = true;
            counter = 0;
            return;
        } else if(counter > 16) {
            //std::cout << "No trigger for " << counter << " cycles, triggering watchdogs" << std::endl;
            out_data_value.write(0);
            out_location.write(0);
            out_timestamp.write(timestamp[0].read());
            out_ready.write(true);
            popped_last_cycle = true;
            counter=0;
        }

    }
};