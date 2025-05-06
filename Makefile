CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O3 -ffast-math -march=native -flto -funroll-loops -fno-exceptions -fno-rtti -MMD -MP -DNDEBUG 
SRC_DIR := src/core
TARGET := chessengine.out
SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp, $(SRC_DIR)/%.o, $(SOURCES))
DEPENDS := $(OBJECTS:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPENDS)

.PHONY: clean
clean:
	rm -rf $(OBJECTS) $(DEPENDS) $(TARGET)
