CXX ?= $(shell command -v g++)
SHELL = bash

LIBS = glm glfw3 glew
BUILD_DIR = build

CXXFLAGS := $(shell pkg-config --cflags $(LIBS))
CXXFLAGS += -Wall -Wextra -O3 -flto -std=c++17
LDFLAGS := $(shell pkg-config --libs $(LIBS)) -pthread

ifeq ($(DEBUG),true)
  CXXFLAGS += -DDEBUG
endif

$(BUILD_DIR)/main:
	mkdir -p "$(BUILD_DIR)"
	$(CXX) src/main.cpp src/*/*.cpp $(CXXFLAGS) $(LDFLAGS) -o $@

.PHONY: build clean

build:
	$(MAKE) "$(BUILD_DIR)/main"
clean:
	rm -rfv "$(BUILD_DIR)"
