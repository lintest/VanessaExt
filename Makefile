TARGET=AddInNative.so

SOURCES=src/AddInNative.cpp \
	src/ProcMngr.cpp \
	src/WinCtrl.cpp \
	src/convertor.cpp \
	src/dllmain.cpp \
	src/exports.cpp \
	src/stdafx.cpp 

LIBS=pthread

OBJECTS  := $(SOURCES:.cpp=.o)
INCLUDES := -Iinclude
CXXLAGS  := $(CXXFLAGS) $(INCLUDES) -m32 -fPIC `pkg-config --cflags uuid`

all: $(TARGET)

-include $(OBJECTS:.o=.d)

%.o: %.cpp
	g++ -c  $(CXXLAGS) $*.cpp -o $*.o
	g++ -MM $(CXXLAGS) $*.cpp >  $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

$(TARGET): $(OBJECTS) Makefile
	g++ $(CXXLAGS) -shared $(OBJECTS) -o $(TARGET) $(addprefix -l, $(LIBS))

clean:
	-rm $(TARGET) *.o *.d
