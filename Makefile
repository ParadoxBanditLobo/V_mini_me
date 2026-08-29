CXX ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra -Wpedantic -pthread
CPPFLAGS ?= -Iinclude $(shell pkg-config --cflags libpng x11)
LDLIBS ?= $(shell pkg-config --libs libpng x11) -ldl -pthread

SOURCES := $(wildcard src/*.cpp)
OBJECTS := $(SOURCES:.cpp=.o)
TARGET := V_mini_me

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDLIBS)

src/%.o: src/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

test: tests/test_direction
	./tests/test_direction

tests/test_direction: tests/test_direction.cpp src/Direction.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

clean:
	rm -f $(OBJECTS) $(TARGET) tests/test_direction

# Optional Windows x86-64 cross-build. Requires LLVM clang++ and lld-link.
windows-x64:
	./build_windows_x64.sh
