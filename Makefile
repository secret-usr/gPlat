# gPlat Project Makefile
# Supports Linux (g++) and macOS (clang++)
# Features: Incremental build, Automatic dependency tracking, Multi-platform checks

# ==========================================
# 1. Platform Detection
# ==========================================
ifeq ($(OS),Windows_NT)
    $(error Error: Building on Windows is not supported due to POSIX threading model usage. Please use WSL (Windows Subsystem for Linux) or a Linux Virtual Machine.)
endif

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    CXX := clang++
    CXXFLAGS_PLATFORM := 
    LDFLAGS_PLATFORM := 
else ifeq ($(UNAME_S),Linux)
    CXX := g++
    CXXFLAGS_PLATFORM := 
    LDFLAGS_PLATFORM := 
else
    $(error Unsupported OS: $(UNAME_S))
endif

# ==========================================
# 2. General Settings
# ==========================================
BUILD_DIR := build
BIN_DIR := bin
LIB_DIR := lib

# Standard flags: C++17, Generate dependency info (-MMD -MP), Warnings, Debug symbols
CXXFLAGS := -std=c++17 -Wall -g -MMD -MP $(CXXFLAGS_PLATFORM)
LDFLAGS := $(LDFLAGS_PLATFORM)

# ==========================================
# 3. Module Definitions
# ==========================================

# --- Module: gplat (Main Server) ---
GPLAT_DIR := gplat
GPLAT_SRCS := $(wildcard $(GPLAT_DIR)/*.cxx)
GPLAT_OBJS := $(patsubst $(GPLAT_DIR)/%.cxx, $(BUILD_DIR)/$(GPLAT_DIR)/%.o, $(GPLAT_SRCS))
GPLAT_BIN := $(BIN_DIR)/gplat
GPLAT_LDLIBS := -lpthread -L$(LIB_DIR) -lhigplat -Wl,-rpath,'$$ORIGIN/../lib'

# --- Module: higplat (Shared Library) ---
HIGPLAT_DIR := higplat
HIGPLAT_SRCS := $(wildcard $(HIGPLAT_DIR)/*.cpp)
HIGPLAT_OBJS := $(patsubst $(HIGPLAT_DIR)/%.cpp, $(BUILD_DIR)/$(HIGPLAT_DIR)/%.o, $(HIGPLAT_SRCS))
HIGPLAT_LIB := $(LIB_DIR)/libhigplat.so
HIGPLAT_CXXFLAGS := -fPIC

# --- Module: createq (Tool) ---
CREATEQ_DIR := createq
CREATEQ_SRCS := $(wildcard $(CREATEQ_DIR)/*.main.cpp) # Avoid duplicate mains if any? No, just *.cpp
CREATEQ_SRCS := $(wildcard $(CREATEQ_DIR)/*.cpp)
CREATEQ_OBJS := $(patsubst $(CREATEQ_DIR)/%.cpp, $(BUILD_DIR)/$(CREATEQ_DIR)/%.o, $(CREATEQ_SRCS))
CREATEQ_BIN := $(BIN_DIR)/createq
CREATEQ_INCLUDES := -Iinclude
# Link against higplat. Using $ORIGIN allows running from bin/ without setting LD_LIBRARY_PATH
CREATEQ_LDFLAGS := -L$(LIB_DIR) -lhigplat -Wl,-rpath,'$$ORIGIN/../lib'

# --- Module: TestApplication ---
TESTAPP_DIR := TestApplication
TESTAPP_SRCS := $(wildcard $(TESTAPP_DIR)/*.cpp)
TESTAPP_OBJS := $(patsubst $(TESTAPP_DIR)/%.cpp, $(BUILD_DIR)/$(TESTAPP_DIR)/%.o, $(TESTAPP_SRCS))

# Handle include/hello.cpp for TestApplication
HELLO_SRC := include/hello.cpp
HELLO_OBJ := $(BUILD_DIR)/include/hello.o

TESTAPP_BIN := $(BIN_DIR)/TestApplication
TESTAPP_LDFLAGS := -L$(LIB_DIR) -lhigplat -Wl,-rpath,'$$ORIGIN/../lib'

# ==========================================
# 4. Targets
# ==========================================

.PHONY: all clean directories help

all: directories $(GPLAT_BIN) $(HIGPLAT_LIB) $(CREATEQ_BIN) $(TESTAPP_BIN)
	@echo "OK"

directories:
	@mkdir -p $(BIN_DIR) $(LIB_DIR)

# --- Rules for gplat ---
$(GPLAT_BIN): $(GPLAT_OBJS) $(HIGPLAT_LIB)
	@echo "Linking $@"
	@$(CXX) $(LDFLAGS) $(GPLAT_OBJS) $(GPLAT_LDLIBS) -o $@

$(BUILD_DIR)/$(GPLAT_DIR)/%.o: $(GPLAT_DIR)/%.cxx
	@mkdir -p $(@D)
	@echo "Compiling $<"
	@$(CXX) $(CXXFLAGS) -I$(GPLAT_DIR) -c $< -o $@

# --- Rules for higplat ---
$(HIGPLAT_LIB): $(HIGPLAT_OBJS)
	@echo "Linking Shared Lib $@"
	@$(CXX) -shared $(LDFLAGS) $^ -o $@

$(BUILD_DIR)/$(HIGPLAT_DIR)/%.o: $(HIGPLAT_DIR)/%.cpp
	@mkdir -p $(@D)
	@echo "Compiling $<"
	@$(CXX) $(CXXFLAGS) $(HIGPLAT_CXXFLAGS) -c $< -o $@

# --- Rules for createq ---
$(CREATEQ_BIN): $(CREATEQ_OBJS) $(HIGPLAT_LIB)
	@echo "Linking $@"
	@$(CXX) $(LDFLAGS) $(CREATEQ_OBJS) $(CREATEQ_LDFLAGS) -o $@

$(BUILD_DIR)/$(CREATEQ_DIR)/%.o: $(CREATEQ_DIR)/%.cpp
	@mkdir -p $(@D)
	@echo "Compiling $<"
	@$(CXX) $(CXXFLAGS) $(CREATEQ_INCLUDES) -c $< -o $@

# --- Rules for TestApplication ---
# TestApplication needs hello.o
$(TESTAPP_BIN): $(TESTAPP_OBJS) $(HELLO_OBJ) $(HIGPLAT_LIB)
	@echo "Linking $@"
	@$(CXX) $(LDFLAGS) $(TESTAPP_OBJS) $(HELLO_OBJ) $(TESTAPP_LDFLAGS) -o $@

$(BUILD_DIR)/$(TESTAPP_DIR)/%.o: $(TESTAPP_DIR)/%.cpp
	@mkdir -p $(@D)
	@echo "Compiling $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule for compiling hello.cpp
$(HELLO_OBJ): $(HELLO_SRC)
	@mkdir -p $(@D)
	@echo "Compiling $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@


clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR) $(BIN_DIR) $(LIB_DIR)
	@echo "OK"

help:
	@echo "Available targets:"
	@echo "  all      : Build all modules (gplat, higplat, createq, TestApplication)"
	@echo "  clean    : Remove build directories and binaries"

# Include dependency files
-include $(GPLAT_OBJS:.o=.d)
-include $(HIGPLAT_OBJS:.o=.d)
-include $(CREATEQ_OBJS:.o=.d)
-include $(TESTAPP_OBJS:.o=.d)
