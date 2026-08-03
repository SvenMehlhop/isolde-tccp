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
#include <vector>

template <unsigned int N>
SC_MODULE(Tb_observer) {
    sc_in<bool> inputs[N];
    SC_CTOR(Tb_observer) {
        SC_METHOD(check);
        for (unsigned int i = 0; i < N; i++) {
            sensitive << inputs[i];
        }
        dont_initialize();
    }

    void check() {
        bool all_true = true;
        for (unsigned int i = 0; i < N; i++) {
            if (!inputs[i].read()) {
                all_true = false;
                break;
            }
        }

        if (all_true) {
           std::ostringstream msg;
           msg << "All testbenches are finished, terminating simulation at " << sc_time_stamp();
           SC_REPORT_INFO("TestbenchObserver", msg.str().c_str());
           sc_stop();
        }
    }
};