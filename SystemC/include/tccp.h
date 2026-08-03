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
#include "fifo.h"
// #include "trace_interface.h"
#include "report_interface.h"
#include "contract_fetch.h"
#include "contract_struct.h"
#include "fifo_demux.h"
#include "watchdog.h"

template <int TRACEIFs, int CONTRACTS>
SC_MODULE(Tccp)
{
    int ITIID = 0;
    int TIPID = 1;
    int CAMID = 2;
    int ROSID = 3;
    int PLATONHWID = 4;
    Fifo *fifoTip;
    Fifo *fifoIti;
    Fifo *fifoCam;
    Fifo *fifoRos;
    Fifo *fifoPlatonHW;
    Report_interface<TRACEIFs, CONTRACTS> *reportIF;
    Contract_fetch<CONTRACTS> *cFetch;
    Fifo_demux<TRACEIFs> *fifoDemux;
    Watchdog *watchdogs[CONTRACTS];

    sc_in<bool> clk;
    sc_in<sc_uint<64>> location[TRACEIFs];
    sc_in<sc_uint<64>> data_value[TRACEIFs];
    sc_in<bool> output_ready[TRACEIFs];
    sc_in<bool> observer_clk[TRACEIFs];
    sc_in<Contract> contract_ram_in;
    sc_out<bool> fifo_overflow;
    sc_out<bool> violation;
    sc_out<sc_uint<64>> violationMonitorID;
    sc_out<sc_uint<64>> contract_ram_addr;

    sc_vector<sc_signal<bool>> fifo_out_ready;
    sc_vector<sc_signal<bool>> fifo_out_overflow;
    sc_vector<sc_signal<bool>> selected;
    sc_vector<sc_signal<sc_uint<64>>> fifo_out_timestamp;
    sc_vector<sc_signal<sc_uint<64>>> fifo_out_location;
    sc_vector<sc_signal<sc_uint<64>>> fifo_out_data_value;

    sc_signal<bool> demux_out_ready{"demux_out_ready"};
    sc_signal<sc_uint<64>> demux_out_location{"demux_out_location"};
    sc_signal<sc_uint<64>> demux_out_timestamp{"demux_out_timestamp"};
    sc_signal<sc_uint<64>> demux_out_data_value{"demux_out_data_value"};
    sc_signal<sc_uint<8>> demux_out_observer_id{"demux_out_observer_id"};

    sc_signal<bool> cfetch_out_active{"cfetch_out_active"};
    sc_signal<Contract> cfetch_out_contract{"cfetch_out_contract"};
    sc_vector<sc_signal<bool>> select_watchdog{"select_watchdog", CONTRACTS};

    sc_vector<sc_signal<bool>> watchdog_out_violation{"watchdog_out_violation", CONTRACTS};

    sc_signal<bool> report_out_overflow{"report_out_overflow"};
    sc_signal<bool> report_out_violation{"report_out_violation"};
    sc_signal<sc_uint<64>> report_out_violationMonitorID{"report_out_violationMonitorID"};

    SC_CTOR(Tccp) : fifo_out_ready("fifo_out_ready", TRACEIFs),
                    fifo_out_overflow("fifo_out_overflow", TRACEIFs),
                    fifo_out_timestamp("fifo_out_timestamp", TRACEIFs),
                    fifo_out_location("fifo_out_location", TRACEIFs),
                    fifo_out_data_value("fifo_out_data_value", TRACEIFs),
                    selected("selected", TRACEIFs),
                    select_watchdog("select_watchdog", CONTRACTS),
                    watchdog_out_violation("watchdog_out_violation", CONTRACTS)
    {

        fifoIti = new Fifo("fifoIti");
        fifoTip = new Fifo("fifoTip");
        fifoCam = new Fifo("fifoCam");
        fifoRos = new Fifo("fifoRos");
        fifoPlatonHW = new Fifo("fifoPlatonHW");
        fifoDemux = new Fifo_demux<TRACEIFs>("fifoDemux");
        reportIF = new Report_interface<TRACEIFs, CONTRACTS>("reportIF");
        cFetch = new Contract_fetch<CONTRACTS>("cfetch");

        fifoIti->tccp_clk(clk);
        fifoIti->location(location[ITIID]);
        fifoIti->data_value(data_value[ITIID]);
        fifoIti->obs_clk(observer_clk[ITIID]);
        fifoIti->output_ready(output_ready[ITIID]);
        fifoIti->out_ready(fifo_out_ready[ITIID]);
        fifoIti->out_timestamp(fifo_out_timestamp[ITIID]);
        fifoIti->out_location(fifo_out_location[ITIID]);
        fifoIti->out_data_value(fifo_out_data_value[ITIID]);
        fifoIti->overflow(fifo_out_overflow[ITIID]);
        fifoIti->selected(selected[ITIID]);

        fifoTip->tccp_clk(clk);
        fifoTip->location(location[TIPID]);
        fifoTip->data_value(data_value[TIPID]);
        fifoTip->output_ready(output_ready[TIPID]);
        fifoTip->obs_clk(observer_clk[TIPID]);
        fifoTip->out_timestamp(fifo_out_timestamp[TIPID]);
        fifoTip->out_location(fifo_out_location[TIPID]);
        fifoTip->out_ready(fifo_out_ready[TIPID]);
        fifoTip->out_data_value(fifo_out_data_value[TIPID]);
        fifoTip->overflow(fifo_out_overflow[TIPID]);
        fifoTip->selected(selected[TIPID]);
        
        fifoCam->tccp_clk(clk);
        fifoCam->location(location[CAMID]);
        fifoCam->data_value(data_value[CAMID]);
        fifoCam->output_ready(output_ready[CAMID]);
        fifoCam->obs_clk(observer_clk[CAMID]);
        fifoCam->out_timestamp(fifo_out_timestamp[CAMID]);
        fifoCam->out_location(fifo_out_location[CAMID]);
        fifoCam->out_ready(fifo_out_ready[CAMID]);
        fifoCam->out_data_value(fifo_out_data_value[CAMID]);
        fifoCam->overflow(fifo_out_overflow[CAMID]);
        fifoCam->selected(selected[CAMID]);

        fifoRos->tccp_clk(clk);
        fifoRos->location(location[ROSID]);
        fifoRos->data_value(data_value[ROSID]);
        fifoRos->output_ready(output_ready[ROSID]);
        fifoRos->obs_clk(observer_clk[ROSID]);
        fifoRos->out_timestamp(fifo_out_timestamp[ROSID]);
        fifoRos->out_location(fifo_out_location[ROSID]);
        fifoRos->out_ready(fifo_out_ready[ROSID]);
        fifoRos->out_data_value(fifo_out_data_value[ROSID]);
        fifoRos->overflow(fifo_out_overflow[ROSID]);
        fifoRos->selected(selected[ROSID]);

        fifoPlatonHW->tccp_clk(clk);
        fifoPlatonHW->location(location[PLATONHWID]);
        fifoPlatonHW->data_value(data_value[PLATONHWID]);
        fifoPlatonHW->output_ready(output_ready[PLATONHWID]);
        fifoPlatonHW->obs_clk(observer_clk[PLATONHWID]);
        fifoPlatonHW->out_timestamp(fifo_out_timestamp[PLATONHWID]);
        fifoPlatonHW->out_location(fifo_out_location[PLATONHWID]);
        fifoPlatonHW->out_ready(fifo_out_ready[PLATONHWID]);
        fifoPlatonHW->out_data_value(fifo_out_data_value[PLATONHWID]);
        fifoPlatonHW->overflow(fifo_out_overflow[PLATONHWID]);
        fifoPlatonHW->selected(selected[PLATONHWID]);

        fifoDemux->clk(clk);
        fifoDemux->active(cfetch_out_active);
        fifoDemux->out_ready(demux_out_ready);
        fifoDemux->out_location(demux_out_location);
        fifoDemux->out_timestamp(demux_out_timestamp);
        fifoDemux->out_data_value(demux_out_data_value);
        fifoDemux->out_observer_id(demux_out_observer_id);
        for (int i = 0; i < TRACEIFs; i++)
        {
            fifoDemux->location[i](fifo_out_location[i]);
            fifoDemux->timestamp[i](fifo_out_timestamp[i]);
            fifoDemux->data_value[i](fifo_out_data_value[i]);
            fifoDemux->output_ready[i](fifo_out_ready[i]);
            fifoDemux->read[i](selected[i]);
        }
        cFetch->clk(clk);
        cFetch->addr(contract_ram_addr);
        cFetch->contract_out(cfetch_out_contract);
        cFetch->contracts_in(contract_ram_in);
        cFetch->active(cfetch_out_active);

        for (int i = 0; i < CONTRACTS; i++)
        {
            watchdogs[i] = new Watchdog(
                sc_gen_unique_name("Watchdog_"), i);
            watchdogs[i]->trigger(demux_out_ready);
            watchdogs[i]->location(demux_out_location);
            watchdogs[i]->timestamp(demux_out_timestamp);
            watchdogs[i]->data_value(demux_out_data_value);
            watchdogs[i]->observer_id(demux_out_observer_id);
            watchdogs[i]->selected(select_watchdog[i]);
            watchdogs[i]->in_contract(cfetch_out_contract);
            watchdogs[i]->violation(watchdog_out_violation[i]);
            reportIF->in_violation[i](watchdog_out_violation[i]);
            cFetch->select_watchdog[i](select_watchdog[i]);
        }

        reportIF->clk(clk);
        reportIF->in_fifo_overflow[TIPID](fifo_out_overflow[TIPID]);
        reportIF->in_fifo_overflow[ITIID](fifo_out_overflow[ITIID]);
        reportIF->in_fifo_overflow[CAMID](fifo_out_overflow[CAMID]);
        reportIF->in_fifo_overflow[ROSID](fifo_out_overflow[ROSID]);
        reportIF->in_fifo_overflow[PLATONHWID](fifo_out_overflow[PLATONHWID]);
        reportIF->out_fifo_overflow(report_out_overflow);
        reportIF->out_violation(report_out_violation);
        reportIF->out_violationMonitorID(report_out_violationMonitorID);

        SC_METHOD(update_reportIF);
        sensitive << clk.pos();
        dont_initialize();
        // sensitive << report_out_overflow << report_out_cFetchOverflow << report_out_violation << report_out_violationMonitorID;
    }
    void update_reportIF()
    {
        //std::cout << report_out_overflow.read() << std::endl;
        fifo_overflow.write(report_out_overflow.read());
        violation.write(report_out_violation.read());
        violationMonitorID.write(report_out_violationMonitorID.read());
    }

    ~Tccp()
    {
        delete fifoTip;
        delete fifoIti;
        delete fifoCam;
        delete fifoRos;
        delete fifoPlatonHW;
        delete reportIF;
        delete cFetch;
        delete fifoDemux;
        for (int i = 0; i < CONTRACTS; i++)
        {
            delete watchdogs[i];
        }
    }
};
