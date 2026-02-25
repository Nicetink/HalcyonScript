# HalcyonScript - Makefile

CC = gcc
CFLAGS = -Wall -O2 -I./include -I./src/halgui
LDFLAGS = -mwindows -L./src/halgui -lhalgui -lcomctl32 -lgdi32 -ld3d11 -ldxgi -lgdiplus

SRCS = src/main.c src/token.c src/lexer.c src/ast.c src/parser.c src/value.c src/runtime.c src/project.c src/halgui_runtime.c src/halgui_gpu_stub.c src/gui.c src/builtin_api.c src/filesystem_api.c src/registry_api.c src/process_api.c src/compression_api.c src/console_api.c src/optical_api.c
OBJS = $(SRCS:.c=.o)
TARGET = halcyon.exe

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

console: LDFLAGS = -mconsole -lcomctl32 -lgdi32
console: $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean console
