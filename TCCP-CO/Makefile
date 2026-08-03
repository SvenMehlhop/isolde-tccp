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
CXXFLAGS = -std=c++20 -I. -Iinclude -Wall -Wextra
LDFLAGS = 

BUILD_DIR = build
SRC_DIR = src
TARGET = $(BUILD_DIR)/tccp_co

SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

.PHONY: all clean run testMinimal testMinimalDebug

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

run: $(TARGET)
	./$(TARGET) $(PROJECT)

testMinimal: $(TARGET)
	./$(TARGET) testMinimal

testMinimalDebug: $(TARGET)
	./$(TARGET) testMinimal --debug


testFDL: $(TARGET)
	./$(TARGET) FDL

testLabel: $(TARGET)
	./$(TARGET) iti_tccp_label_test