CC = g++
CFLAGS = -std=c++17 -Wall -Wextra
LDFLAGS = -lglfw -lGL -ldl

# Include paths
INCLUDES = -I./src -I./lib -I./lib/imgui -I./lib/imgui/backends -I./lib/glad/include

# Source files
SOURCES = src/main.cpp \
          glad/glad.c \
          imgui/imgui.cpp \
          imgui/imgui_draw.cpp \
          imgui/imgui_tables.cpp \
          imgui/imgui_widgets.cpp \
          imgui/imgui_demo.cpp \
          imgui/backends/imgui_impl_glfw.cpp \
          imgui/backends/imgui_impl_opengl3.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)
OBJECTS := $(OBJECTS:.c=.o)

# Target executable
TARGET = mesh_viewer

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Build successful! Run with: ./$(TARGET) <mesh_file.off>"

%.o: %.cpp
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean

# Copy shaders to working directory (if needed)
copy_resources:
	cp -r shaders .
	cp -r models .