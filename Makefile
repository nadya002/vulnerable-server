CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -I.
TARGET = server
SRC_DIR = src
LIB_DIR = lib

SOURCES = $(SRC_DIR)/server.cpp $(SRC_DIR)/http_handler.cpp $(LIB_DIR)/shop.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(LIB_DIR)/%.o: $(LIB_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJECTS)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
