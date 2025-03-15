CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g
LDFLAGS = $(shell pkg-config --cflags --libs gtk+-3.0)

# Source files
SRC = tetris.cpp
OBJ = $(SRC:.cpp=.o)
EXEC = tetris

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.cpp tetris.h
	$(CXX) -c $< -o $@ $(CXXFLAGS) $(LDFLAGS)

clean:
	rm -f $(OBJ) $(EXEC)

.PHONY: all clean
