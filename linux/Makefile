CXX ?= g++
CC ?= gcc

BUILD_DIR = build
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -DLINUX_RIDS -D_POSIX_C_SOURCE=200809L \
           -Iinclude -I../RemoteIDSpoofer
CFLAGS = -std=c11 -Wall -Wextra -O2 -DLINUX_RIDS -D_POSIX_C_SOURCE=200809L \
         -Iinclude -I../RemoteIDSpoofer

LDFLAGS = -lpcap

CORE_DIR = ../RemoteIDSpoofer
PROFILES_JSON = $(CORE_DIR)/drone_profiles.json
GEN_PROFILES = scripts/gen_drone_profiles.py

CPP_SRCS = \
	src/main.cpp \
	src/config.cpp \
	src/waypoints.cpp \
	src/spoofer.cpp \
	src/wifi_tx.cpp \
	src/id_open_linux.cpp \
	$(CORE_DIR)/id_open.cpp \
	$(CORE_DIR)/id_open_beacon.cpp \
	$(CORE_DIR)/utm.cpp \
	$(CORE_DIR)/drone_profiles.cpp

C_SRCS = \
	$(CORE_DIR)/opendroneid.c \
	$(CORE_DIR)/wifi.c \
	$(CORE_DIR)/alt_unix_time.c

CPP_OBJS = $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(filter src/%,$(CPP_SRCS))) \
           $(patsubst $(CORE_DIR)/%.cpp,$(BUILD_DIR)/core_%.o,$(filter $(CORE_DIR)/%,$(CPP_SRCS)))
C_OBJS = $(patsubst $(CORE_DIR)/%.c,$(BUILD_DIR)/core_%.o,$(C_SRCS))

TARGET = rids-spoofer

.PHONY: all clean profiles

all: profiles $(TARGET)

profiles: $(CORE_DIR)/drone_profiles.cpp

$(CORE_DIR)/drone_profiles.cpp: $(PROFILES_JSON) $(GEN_PROFILES)
	python3 $(GEN_PROFILES) $(PROFILES_JSON) $@

$(TARGET): $(CPP_OBJS) $(C_OBJS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: src/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILD_DIR)/core_%.o: $(CORE_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILD_DIR)/core_%.o: $(CORE_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
