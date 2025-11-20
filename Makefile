# SFML directory (updated for SFML 2.6.2)
SFML_INC = /opt/homebrew/Cellar/sfml/2.6.2/include
SFML_LIB = /opt/homebrew/Cellar/sfml/2.6.2/lib

# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++11 -Wall -I$(SFML_INC)

# Linker flags
LDFLAGS = -L$(SFML_LIB) -rpath $(SFML_LIB) -lsfml-network -lsfml-system -lsfml-graphics -lsfml-window -lncurses

# Source files and targets
SRCS = server.cpp user.cpp
OBJS = $(SRCS:.cpp=build/%.o)
TARGETS = build/server build/user

# Default target: build both executables
all: $(TARGETS)

# Rule for building server
build/server: build/server.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# Rule for building user
build/user: build/user.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# Rule for compiling .cpp files to .o files in build/ directory
build/%.o: %.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Create the build directory if it doesn't exist
build:
	mkdir -p build

# Clean up the object files and executables
clean:
	rm -rf build

# Phony targets
.PHONY: all clean
