CXX := g++
CC := gcc

CXXFLAGS := -Wall -g -pthread -std=c++11
CFLAGS := -Wall -g -pthread

INCLUDE := -I.

EXEC := simulator

CPP_SOURCES := $(wildcard *.cpp)
C_SOURCES := $(wildcard *.c)

CPP_OBJECTS := $(CPP_SOURCES:.cpp=.o)
C_OBJECTS := $(C_SOURCES:.c=.o)
OBJECTS := $(CPP_OBJECTS) $(C_OBJECTS)

all: $(EXEC)

$(EXEC): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(INCLUDE) $^ -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(EXEC)

.PHONY: all clean
