CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -pedantic

all: simplify area_and_topology_preserving_polygon_simplification

simplify: main.cpp
	$(CXX) $(CXXFLAGS) main.cpp -o simplify

area_and_topology_preserving_polygon_simplification: simplify
	cp simplify area_and_topology_preserving_polygon_simplification

clean:
	rm -f simplify simplify.exe area_and_topology_preserving_polygon_simplification area_and_topology_preserving_polygon_simplification.exe main.exe main.obj

.PHONY: all clean
