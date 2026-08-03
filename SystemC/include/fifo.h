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

SC_MODULE(Fifo)
{
    sc_in<bool> obs_clk;
    sc_in<bool> tccp_clk;
    sc_in<sc_uint<64>> location;
    sc_in<sc_uint<64>> data_value;
    sc_in<bool> output_ready;
    sc_in<bool> selected;

    sc_out<bool> out_ready;
    sc_out<sc_uint<64>> out_timestamp;
    sc_out<sc_uint<64>> out_location;
    sc_out<sc_uint<64>> out_data_value;
    sc_out<bool> overflow;
    
    sc_signal<sc_uint<64>> current_quesize{"current_quesize"}; // TODO just for debug reasons

    std::vector<std::tuple<sc_uint<64>, sc_uint<64>, sc_uint<64>>> fifo_queue;
    const size_t fifo_length = 10;
    sc_uint<64> timestamp = 0;
    sc_signal<sc_uint<64>> ts_debug{"ts_debug"};// TODO just for debug reasons
    

    void write_fifo()
    {
        //std::cout << "Aktuelle Simulationszeit: " << sc_time_stamp() << std::endl;

        if (fifo_queue.size() < fifo_length)
        {

            if (output_ready.read())
            {
                //std::cout << "Fifo reg! At: " << timestamp << " Queue length " << fifo_queue.size() << " loc: " << location.read() << std::endl;
                fifo_queue.push_back(std::make_tuple(
                    timestamp,
                    location.read(),
                    data_value.read()));
            }
        }
        // DEBUG
        /*if (fifo_queue.size() >= fifo_length)
        {
            overflow.write(1);
            std::cout << "FIFO Overflow! Input data dropped. At: " << timestamp << std::endl;
        }
        else
        {
            overflow.write(0);
        }*/

        // TODO -> after removal of debug log
        overflow.write(fifo_queue.size() >= fifo_length);
        //std::cout << "queuesize " << fifo_queue.size() << std::endl;
        current_quesize.write(fifo_queue.size());
    }

    void iterate()
    {
        timestamp++;
        ts_debug.write(timestamp);

        if (selected.read())
        {
            if (!fifo_queue.empty())
            {
                //std::cout << "pop at " << timestamp << std::endl;
                pop_front(fifo_queue);
            }
        }

        // std::cout << "trigger read fifo" << std::endl;
        if (!fifo_queue.empty())
        {

            auto front = fifo_queue.front();
            out_timestamp.write(std::get<0>(front));
            out_location.write(std::get<1>(front));
            out_data_value.write(std::get<2>(front));
            out_ready.write(true);
            // std::cout << "Fifo: " << std::get<1>(front) << std::endl;
        }
        else
        {
            out_ready.write(false);
            out_timestamp.write(timestamp);
            
            // std::cout << "fifo queue empty" << std::endl;
        }
    }
    template <typename V>
    void pop_front(V & v)
    {
        assert(!v.empty());
        v.erase(v.begin());
    }

    SC_CTOR(Fifo)
    {
        SC_METHOD(write_fifo);
        sensitive << obs_clk.pos();
        dont_initialize();

        SC_METHOD(iterate);
        sensitive << tccp_clk.pos();
        dont_initialize();
    }
};