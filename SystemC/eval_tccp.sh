#!/bin/bash

#    Copyright 2024, 2026 OFFIS e.V.
#    This activity has received funding from the Key Digital Technologies Joint Undertaking (KDT JU)
#    under grant agreement No 877056. 
#    The JU receives support from the European Union’s Horizon 2020 research
#    and innovation programme and Spain, Italy, Austria, Germany, Finland, Switzerland.
#    https://www.offis.de/en/offis/project/isolde.html

#    OFFIS e.V. licenses this file
#    to you under the Apache License, Version 2.0 (the
#    "License"); you may not use this file except in compliance
#    with the License.  You may obtain a copy of the License at

#    http://www.apache.org/licenses/LICENSE-2.0

#    Unless required by applicable law or agreed to in writing,
#    software distributed under the License is distributed on an
#    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#    KIND, either express or implied.  See the License for the
#    specific language governing permissions and limitations
#    under the License.
#    Contributors:
#        Sven Mehlhop - initial implementation


# Usage: ./eval_tccp.sh [simulation_trace.vcd]

VCD_FILE=${1:-simulation_trace.vcd}

if [ ! -f "$VCD_FILE" ]; then
    echo "Error: File $VCD_FILE not found."
    exit 1
fi

get_symbol() {
    grep -a "\<$1\>" "$VCD_FILE" | grep -a "\$var" | awk -v name="$1" '{for(i=1;i<NF;i++) if($i==name) print $(i-1)}'
}

OVF_SYM=$(get_symbol "tccp_out_fifo_overflow")
VIO_SYM=$(get_symbol "tccp_out_violation")
ID_SYM=$(get_symbol "tccp_out_violationMonitorID")

if [ -z "$OVF_SYM" ] || [ -z "$VIO_SYM" ] || [ -z "$ID_SYM" ]; then
    echo "Error: Could not find required TCCP signals in VCD header."
    exit 1
fi

RESULTS=$(awk -v ovf_sym="$OVF_SYM" -v vio_sym="$VIO_SYM" -v id_sym="$ID_SYM" '
BEGIN {
    curr_id = 0;
    violation_count = 0;
    overflow_count = 0;
}

# Bus value change: b[01]* <symbol>
$1 ~ /^b[01]+/ && $2 == id_sym {
    bin_val = substr($1, 2);
    dec_val = 0;
    for(i=1; i<=length(bin_val); i++) {
        dec_val = dec_val * 2 + (substr(bin_val, i, 1) == "1" ? 1 : 0);
    }
    curr_id = dec_val;
    if (is_violation && curr_id != 0) v_ids[curr_id]++;
}

# Bit change value: <v><symbol>
$0 == "1"ovf_sym { overflow_count++; }
$0 == "1"vio_sym { 
    violation_count++; 
    is_violation = 1;
    if (curr_id != 0) v_ids[curr_id]++;
}
$0 == "0"vio_sym {
    is_violation = 0;
}

END {
    # Format: violation_count|overflow_count|id:count,id:count...
    ids = "";
    for (id in v_ids) {
        if (ids != "") ids = ids ", ";
        ids = ids id ":" v_ids[id];
    }
    printf "%d|%d|%s", violation_count, overflow_count, ids;
}
' "$VCD_FILE")

V_COUNT=$(echo "$RESULTS" | cut -d'|' -f1)
O_COUNT=$(echo "$RESULTS" | cut -d'|' -f2)
V_IDS=$(echo "$RESULTS" | cut -d'|' -f3)

echo "========================================"
echo "    TCCP Simulation Evaluation"
echo "========================================"
echo "VCD File:   $VCD_FILE"
echo "----------------------------------------"
echo "FIFO Overflows:      $O_COUNT"
echo "Contract Violations: $V_COUNT"

if [ "$V_COUNT" -gt 0 ]; then
    echo "Violated IDs:        $V_IDS"
fi

echo "========================================"
