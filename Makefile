CC = g++
CFLAGS = -Wall -Wextra -std=c++17 \
         $(shell sdl2-config --cflags) \
         $(shell pkg-config --cflags SDL2_ttf SDL2_image SDL2_mixer)
LDFLAGS = $(shell sdl2-config --libs) \
          $(shell pkg-config --libs SDL2_ttf SDL2_image SDL2_mixer)

SRC = src/main.cpp \
      src/app.cpp \
      src/config.cpp \
      src/textures.cpp \
      src/renderer.cpp \
      src/blocks.cpp \
      src/navbar.cpp \
      src/filemenu.cpp \
      src/tab_bar.cpp \
      src/categories.cpp \
      src/palette.cpp \
      src/canvas.cpp \
      src/drag_area.cpp \
      src/stage.cpp \
      src/settings.cpp \
      src/block_ui.cpp \
      src/costumes_tab.cpp\
      src/sounds_tab.cpp\
      src/sprite_panel.cpp\
      src/workspace.cpp\
      src/interpreter.cpp\
      src/audio.cpp\
      src/dotenv.cpp

OBJ = $(SRC:.cpp=.o)
TARGET = scratch_clone

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

src/%.o: src/%.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean