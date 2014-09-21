SOURCES = iblt.cpp murmurhash3.cpp utilstrencodings.cpp \
	iblt_test.cpp
OBJECTS = $(SOURCES:.cpp=.o)

TARGET = iblt_test

CXX = c++
CCLD = c++
CXXFLAGS = -g -ggdb -Wall

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CCLD) -o $(TARGET) $(OBJECTS)

run: $(TARGET)
	$(TARGET)

clean:
	rm -f $(OBJECTS) $(TARGET)

# Recompile the world if any header changes,
# because it is the simplest possible thing
# that works. And compiling is plenty fast.
$(OBJECTS): *.h Makefile

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<
