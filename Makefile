CC = gcc # The compiler

# Compiler flags:
# -Iinclude: Look for header files in the 'include' directory
# -Wall:     Enable all compiler's warning messages
CFLAGS = -Iinclude -Wall

# Final executable
TARGET = bin/dmes

# Find all .c files in the src directory
SRCS = $(wildcard src/*.c)

# Generate a list of object files (.o) from the source files
OBJS = $(SRCS:src/%.c=obj/%.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p bin
	$(CC) $(OBJS) -o $(TARGET)
	@echo "Build complete!"	# TODO: Print running instructions

obj/%.o: src/%.c
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf bin obj
	@echo "Cleanup complete!"

# Phony targets are not files
.PHONY: all clean