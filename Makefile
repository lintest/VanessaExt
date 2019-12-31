CXX		  := g++ 
CXX_FLAGS := $(CXXFLAGS) -Wall -Wextra -std=c++17 -ggdb -fpermissive

BIN		:= bin
SRC		:= src
INC		:= include
LIB		:= lib

LIBRARIES	:= -lX11
EXECUTABLE	:= test

all: $(BIN)/$(EXECUTABLE)

run: clean all
	clear
	./$(BIN)/$(EXECUTABLE)

$(BIN)/$(EXECUTABLE): src/WinCtrl.cpp src/ProcMngr.cpp src/test.cpp src/stdafx.cpp
	$(CXX) $(CXX_FLAGS) -I$(INC) -L$(LIB) $^ -o $@ $(LIBRARIES)

clean:
	-rm $(BIN)/*
