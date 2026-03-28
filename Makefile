CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -pedantic
SOURCES := main.cpp geometry.cpp polygon.cpp simplifier.cpp io.cpp
OBJECTS := $(SOURCES:.cpp=.o)

all: simplify area_and_topology_preserving_polygon_simplification

simplify: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o simplify

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

area_and_topology_preserving_polygon_simplification: simplify
	cp simplify area_and_topology_preserving_polygon_simplification

clean:
	rm -f simplify simplify.exe area_and_topology_preserving_polygon_simplification area_and_topology_preserving_polygon_simplification.exe *.o main.exe main.obj

.PHONY: all clean
