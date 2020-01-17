TARGET=test_xcb

SOURCES=xcb_clip.cpp

LIBS=pthread X11 xcb

OBJECTS=$(SOURCES:.cpp=.o)
INCLUDES=
CXXLAGS=$(CXXFLAGS) $(INCLUDES) -m64 -fPIC -fpermissive -lX11 `libpng-config --cflags` `pkg-config --cflags xcb`

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
	g++ $(CXXLAGS) $(OBJECTS) -o $(TARGET) $(addprefix -l, $(LIBS)) `libpng-config --ldflags` `pkg-config --libs xcb` 

clean:
	-rm $(TARGET) *.o *.d
