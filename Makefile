CXX ?= $(shell command -v g++)
SHELL = bash

LIBS = glm glfw3 glew stb
BUILD_DIR = build
CACHE_DIR = cache

OBJECT_DIR = $(BUILD_DIR)/objects
AMMONITE_OBJECTS_SOURCE = $(wildcard ./src/ammonite/*.cpp)
AMMONITE_OBJECTS = $(subst ./src/ammonite,$(OBJECT_DIR),$(subst .cpp,.o,$(AMMONITE_OBJECTS_SOURCE)))

CXXFLAGS := $(shell pkg-config --cflags $(LIBS))
CXXFLAGS += -Wall -Wextra -O3 -flto -std=c++17
LDFLAGS := $(shell pkg-config --libs $(LIBS)) -pthread

ifeq ($(DEBUG),true)
  CXXFLAGS += -DDEBUG
endif

$(BUILD_DIR)/demo: $(AMMONITE_OBJECTS) $(OBJECT_DIR)/demo.o
	@mkdir -p "$(BUILD_DIR)"
	$(CXX) -o "$(BUILD_DIR)/demo" $(OBJECT_DIR)/*.o src/common/*.cpp $(CXXFLAGS) $(LDFLAGS)

$(AMMONITE_OBJECTS): $(AMMONITE_OBJECTS_SOURCE)
	@mkdir -p "$(OBJECT_DIR)"
	$(CXX) $(subst $(OBJECT_DIR),src/ammonite,$(subst .o,.cpp,$(@))) -c $(CXXFLAGS) -o "$@"

$(OBJECT_DIR)/demo.o: ./src/demo.cpp
	@mkdir -p "$(OBJECT_DIR)"
	$(CXX) ./src/demo.cpp -c $(CXXFLAGS) -o "$@"

.PHONY: build clean cache
build:
	$(MAKE) "$(BUILD_DIR)/demo"
clean:
	rm -rfv "$(BUILD_DIR)"
cache:
	rm -rfv "$(CACHE_DIR)"
