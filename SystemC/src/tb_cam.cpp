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
#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <random>

static const int setupCycles = 100;
static const int camOnCycles = 15;
static const int camOffCycles = 85;
static const int tbCycles = 2500;
static const int upperBoundCamOnLux = 900;
static const int lowerBoundCamOnLux = 700;
static const int upperBoundCamOffLux = 900;
static const int lowerBoundCamOffLux = 1;

SC_MODULE(Tb_cam)
{
    sc_in<bool> clk;
    sc_out<bool> new_val;
    sc_out<bool> finished_tb;
    sc_out<sc_uint<64>> val;
    sc_out<sc_uint<64>> iaddr;

    int cc = 0;
    int pcc = 0;
    bool init = false;
    bool finished = false;
    bool active = false; // Runtime flag
    std::random_device rd;
    std::mt19937 gen{rd()};
    std::uniform_int_distribution<> distribOn{lowerBoundCamOnLux, upperBoundCamOnLux};
    std::uniform_int_distribution<> distribOff{lowerBoundCamOffLux, upperBoundCamOffLux};

    // TODO
    void run()
    {
        if (!active && !init && !finished)
        {
            SC_REPORT_INFO("TBCam", "TB CAM not active.");
            finished_tb.write(true);
            finished = true;
        }
        else if (!init && active)
        {
            new_val.write(true);
            val.write(0);
            iaddr.write(0);
            init = true;
        }
        else if (init && active)
        {
            cc++;
            new_val.write(false);
            if (cc < setupCycles)
            {
                new_val.write(true);
                val.write(0);
                iaddr.write(3454);
                return;
            }
            else if (cc > tbCycles)
            {
                if (!finished)
                {
                    std::ostringstream msg;
                    msg << "Cam data exhausted at " << sc_time_stamp();
                    SC_REPORT_INFO("TestBench Cam", msg.str().c_str());
                    finished_tb.write(true);
                    finished = true;
                }
            }
            else
            {
                pcc++;
                new_val.write(true);
                iaddr.write(pcc);
                //std::cout << " pcc: " << pcc << "end  ";
                if (pcc < camOnCycles)
                {
                    val.write(distribOn(gen));
                    new_val.write(true);
                }
                else if (pcc < camOffCycles)
                {
                    val.write(distribOff(gen));
                    new_val.write(true);
                }
                else
                {
                    pcc = 0;
                    iaddr.write(pcc);
                }
            }
        }
    }

    SC_HAS_PROCESS(Tb_cam);
    Tb_cam(sc_module_name nm, bool is_active = false) : sc_module(nm), active(is_active)
    {
        SC_METHOD(run);
        sensitive << clk.pos();
    }
};
