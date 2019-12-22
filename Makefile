CXX		  := g++ -fpermissive
CXX_FLAGS := -Wall -Wextra -std=c++17 -ggdb `pkg-config --cflags uuid`

BIN		:= bin
SRC		:= src
INCLUDE	:= include
LIB		:= lib

LIBRARIES	:= -lX11 `pkg-config --libs uuid`
EXECUTABLE	:= winlist


all: $(BIN)/$(EXECUTABLE)

run: clean all
	clear
	./$(BIN)/$(EXECUTABLE)

$(BIN)/$(EXECUTABLE): $(SRC)/*.cpp
	$(CXX) $(CXX_FLAGS) -I$(INCLUDE) -L$(LIB) $^ -o $@ $(LIBRARIES)

clean:
	-rm $(BIN)/*
