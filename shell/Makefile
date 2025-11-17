CC = gcc
CFLAGS = -std=c99 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -Wall -Wextra -Werror -Wno-unused-parameter -fno-asm
SHELL_DIR = /home/ameyb/Desktop/OSN/MP1/mini-project-1-AmeyBangera/shell
SRC_DIR = $(SHELL_DIR)/src
INCLUDE_DIR = $(SHELL_DIR)/include

# Include the header directory
CFLAGS += -I$(INCLUDE_DIR)

# Find all source files in src directory
SRCS = $(wildcard $(SRC_DIR)/*.c)
# Generate object file names
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(SRC_DIR)/%.o)
# Output executable
TARGET = $(SHELL_DIR)/shell.out

all: $(TARGET)

# Link object files to create the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile source files into object files
$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SRC_DIR)/*.o $(TARGET)

.PHONY: all clean