TARGET=AddInNative.so

SOURCES=AddInNative.cpp \
	ClipMngr.cpp \
	ProcMngr.cpp \
	ScreenMngr.cpp \
	WindowMngr.cpp \
	screenshot.cpp \
	screenshot.h \
	dllmain.cpp \
	xcb_clip.cpp \
	stdafx.cpp 

LIBS=pthread

OBJECTS=$(SOURCES:.cpp=.o)
INCLUDES=-I../include
CXXLAGS=$(CXXFLAGS) $(INCLUDES) -m64 -fPIC -fpermissive `libpng-config --cflags --ldflags`

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
