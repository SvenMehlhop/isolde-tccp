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

SC_MODULE(Tb_iti)
{
    sc_in<bool> clk;
    sc_out<bool> new_val;
    sc_out<bool> finished_tb;
    sc_out<sc_uint<64>> iaddr;

    sc_int<64> cc;
    bool init = false;
    int targetCC = 0;
    bool finished = false;
    bool active = false; // Runtime flag
    std::string dasm_path;
    std::string iti_path;

    struct DasmFile
    {
        sc_int<64> cc;
        sc_uint<64> addr;
    };

    struct ItiFile
    {
        sc_uint<64> addr;
    };
    std::vector<DasmFile> dasmData;
    std::vector<ItiFile> itiData;

    std::vector<DasmFile> parseDasmFile(const std::string &filename)
    {
        std::vector<DasmFile> result;
        std::ifstream file(filename);
        std::string line;

        while (std::getline(file, line))
        {
            std::istringstream iss(line);
            std::string addrHex;

            if (iss >> cc >> addrHex)
            {
                if (addrHex.find("0x") == 0)
                {
                    addrHex = addrHex.substr(2);
                }
                try
                {
                    uint64_t addrValue = std::stoull(addrHex, nullptr, 16);
                    sc_uint<64> addr = addrValue;

                    DasmFile entry = {cc, addr};
                    result.push_back(entry);
                    // std::cout << "[DASM] cc: " << entry.cc << "dec(cc)" << std::dec << entry.cc << ", addr: 0x" << std::hex << entry.addr << std::endl;
                }
                catch (const std::invalid_argument &e)
                {
                    std::cerr << "[ERROR] Invalid hex value: '" << addrHex << "' in line: '" << line << "'" << std::endl;
                }
                catch (const std::out_of_range &e)
                {
                    std::cerr << "[ERROR] Hex value out of range: '" << addrHex << "' in line: '" << line << "'" << std::endl;
                }
            }
        }
        file.close();
        return result;
    }

    std::vector<ItiFile> parseItiFile(const std::string &filename)
    {
        std::vector<ItiFile> result;
        std::ifstream file(filename);
        std::string line;

        while (std::getline(file, line))
        {
            size_t pos = line.find("iadd= ");
            if (pos != std::string::npos)
            {
                std::string addrHex = line.substr(pos + 6, 18);
                sc_uint<64> addr = addrHex.c_str();
                ItiFile entry = {addr};
                result.push_back(entry);
                // std::cout << "[ITI] addr: 0x" << std::hex << entry.addr << std::endl;
            }
        }
        file.close();
        return result;
    }

    void run()
    {
        // std::cout << "ITI run tb" << std::endl;
        if (!active && !init)
        {
            SC_REPORT_INFO("ITIFileProcessor", "ITI not active.");
            finished_tb.write(true);
            init = true;
        }
        else if (!init && active)
        {
            dasmData = parseDasmFile(dasm_path);
            itiData = parseItiFile(iti_path);
            SC_REPORT_INFO("TestBench Iti", "ITI Testbench initialized.");

            cc = 0;
            init = true;
        }
        else if (init && active)
        {
            new_val.write(false);
            if (!dasmData.empty() && !itiData.empty())
            {
                targetCC = dasmData.front().cc;
                if (cc < targetCC)
                {
                    cc++;
                    return;
                }
                if (dasmData.front().addr == itiData.front().addr)
                {
                    /*std::cout << "[MATCH] cc: " << dasmData.front().cc
                      << ", DASM addr: 0x" << std::hex << dasmData.front().addr
                      << ", ITI addr: 0x" << std::hex << itiData.front().addr << std::endl;*/

                    new_val.write(true);
                    iaddr.write(itiData.front().addr);
                    dasmData.erase(dasmData.begin());
                    itiData.erase(itiData.begin());
                }
                else
                {
                    /*std::cout << "[NO MATCH] cc: " << dasmData.front().cc
                      << ", DASM addr: 0x" << std::hex << dasmData.front().addr
                      << ", ITI addr: 0x" << std::hex << itiData.front().addr << std::endl;*/

                    dasmData.erase(dasmData.begin());
                }

                cc++;
            }
            else if (dasmData.empty())
            {
                std::ostringstream msg;
                msg << "DASM data exhausted at " << sc_time_stamp();
                SC_REPORT_INFO("TestBench Iti", msg.str().c_str());
                finished_tb.write(true);
            }
            else if (itiData.empty() && !finished)
            {
                std::ostringstream msg;
                msg << "ITI data exhausted at " << sc_time_stamp();
                SC_REPORT_INFO("TestBench Iti", msg.str().c_str());
                finished_tb.write(true);
                finished = true;
            }
        }
    }

    SC_HAS_PROCESS(Tb_iti);
    Tb_iti(sc_module_name nm, bool is_active, std::string d_path, std::string i_path) 
        : sc_module(nm), active(is_active), dasm_path(d_path), iti_path(i_path)
    {
        SC_METHOD(run);
        sensitive << clk.pos();
    }
};
