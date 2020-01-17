CXX		  := g++ 
CXX_FLAGS := $(CXXFLAGS) -Wall -Wextra -std=c++17 -ggdb -fpermissive `libpng-config --cflags` `pkg-config --cflags xcb`

BIN		:= bin
SRC		:= src
INC		:= include
LIB		:= lib

LIBRARIES	:= -lX11 -lpng -lXrandr -lpthread
EXECUTABLE	:= test

SRC = \
	src/ClipMngr.cpp \
	src/ProcMngr.cpp \
	src/ScreenMngr.cpp \
	src/WindowMngr.cpp \
	src/screenshot.cpp \
	src/xcb_clip.cpp \
	src/stdafx.cpp \
	src/test.cpp

all: $(BIN)/$(EXECUTABLE)

run: clean all
	clear
	./$(BIN)/$(EXECUTABLE)

$(BIN)/$(EXECUTABLE): $(SRC)
	$(CXX) $(CXX_FLAGS) -I$(INC) -L$(LIB) $^ -o $@ $(LIBRARIES) `libpng-config --ldflags` `pkg-config --libs xcb`

clean:
	-rm $(BIN)/*
