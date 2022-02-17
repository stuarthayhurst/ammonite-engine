CXX ?= $(shell command -v g++)
SHELL = bash

LIBS = glm glfw3 glew
BUILD_DIR = build
CACHE_DIR = cache

CXXFLAGS := $(shell pkg-config --cflags $(LIBS))
CXXFLAGS += -Wall -Wextra -O3 -flto -std=c++17
LDFLAGS := $(shell pkg-config --libs $(LIBS)) -pthread

ifeq ($(DEBUG),true)
  CXXFLAGS += -DDEBUG
endif

$(BUILD_DIR)/demo:
	mkdir -p "$(BUILD_DIR)"
	$(CXX) src/demo.cpp src/*/*.cpp $(CXXFLAGS) $(LDFLAGS) -o $@

.PHONY: build clean cache
build:
	$(MAKE) "$(BUILD_DIR)/demo"
clean:
	rm -rfv "$(BUILD_DIR)"
cache:
	rm -rfv "$(CACHE_DIR)"
