CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -pedantic -Isrc
SOURCES := src/main.cpp src/geometry.cpp src/polygon.cpp src/simplifier.cpp src/io.cpp
OBJECTS := $(SOURCES:.cpp=.o)

all: simplify area_and_topology_preserving_polygon_simplification

simplify: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o simplify

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

area_and_topology_preserving_polygon_simplification: simplify
	cp simplify area_and_topology_preserving_polygon_simplification

clean:
	rm -f simplify simplify.exe area_and_topology_preserving_polygon_simplification area_and_topology_preserving_polygon_simplification.exe src/*.o main.exe main.obj

.PHONY: all clean
