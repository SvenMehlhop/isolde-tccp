# SystemC Simulation - Timing Contracts Coprocessor

## Running the Simulation

```sh
git clone https://github.com/offis/isolde-tccp
cd isolde-tccp
git submodule update --init --recursive
cd SystemC/TCCP
export SYSTEMC_HOME=/usr/local/systemc-2.3.3 # Set to your SystemC directory
make
```

## Testbench
Simulation parameters are defined in [**`config.txt`**](config.txt) and [**`contracts.csv`**](contracts.csv).

### Clock Configuration
Clocks are defined in `config.txt`. Each clock takes 4 parameters: `period (ns)`, `duty_cycle`, `start_time (ns)`, and `initial_state (true/false)`.

Example:
```text
CLOCK_TCCP=1,0.5,120,true
CLOCK_DPRAM=1,0.5,10,true
```

In the testbench configuration, the TCCP is connected to five observers and consists of monitors for up to 100 contracts. The FIFOs inside the TCCP are able to buffer up to 10 events.

### Testbench Contracts
Contracts are managed in [**`contracts.csv`**](contracts.csv). You can specify which file to use in `config.txt` via `CONTRACT_FILE=contracts.csv`.
In this eample, the config refers to a manual contract located in the TCCP-CO folder at [../TCCP-CO/Output/Manual/contracts.txt](../TCCP-CO/Output/Manual/contracts.txt).

A contract definition in the CSV follows this format:
`exists, id, type, obs_id, val_a, val_b, val_interval_start, val_interval_stop`

Example:
`1, 0, 0, 1, 10, 20, 1000, 2000`

Would translate to:
Contract should be initialized, Contract_id 0, contract_type 0, Observer_id 1(TIP), Event_Val_a 10, Event_Val_b 20, Val_Interval_Start 1000, Val_Interval_Start 2000
Where the timing is measured in TCCP cycles. Meaning, this contract "0" would monitor the periodic occurrence of the PC 10. A new event with the specific PC value should be arriving 1000 to 2000 TCCP-clk cycles after the last one.

In the following table you find the contracts mentioned in the testbenches. The contracts for observer 0 are explained in section ITI, observer 1 in section TIP, observer 2 in section Camera (CAM), observer 3 in section ROS, and observer 4 in section Platon_HW.

| ID  | Monitor Type | Observer | Event/ Val a      | Event/ Val b      | ValInt Start | ValInt Stop  | Note |
|:---:|:-------:|:--------:|:-----------------:|:-----------------:|:------------:|:------------:|:------------:|
| 1   | 0       | 3 (ROS)  | 107               | 0                 | 300000       | 700000       |Periodic heartbeat|
| 2   | 1       | 3 (ROS)  | 107               | 106               | 0            | 50000        |Reaction latency|
| 4   | 1       | 1 (TIP)  | 2147483648        | 2147483676        | 10           | 20           ||
| 5   | 1       | 1 (TIP)  | 2147483648        | 2147483676        | 6            | 30           ||
| 6   | 0       | 4 (HW)   | 10                | 0                 | 80000        | 120000       |Periodic heartbeat|
| 7   | 4       | 4 (HW)   | 1                  | 2                 | 0            | 30           |Value range|
| 24  | 2       | 0 (ITI)  | 0x80000030        | 0x80000018        | 4            | 12           ||
| 32  | 3       | 2 (CAM)  | 3000              | 3100              | 1            | 1023         ||
| 33  | 3       | 2 (CAM)  | 3000              | 3100              | 2000         | 4025         |violation|
| 34  | 3       | 2 (CAM)  | 2200              | 2400              | 1            | 1023         |violation |
| 35  | 4       | 2 (CAM)  | 1                 | 14                | 700          | 900          |violation |
| 36  | 4       | 2 (CAM)  | 1                 | 14                | 800          | 900          |violation |
| 44  | 1       | 0 (ITI)  | 0x80000028        | 0x80000030        | 4            | 16           ||
| 45  | 0       | 0 (ITI)  | 0x80000018        | 0                 | 5            | 35           ||

The ID is referring to the contract/ monitor ID, Monitor refers to the monitor type, Event/ Val a+b refer to the 

### Scenario
Testbench settings are switched in the [**`config.txt`**](config.txt) file.
Example:
```text
ITI_ACTIVE=true
TIP_ACTIVE=false
CAM_ACTIVE=false
```

### Evaluation
After running the simulation (`./main`), you can use the evaluation script to check for violations:
```sh
./eval_tccp.sh
```
This script will report the total number of FIFO overflows and contract violations detected in the VCD trace.


### Iti
The ITI clock starts after 950 ns, with a new cycle every 2 ns (500 MHz) and a pulse length of 1 ns.
This can be modified in `config.txt` via:
`CLOCK_ITI=2,0.5,950,true`

Testbench input is the [.dasm file](../TestInput/ITI-test/trace_hart_0.dasm) and the [iti.trace file](../TestInput/ITI-test/iti.trace). The dasm file is used to create a somehow realistic time-wise sequence. For each instruction, one cycle passes, until the PC of an instructon matches with one of the iti-trace. The iti-trace functions as the real stimuli for the iti observer. 

Based on the contracts #44 and #45, we monitor the execution time between PC-value `0x8000000c` and `0x80000030` (#44), and the periodic occurences of PC-value `0x8000000c`(#45). You should see some violations of both only in the beginning.

Activating only contract #24 with the iti observer id, you should see some violations, but only in the beginning



### TIP
The TIP clock starts after 950 ns, with a new cycle every 2 ns (500 MHz) and a pulse length of 1 ns.
This can be modified in `config.txt` via:
`CLOCK_TIP=2,0.5,950,true`

The tip input is based on an testbench run on CVA6 with the TIP interface. The file can be found here: [tip-input](TestInput/tip-test/tip_port_0_signals_dump.txt).



### Camera
The camera clock starts after 950 ns, with a new cycle every 20 ns (50 MHz) and a pulse length of 10 ns.
This can be modified in `config.txt` via:
`CLOCK_CAM=20,0.5,950,true`

The camera test bench emulates a brightness sensor observing the capture of a driver in an automotive use case. It is based on a sensor with a digital reprentation of lux values ranging between 0 and 1023. The emulated behavior consists of a wake-up and initialization phase during which the sensor output is 0.
After initialization, the scenario involves the camera regularly taking pictures, which then need to be analyzed. While taking a picture, the brightness should be within a certain range to fulfill requirements for driver safety and the quality of subsequent computations.
Monitors 32, 33, 34, 35, and 36 showcase the use of TSBC contracts and monitor types 3 and 4. Monitors 32-34 are aimed at the time point after the sensor's setup phase. Monitor 32 adheres to regular behavior. After 3000 cycles, the sensor should be operational and output values between 1 and 1023. 
Monitor 33 showcases an error in the values. The sensor output is always lower than 1023, so the monitor for the contract indicating a value range between 2000 and 4025 should detect a violation.


Monitor 34 is displaying an error in the timeframe. The test bench outputs a 0 until around 3000 cycles; therefore, the contract is violated since the value is 0 between cycles 2200 and 2400.


Monitor 35 is also demonstrating typical behavior. Here, the timing is based on the current PC value. The testbench generates values between 700 and 900 between PCs 1 and 14, and then values between 1 and 900 afterwards.
Monitor 36's contract requires that the value stay between 800 and 900 during the camera's on phase. Since the test bench generates values between 700 and 900 for that phase, there is a possibility of a violation.



### ROS
The ROS observer (ID 3) clock starts after 95,000,000 ns, with a period of 100 ns (10 MHz).
This can be modified in `config.txt` via:
`CLOCK_ROS=100,0.1,95000000,false`

The ROS testbench (`tb_ros.cpp`) reads simulated ROS execution events from a JSON trace file containing timing model data (configured via `ROS_TRACE_FILE` in `config.txt`, e.g., `../../Dump/evalStreet/time_model.json`). 
Each event represents a specific ROS node execution state/transition. 
Monitors 1, 2, 3, 4, and 5 evaluate timing constraints on ROS processing pipelines (such as node execution heartbeats and end-to-end communication latencies between nodes). Tightened latency limits in monitors 9, 10, and 11 demonstrate contract violations under normal system jitters.

*Note: The TCCP clk freq should be modified based on the sampling rate of the ROS testbench, to ensure the ability to simulate in a reasonable timeframe.*

### Platon_HW
The Platon_HW observer (ID 4) clock starts after 95,000,000 ns, with a period of 100 ns (10 MHz).
This can be modified in `config.txt` via:
`CLOCK_PLATON_HW=100,0.1,95000000,false`

The Platon_HW testbench (`tb_platon_hw.cpp`) emulates a physical hardware system, such as a motor control module. It feeds motor speed values (RPMs) and control status values to the Platon_HW observer.
- Contract #6 monitors the periodic occurrence of motor speed value updates (heartbeat check).
- Contracts #7 and #8 showcase monitor type 4 (PC-scoped boundary constraints), verifying that the RPM values stay within safe operating ranges (e.g. 0 to 30 or 2700 to 3300) during specific program phases (between start and stop PCs).

*Note: The TCCP clk freq should be modified based on the sampling rate of the Platon_HW testbench, to ensure the ability to simulate in a reasonable timeframe.*
