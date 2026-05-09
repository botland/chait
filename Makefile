# Define the compiler
CC = gcc

# Compiler flags
CFLAGS = -g -lcurl -pthread -lreadline $(shell pkg-config --cflags --libs libcjson)

# Build directories
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj

# Source files - explicit curated list only (no wildcards, no placeholders)
MAIN_SRC = main.c client.c curl_utils.c json_utils.c markdown.c tool_calls.c history.c command.c socket.c registry.c event.c
TOOLS = get_file_content.c find_file_path.c save_to_file.c

# Object files - only core + tools (multiagent sources removed to prevent build failure from incomplete files)
OBJ = $(addprefix $(OBJ_DIR)/,$(MAIN_SRC:.c=.o)) \
      $(addprefix $(OBJ_DIR)/tools/,$(TOOLS:.c=.o))

# Target executable
TARGET = $(BUILD_DIR)/client

# Default target
all: $(BUILD_DIR) $(TARGET)

# Create build directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(OBJ_DIR)/tools

# Link the object files to create the target
$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

# Compile source files into object files
$(OBJ_DIR)/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/tools/%.o: src/tools/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up the build
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean