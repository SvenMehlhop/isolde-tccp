#    Copyright 2024, 2026 OFFIS e.V.
#    This activity has received funding from the Key Digital Technologies Joint Undertaking (KDT JU)
#    under grant agreement No 877056. 
#    The JU receives support from the European Union’s Horizon 2020
#    research and innovation programme and Spain, Italy, Austria, Germany, Finland, Switzerland.
#    https://www.offis.de/en/offis/project/isolde.html
#
#    OFFIS e.V. licenses this file
#    to you under the Apache License, Version 2.0 (the
#    "License"); you may not use this file except in compliance
#    with the License.  You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing,
#    software distributed under the License is distributed on an
#    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#    KIND, either express or implied.  See the License for the
#    specific language governing permissions and limitations
#    under the License.    
#    Contributors:
#        Sven Mehlhop - initial implementation

CXX = g++
LDFLAGS = -L. -L$(SYSTEMC_HOME)/lib-linux64 -lsystemc
CXXFLAGS = -DSC_ENABLE_MESSAGE_LOGGING -O3 -I$(SYSTEMC_HOME)/include -Iinclude -Isrc -std=c++17
WLFLAGG = -Wl,-rpath=$(SYSTEMC_HOME)/lib-linux64 -lm

BUILD_DIR = build
SRC_DIR = src
TARGET = $(BUILD_DIR)/main

SRCS = $(SRC_DIR)/main.cpp
OBJS = $(BUILD_DIR)/main.o

.PHONY: all clean run sim debug

all: $(TARGET)
	./$(TARGET)
	./eval_tccp.sh simulation_trace.vcd

$(TARGET): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(WLFLAGG)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

sim: $(TARGET)
	./$(TARGET)
	./eval_tccp.sh simulation_trace.vcd

debug: $(TARGET)
	./$(TARGET) --debug 2>&1 | tee debug.log
	./eval_tccp.sh simulation_trace.vcd

clean:
	rm -rf $(BUILD_DIR) simulation_trace.vcd debug.log main *.o