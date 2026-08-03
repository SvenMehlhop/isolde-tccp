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
#include <string>
#include <iostream>
#include <random>

static const uint64_t MCU_STATE_IDLE      = 1;
static const uint64_t MCU_STATE_RAMP      = 2;
static const uint64_t MCU_STATE_FAST      = 3;

static const uint64_t MCU_ADDR_HEARTBEAT  = 10; // Moved away from state IDs

// Nominal RPM values
static const int MCU_RPM_IDLE_TARGET = 0;
static const int MCU_RPM_FAST_TARGET = 3000;

// Probability of injecting a heartbeat gap (0.2% = 20 / 10000)
static const int MCU_HEARTBEAT_MISS_PROB = 20;

// Probability of injecting a fault RPM (0.25% = 25 / 10000)
static const int MCU_FAULT_PROB = 45;

// Fault RPM Ranges (Overspeed is still > 3200)
static const int MCU_RPM_OVERSPEED_MIN = 3200;
static const int MCU_RPM_OVERSPEED_MAX = 3800;
static const int MCU_RPM_STALL_MIN     = 0;
static const int MCU_RPM_STALL_MAX     = 100;

// nominal updates 
static const int MCU_UPDATE_INTERVAL_TICKS = 100000;

// 30s simulation time at 100ns clock
static const int MCU_SIM_TICKS = 300000000;

SC_MODULE(Tb_platon_hw)
{
    sc_in<bool>           clk;
    sc_out<bool>          new_val;
    sc_out<bool>          finished_tb;
    sc_out<sc_uint<64>>   iaddr;
    sc_out<sc_uint<64>>   val;

    bool     init     = false;
    bool     finished = false;
    bool     active   = false;
    uint64_t tick     = 0;

    bool     write_state_event = false;
    uint64_t next_state_id     = 0;
    uint64_t next_rpm_val      = 0;

    // Dynamic profile state
    double   current_rpm_dyn = 0.0;
    double   target_rpm_dyn  = 0.0;
    int      updates_to_next_target = 0;

    std::mt19937                          gen{std::random_device{}()};
    std::uniform_int_distribution<int>    dist_target   {0, 3100};    // Target stays in nominal
    std::uniform_int_distribution<int>    dist_duration {100, 500};   // 1s - 5s between target changes
    std::uniform_int_distribution<int>    dist_overspeed{MCU_RPM_OVERSPEED_MIN,
                                                         MCU_RPM_OVERSPEED_MAX};
    std::uniform_int_distribution<int>    dist_stall    {0, 200};     // Temporary stall
    std::uniform_int_distribution<int>    dist_noise    {-25, 25};    // Increased jitter
    std::uniform_int_distribution<int>    dist_miss     {0, 9999};    // 0.01% steps

    void run()
    {
        if (!active && !init)
        {
            SC_REPORT_INFO("Tb_platon_hw", "TB Platon_HW not active.");
            finished_tb.write(true);
            finished = true;
            init = true;
            return;
        }

        if (!init && active)
        {
            SC_REPORT_INFO("Tb_platon_hw", "Platon_HW Testbench initialized (RPM mode).");
            iaddr.write(MCU_STATE_IDLE);
            init = true;
            return;
        }

        if (init && active && !finished)
        {
            if (tick % 1000000 == 0)
            {
                std::cout << "[PROGRESS] Tb_platon_hw: Tick " << tick << " of " << MCU_SIM_TICKS << " (" << (tick * 100.0 / MCU_SIM_TICKS) << "%) at " << sc_time_stamp() << std::endl;
            }
            new_val.write(false);

            if (tick >= static_cast<uint64_t>(MCU_SIM_TICKS))
            {
                std::ostringstream msg;
                msg << "Motor control simulation complete at " << sc_time_stamp();
                SC_REPORT_INFO("Tb_platon_hw", msg.str().c_str());
                finished_tb.write(true);
                finished = true;
                return;
            }

            if (write_state_event)
            {
                iaddr.write(next_state_id);
                val.write(next_rpm_val);
                new_val.write(true);
                write_state_event = false;
            }
            else if (tick % MCU_UPDATE_INTERVAL_TICKS == 0 && tick > 0)
            {
                // Calculate phase-based target and state ID
                // Phases: 
                // 0-5s: Idle (0-5M)
                // 5-10s: Ramp Up (5M-10M)
                // 10-20s: Fast (10M-20M)
                // 20-25s: Ramp Down (20M-25M)
                // 25-30s: Idle (25M-30M)
                uint64_t state_id = MCU_STATE_IDLE;
                double   target = MCU_RPM_IDLE_TARGET;

                if (tick < 50000000) {
                    state_id = MCU_STATE_IDLE;
                    target = MCU_RPM_IDLE_TARGET;
                } else if (tick < 100000000) {
                    state_id = MCU_STATE_RAMP;
                    target = MCU_RPM_FAST_TARGET;
                } else if (tick < 200000000) {
                    state_id = MCU_STATE_FAST;
                    target = MCU_RPM_FAST_TARGET;
                } else if (tick < 250000000) {
                    state_id = MCU_STATE_RAMP;
                    target = MCU_RPM_IDLE_TARGET;
                } else {
                    state_id = MCU_STATE_IDLE;
                    target = MCU_RPM_IDLE_TARGET;
                }

                // Smooth ramp
                double ramp_step = 10.0; // 1000 RPM/s at 10ms updates (100000 ticks)
                double diff = target - current_rpm_dyn;
                if (std::abs(diff) <= ramp_step) current_rpm_dyn = target;
                else if (diff > 0) current_rpm_dyn += ramp_step;
                else current_rpm_dyn -= ramp_step;

                bool heartbeat_miss = (dist_miss(gen) < MCU_HEARTBEAT_MISS_PROB);

                if (!heartbeat_miss)
                {
                    bool fault_rpm = (dist_miss(gen) < MCU_FAULT_PROB);
                    int rpm_val;
                    std::string fault_type = "";

                    if (fault_rpm)
                    {
                        if (dist_miss(gen) < 5000) {
                            rpm_val = dist_overspeed(gen);
                            fault_type = " [OVERSPEED]";
                        } else {
                            rpm_val = dist_stall(gen);
                            fault_type = " [STALL]";
                        }
                    }
                    else
                    {
                        rpm_val = static_cast<int>(current_rpm_dyn) + dist_noise(gen);
                        if (rpm_val < 0) rpm_val = 0;
                    }

                    // Write heartbeat event using the dedicated address MCU_ADDR_HEARTBEAT (10)
                    iaddr.write(MCU_ADDR_HEARTBEAT);
                    val.write(static_cast<uint64_t>(rpm_val));
                    new_val.write(true);

                    // Queue the state ID update event to be written in the next clock cycle
                    next_state_id = state_id;
                    next_rpm_val = static_cast<uint64_t>(rpm_val);
                    write_state_event = true;

                    // std::cout << "[Platon_HW] tick=" << tick
                    //            << " rpm=" << rpm_val << " RPM"
                    //            << " state=" << state_id
                    //            << fault_type << std::endl;
                }
                else
                {
                    // SC_REPORT_INFO("Tb_platon_hw", "Heartbeat miss injected.");
                }
            }

            tick++;
        }
    }

    SC_HAS_PROCESS(Tb_platon_hw);
    Tb_platon_hw(sc_module_name nm, bool is_active = false)
        : sc_module(nm), active(is_active)
    {
        SC_METHOD(run);
        sensitive << clk.pos();
    }
};
