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
#include "configManager.h"
#include "contract_struct.h"
#include "dual_port_ram.h"
#include "core_observerIti.h"
#include "core_observerTip.h"
#include "hw_observerCam.h"
#include "ros_observer.h"
#include "hw_observerPlatonHW.h"
#include "tb_cam.cpp"
#include "tb_fill_ram.h"
#include "tb_iti.cpp"
#include "tb_observer.h"
#include "tb_ros.cpp"
#include "tb_platon_hw.cpp"
#include "tb_tip.cpp"
#include "tccp.h"
#include <sysc/tracing/sc_trace.h>
#include <systemc.h>
#include <vector>

static const int TRACEIFs = 5;
static const int NUM_CONTRACTS = 100;
//[0] ITI
//[1] TIP
//[2] CAM
//[3] ROS
//[4] Platon_HW

SC_MODULE(main_system)
{
  Tb_iti *tb_iti;
  Tb_tip *tb_tip;
  Tb_cam *tb_cam;
  Tb_ros *tb_ros;
  Tb_platon_hw *tb_platon_hw;
  Core_observerTip *tip_observer;
  Core_observerIti *iti_observer;
  HW_observerCam *cam_observer;
  Ros_observer *ros_observer;
  HW_observerPlatonHW *platon_hw_observer;
  Tccp<TRACEIFs, NUM_CONTRACTS> *tccp;
  Tb_observer<TRACEIFs> *tb_observer;
  Dual_port_ram<NUM_CONTRACTS> *dp_ram;
  Tb_fill_ram *tb_fill_ram;

  sc_clock tccp_clk;
  sc_clock tip_clk;
  sc_clock iti_clk;
  sc_clock cam_clk;
  sc_clock dp_ram_clk;
  sc_clock ros_clk;
  sc_clock platon_hw_clk;

  sc_vector<sc_signal<bool>> tb_finished;
  sc_vector<sc_signal<sc_uint<64>>> location;
  sc_vector<sc_signal<sc_uint<64>>> data_value;
  sc_vector<sc_signal<bool>> output_ready;

  // ITI
  sc_signal<bool> tb_iti_new_val{"tb_iti_new_val"};
  sc_signal<sc_uint<64>> tb_iti_iaddr{"tb_iti_iaddr"};

  // TIP
  sc_signal<bool> tb_tip_new_val{"tb_tip_new_val"};
  sc_signal<sc_uint<64>> tb_tip_iaddr{"tb_tip_iaddr"};
  sc_signal<sc_uint<1>> tb_tip_iretire{"tb_tip_iretire"};
  sc_signal<sc_uint<64>> tb_tip_time_t{"tb_tip_time_t"};
  sc_signal<sc_uint<2>> tb_tip_priv{"tb_tip_priv"};
  sc_signal<sc_uint<64>> tb_tip_cause{"tb_tip_cause"};
  sc_signal<sc_uint<64>> tb_tip_tval{"tb_tip_tval"};
  sc_signal<sc_uint<8>> tb_tip_itype{"tb_tip_itype"};

  // CAM
  sc_signal<bool> tb_cam_new_val{"tb_cam_new_val"};
  sc_signal<sc_uint<64>> tb_cam_val{"tb_cam_val"};
  sc_signal<sc_uint<64>> tb_cam_iaddr{"tb_cam_iaddr"};

  // ROS
  sc_signal<bool> tb_ros_new_val{"tb_ros_new_val"};
  sc_signal<sc_uint<64>> tb_ros_iaddr{"tb_ros_iaddr"};

  // Platon_HW
  sc_signal<bool> tb_platon_hw_new_val{"tb_platon_hw_new_val"};
  sc_signal<sc_uint<64>> tb_platon_hw_val{"tb_platon_hw_val"};
  sc_signal<sc_uint<64>> tb_platon_hw_iaddr{"tb_platon_hw_iaddr"};

  sc_signal<bool> tccp_out_fifo_overflow{"tccp_out_fifo_overflow"};
  sc_signal<bool> tccp_out_violation{"tccp_out_violation"};
  sc_signal<sc_uint<64>> tccp_out_violationMonitorID{
      "tccp_out_violationMonitorID"};

  // dpram
  sc_signal<sc_uint<64>> dp_ram_addr_a{"dp_ram_addr_a"};
  sc_signal<Contract> dp_ram_din_a{"dp_ram_din_a"};
  sc_signal<bool> dp_ram_we_a{"dp_ram_we_a"};

  sc_signal<Contract> dp_ram_dout_b{"dp_ram_dout_b"};
  sc_signal<sc_uint<64>> dp_ram_addr_b{"dp_ram_addr_b"};
  sc_signal<bool> dp_ram_data_rdy{"dp_ram_data_rdy"};

  SC_HAS_PROCESS(main_system);
  main_system(sc_module_name nm, ConfigManager & config)
      : sc_module(nm),
        tccp_clk("clk", config.clock_tccp.period, SC_NS,
                 config.clock_tccp.duty_cycle, config.clock_tccp.start_time,
                 SC_NS, config.clock_tccp.initial_state),
        tip_clk("tip_clk", config.clock_tip.period, SC_NS,
                config.clock_tip.duty_cycle, config.clock_tip.start_time, SC_NS,
                config.clock_tip.initial_state),
        iti_clk("iti_clk", config.clock_iti.period, SC_NS,
                config.clock_iti.duty_cycle, config.clock_iti.start_time, SC_NS,
                config.clock_iti.initial_state),
        cam_clk("cam_clk", config.clock_cam.period, SC_NS,
                config.clock_cam.duty_cycle, config.clock_cam.start_time, SC_NS,
                config.clock_cam.initial_state),
        dp_ram_clk("dp_ram_clk", config.clock_dpram.period, SC_NS,
                   config.clock_dpram.duty_cycle, config.clock_dpram.start_time,
                   SC_NS, config.clock_dpram.initial_state),
        ros_clk("ros_clk", config.clock_ros.period, SC_NS,
                config.clock_ros.duty_cycle, config.clock_ros.start_time, SC_NS,
                config.clock_ros.initial_state),
        platon_hw_clk("platon_hw_clk", config.clock_platon_hw.period, SC_NS,
                      config.clock_platon_hw.duty_cycle,
                      config.clock_platon_hw.start_time, SC_NS,
                      config.clock_platon_hw.initial_state),
        tb_finished("tb_finished", TRACEIFs), location("location", TRACEIFs),
        data_value("data_value", TRACEIFs),
        output_ready("output_ready", TRACEIFs)

  {
    tb_observer = new Tb_observer<TRACEIFs>("tb_observer");

    dp_ram = new Dual_port_ram<NUM_CONTRACTS>("dp_ram");
    tb_fill_ram = new Tb_fill_ram("tb_fill_ram", config.contracts);

    tccp = new Tccp<TRACEIFs, NUM_CONTRACTS>("tccp");
    tccp->clk(tccp_clk);

    // ITI
    tb_iti = new Tb_iti("tb_iti", config.iti_active, config.iti_dasm_file,
                        config.iti_trace_file);
    iti_observer = new Core_observerIti("iti_observer");

    tb_iti->clk(iti_clk);
    tb_iti->new_val(tb_iti_new_val);
    tb_iti->iaddr(tb_iti_iaddr);
    tb_iti->finished_tb(tb_finished[0]);
    iti_observer->clk(iti_clk);
    iti_observer->new_val(tb_iti_new_val);
    iti_observer->iaddr(tb_iti_iaddr);
    iti_observer->location(location[0]);
    iti_observer->data_value(data_value[0]);
    iti_observer->output_ready(output_ready[0]);

    // TIP
    tb_tip = new Tb_tip("tb_tip", config.tip_active, config.tip_trace_file);
    tip_observer = new Core_observerTip("tip_observer");

    tb_tip->clk(tip_clk);
    tb_tip->new_val(tb_tip_new_val);
    tb_tip->finished_tb(tb_finished[1]);
    tb_tip->iretire(tb_tip_iretire);
    tb_tip->iaddr(tb_tip_iaddr);
    tb_tip->time_t(tb_tip_time_t);
    tb_tip->priv(tb_tip_priv);
    tb_tip->cause(tb_tip_cause);
    tb_tip->tval(tb_tip_tval);
    tb_tip->itype(tb_tip_itype);

    tip_observer->clk(tip_clk);
    tip_observer->new_val(tb_tip_new_val);
    tip_observer->iaddr(tb_tip_iaddr);
    tip_observer->iretire(tb_tip_iretire);
    tip_observer->time_t(tb_tip_time_t);
    tip_observer->priv(tb_tip_priv);
    tip_observer->cause(tb_tip_cause);
    tip_observer->tval(tb_tip_tval);
    tip_observer->itype(tb_tip_itype);
    tip_observer->location(location[1]);
    tip_observer->data_value(data_value[1]);
    tip_observer->output_ready(output_ready[1]);

    // CAM
    tb_cam = new Tb_cam("tb_cam", config.cam_active);
    cam_observer = new HW_observerCam("cam_observer");

    tb_cam->clk(cam_clk);
    tb_cam->new_val(tb_cam_new_val);
    tb_cam->val(tb_cam_val);
    tb_cam->iaddr(tb_cam_iaddr);
    tb_cam->finished_tb(tb_finished[2]);
    cam_observer->clk(cam_clk);
    cam_observer->new_val(tb_cam_new_val);
    cam_observer->values(tb_cam_val);
    cam_observer->iaddr(tb_cam_iaddr);
    cam_observer->location(location[2]);
    cam_observer->data_value(data_value[2]);
    cam_observer->output_ready(output_ready[2]);

    // ROS
    tb_ros = new Tb_ros("tb_ros", config.ros_active, config.ros_trace_file);
    ros_observer = new Ros_observer("ros_observer");

    tb_ros->clk(ros_clk);
    tb_ros->new_val(tb_ros_new_val);
    tb_ros->iaddr(tb_ros_iaddr);
    tb_ros->finished_tb(tb_finished[3]);
    ros_observer->clk(ros_clk);
    ros_observer->new_val(tb_ros_new_val);
    ros_observer->iaddr(tb_ros_iaddr);
    ros_observer->location(location[3]);
    ros_observer->data_value(data_value[3]);
    ros_observer->output_ready(output_ready[3]);

    // Platon_HW
    tb_platon_hw = new Tb_platon_hw("tb_platon_hw", config.platon_hw_active);
    platon_hw_observer = new HW_observerPlatonHW("platon_hw_observer");

    tb_platon_hw->clk(platon_hw_clk);
    tb_platon_hw->new_val(tb_platon_hw_new_val);
    tb_platon_hw->iaddr(tb_platon_hw_iaddr);
    tb_platon_hw->val(tb_platon_hw_val);
    tb_platon_hw->finished_tb(tb_finished[4]);
    platon_hw_observer->clk(platon_hw_clk);
    platon_hw_observer->new_val(tb_platon_hw_new_val);
    platon_hw_observer->iaddr(tb_platon_hw_iaddr);
    platon_hw_observer->values(tb_platon_hw_val);
    platon_hw_observer->location(location[4]);
    platon_hw_observer->data_value(data_value[4]);
    platon_hw_observer->output_ready(output_ready[4]);

    for (int i = 0; i < TRACEIFs; i++)
    {
      tb_observer->inputs[i](tb_finished[i]);
      tccp->location[i](location[i]);
      tccp->data_value[i](data_value[i]);
      tccp->output_ready[i](output_ready[i]);
    }
    tccp->observer_clk[0](iti_clk);
    tccp->observer_clk[1](tip_clk);
    tccp->observer_clk[2](cam_clk);
    tccp->observer_clk[3](ros_clk);
    tccp->observer_clk[4](platon_hw_clk);

    tccp->fifo_overflow(tccp_out_fifo_overflow);
    tccp->violation(tccp_out_violation);
    tccp->violationMonitorID(tccp_out_violationMonitorID);
    tccp->contract_ram_in(dp_ram_dout_b);
    tccp->contract_ram_addr(dp_ram_addr_b);

    // DP RAM
    dp_ram->clk_a(dp_ram_clk);
    dp_ram->clk_b(tccp_clk);
    dp_ram->addr_b(dp_ram_addr_b);
    dp_ram->data_out_b(dp_ram_dout_b);
    dp_ram->addr_a(dp_ram_addr_a);
    dp_ram->data_in_a(dp_ram_din_a);
    dp_ram->we_a(dp_ram_we_a);

    tb_fill_ram->clk(dp_ram_clk);
    tb_fill_ram->we(dp_ram_we_a);
    tb_fill_ram->addr(dp_ram_addr_a);
    tb_fill_ram->data_out(dp_ram_din_a);
  }

  ~main_system()
  {
    delete tb_iti;
    delete tb_tip;
    delete tb_cam;
    delete tb_ros;
    delete tb_platon_hw;
    delete tip_observer;
    delete iti_observer;
    delete cam_observer;
    delete ros_observer;
    delete platon_hw_observer;
    delete tccp;
    delete tb_observer;
    delete dp_ram;
    delete tb_fill_ram;
  }
};

void trace_all_signals(sc_trace_file *tf, sc_object *obj)
{
  if (auto sig = dynamic_cast<sc_signal<bool> *>(obj))
  {
    sc_trace(tf, *sig, sig->name());
  }
  else if (auto sig = dynamic_cast<sc_signal<sc_uint<1>> *>(obj))
  {
    sc_trace(tf, *sig, sig->name());
  }
  else if (auto sig = dynamic_cast<sc_signal<sc_uint<2>> *>(obj))
  {
    sc_trace(tf, *sig, sig->name());
  }
  else if (auto sig = dynamic_cast<sc_signal<sc_uint<8>> *>(obj))
  {
    sc_trace(tf, *sig, sig->name());
  }
  else if (auto sig = dynamic_cast<sc_signal<sc_uint<64>> *>(obj))
  {
    sc_trace(tf, *sig, sig->name());
  }

  for (auto *child : obj->get_child_objects())
  {
    trace_all_signals(tf, child);
  }
}

bool g_debug_mode = false;

int sc_main(int argc, char *argv[])
{
  std::string configFile = "config.txt";
  for (int i = 1; i < argc; i++)
  {
    std::string arg = argv[i];
    if (arg == "--debug" || arg == "-d")
    {
      g_debug_mode = true;
    }
    else if (arg[0] != '-')
    {
      configFile = arg;
    }
  }
  ConfigManager config(configFile);

  main_system ms("main_system", config);
  sc_trace_file *wf = sc_create_vcd_trace_file("simulation_trace");
  wf->set_time_unit(100, SC_PS);
  //trace_all_signals(wf, &ms);
  // Trace only outputs of TCC
  sc_trace(wf, ms.tccp_out_fifo_overflow, "tccp_out_fifo_overflow");
  sc_trace(wf, ms.tccp_out_violation, "tccp_out_violation");
  sc_trace(wf, ms.tccp_out_violationMonitorID, "tccp_out_violationMonitorID");
  sc_trace(wf, ms.data_value[4], "HW_PLATON_value");
  sc_start();
  sc_close_vcd_trace_file(wf);
  return 0;
}
