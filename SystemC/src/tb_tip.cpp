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
#include <deque>


SC_MODULE(Tb_tip)
{
    sc_in<bool> clk;
    sc_out<bool> new_val;
    sc_out<bool> finished_tb;
    sc_out<sc_uint<64>> iaddr;
    sc_out<sc_uint<1>> iretire;
    sc_out<sc_uint<64>> time_t;
    sc_out<sc_uint<2>> priv;
    sc_out<sc_uint<64>> cause;
    sc_out<sc_uint<64>> tval;
    sc_out<sc_uint<8>> itype;

    struct TipFile
    {
        sc_uint<1> iretire;
        sc_uint<64> iaddr;
        sc_uint<64> time_t;
        sc_uint<2> priv;
        sc_uint<64> cause;
        sc_uint<64> tval;
        sc_uint<8> itype;
    };

    bool init = false;
    uint64_t targetCC = 0;
    uint64_t cc = 0; // Cycle counter
    std::deque<TipFile> tipData;
    sc_signal<bool> clk_sig;
    bool finished = false;
    bool active = false; // Runtime flag
    std::string tip_path;

    std::deque<TipFile> parseTipFile(const std::string &filename)
    {
        std::deque<TipFile> result;
        std::ifstream file(filename);
        if (!file.is_open())
        {
            SC_REPORT_ERROR("TipFileParser", ("Could not open " + filename).c_str());
            return result;
        }

        std::string line;
        while (std::getline(file, line))
        {
            TipFile entry{};

            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);
            size_t pos;
            while ((pos = line.find("tip_o_[0].")) != std::string::npos)
                line.erase(pos, 10);

            std::istringstream iss(line);
            std::string token;
            while (std::getline(iss, token, ','))
            {
                token.erase(0, token.find_first_not_of(" \t"));
                token.erase(token.find_last_not_of(" \t") + 1);

                size_t eq = token.find('=');
                if (eq == std::string::npos)
                    continue;

                std::string field = token.substr(0, eq);
                std::string val_str = token.substr(eq + 1);

                field.erase(0, field.find_first_not_of(" \t"));
                field.erase(field.find_last_not_of(" \t") + 1);
                val_str.erase(0, val_str.find_first_not_of(" \t"));
                val_str.erase(val_str.find_last_not_of(" \t") + 1);

                uint64_t value = 0;
                try
                {
                    if (val_str.find("0x") == 0 || val_str.find("0X") == 0)
                        value = std::stoull(val_str, nullptr, 16);
                    else if (field == "itype")
                        value = std::stoull(val_str, nullptr, 16);
                    else
                        value = std::stoull(val_str, nullptr, 10);
                }
                catch (...)
                {
                    std::string msg = "Error parsing field '" + field + "' = '" + val_str + "'";
                    SC_REPORT_ERROR("TestBench TIP", msg.c_str());
                    continue;
                }

                if (field == "iretire")
                    entry.iretire = value;
                else if (field == "iaddr")
                    entry.iaddr = value;
                else if (field == "time_t")
                    entry.time_t = value;
                else if (field == "priv")
                    entry.priv = value;
                else if (field == "cause")
                    entry.cause = value;
                else if (field == "tval")
                    entry.tval = value;
                else if (field == "itype")
                    entry.itype = value;
            }

            result.push_back(entry);
        }

        return result;
    }

    void run()
    {
        if (!active && !init)
        {
            SC_REPORT_INFO("TestBench TIP", "TIP not active.");
            finished_tb.write(true);
            init = true;
        }
        else if (!init && active)
        {
            tipData = parseTipFile(tip_path);
            init = true;
            if (tipData.empty())
            {
                SC_REPORT_ERROR("TestBench TIP", "No TipFile entries found.");
                finished_tb.write(true);
                return;
            }
            // Get the next entry's time_t (target cycle)
            targetCC = tipData.front().time_t;
            SC_REPORT_INFO("TestBench TIP", "TIP Testbench initialized.");
        }
        else if (init && active && !finished)
        {
            // std::cout << "TIP Testbench running. Current CC: " << cc << ", Target CC: " << targetCC << ", Time: " << tipData.front().time_t << std::endl;
            new_val.write(false);
            if (!tipData.empty())
            {
                        while (!tipData.empty() && cc > tipData.front().time_t)
                {
                    tipData.pop_front();
                }

                        if (!tipData.empty() && cc == tipData.front().time_t)
                {
                    // std::cout << "At CC: " << cc << ", outputting TIP entry with iaddr: " << std::hex << tipData.front().iaddr << std::dec << " with size: " << tipData.size() << std::endl;
                    auto front = std::move(tipData.front());
                    new_val.write(true);
                    iretire.write(front.iretire);
                    iaddr.write(front.iaddr);
                    time_t.write(front.time_t);
                    priv.write(front.priv);
                    cause.write(front.cause);
                    tval.write(front.tval);
                    itype.write(front.itype);

                    // Remove the processed entry
                    tipData.pop_front();
                }
                cc++;
                if(cc>3500){
                    SC_REPORT_INFO("TestBench TIP", "TIP data terminated");
                    finished_tb.write(true);
                    finished = true;
                }
            }
            else if (tipData.empty())
            {
                if (!finished)
                {
                    SC_REPORT_INFO("TestBench TIP", "TIP data exhausted");
                    finished_tb.write(true);
                    finished = true;
                }
            }
        }
    }

    SC_HAS_PROCESS(Tb_tip);
    Tb_tip(sc_module_name nm, bool is_active, std::string t_path) 
        : sc_module(nm), active(is_active), tip_path(t_path)
    {
        SC_METHOD(run);
        sensitive << clk.pos();
    }
};
