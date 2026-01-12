# HalGUI Makefile
# Standard GUI Framework for HalcyonScript

CC = gcc
CFLAGS = -Wall -Wextra -O2 -I.
LDFLAGS = -lgdi32 -lcomdlg32 -lole32 -lshell32 -mwindows

# Source files
SOURCES = halgui_core.c halgui_render.c halgui_widgets.c halgui_dialogs.c halgui_themes.c
OBJECTS = $(SOURCES:.c=.o)

# Output
LIBRARY = libhalgui.a

# Demo
DEMO_SRC = ../../examples/halgui_demo.c
DEMO_OUT = ../../examples/halgui_demo.exe

.PHONY: all clean demo

all: $(LIBRARY)

$(LIBRARY): $(OBJECTS)
	ar rcs $@ $^

%.o: %.c halgui.h
	$(CC) $(CFLAGS) -c $< -o $@

demo: $(LIBRARY) $(DEMO_SRC)
	$(CC) $(CFLAGS) $(DEMO_SRC) -L. -lhalgui $(LDFLAGS) -o $(DEMO_OUT)
	@echo "Demo built: $(DEMO_OUT)"

clean:
	rm -f $(OBJECTS) $(LIBRARY) $(DEMO_OUT)

# Build all and demo
build: all demo
	@echo "HalGUI build complete!"
