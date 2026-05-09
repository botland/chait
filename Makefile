# Define the compiler
CC = gcc

# Compiler flags
CFLAGS = -g -pthread -Wall -Wextra -I./src -std=c11 -lcurl -lreadline $(shell pkg-config --cflags --libs libcjson)

# Build directories
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj

# Source files - fixed for multiagent
SOURCES = $(wildcard src/*.c) $(wildcard src/tools/*.c) $(wildcard src/multiagent/*.c)
OBJECTS = $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SOURCES))

# Target executable
TARGET = $(BUILD_DIR)/client

# Default target
all: $(BUILD_DIR) $(TARGET)

# Create build directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(OBJ_DIR)/tools
	mkdir -p $(OBJ_DIR)/multiagent

# Link the object files to create the target
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

# Compile source files into object files (handles subdirs)
$(BUILD_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up the build
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean