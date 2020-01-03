CXX		  := g++ 
CXX_FLAGS := $(CXXFLAGS) -Wall -Wextra -std=c++17 -ggdb -fpermissive

BIN		:= bin
SRC		:= src
INC		:= include
LIB		:= lib

LIBRARIES	:= -lX11 -lpng -lXrandr
EXECUTABLE	:= test

SRC = \
	src/ProcMngr.cpp \
	src/ScreenMngr.cpp \
	src/WindowMngr.cpp \
	src/screenshot.cpp \
	src/stdafx.cpp \
	src/test.cpp

all: $(BIN)/$(EXECUTABLE)

run: clean all
	clear
	./$(BIN)/$(EXECUTABLE)

$(BIN)/$(EXECUTABLE): $(SRC)
	$(CXX) $(CXX_FLAGS) -I$(INC) -L$(LIB) $^ -o $@ $(LIBRARIES)

clean:
	-rm $(BIN)/*
