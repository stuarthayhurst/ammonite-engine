CXX ?= $(shell command -v g++)
SHELL = bash

LIBS = glm glfw3 glew stb tinyobjloader
BUILD_DIR = build
CACHE_DIR = cache
INSTALL_DIR = /usr/lib

OBJECT_DIR = $(BUILD_DIR)/objects
AMMONITE_OBJECTS_SOURCE = $(wildcard ./src/ammonite/*.cpp)
AMMONITE_OBJECTS = $(subst ./src/ammonite,$(OBJECT_DIR),$(subst .cpp,.o,$(AMMONITE_OBJECTS_SOURCE)))
COMMON_OBJECTS_SOURCE = $(wildcard ./src/common/*.cpp)
COMMON_OBJECTS = $(subst ./src/common,$(OBJECT_DIR),$(subst .cpp,.o,$(COMMON_OBJECTS_SOURCE)))
AMMONITE_HEADER_SOURCE = $(wildcard ./src/ammonite/*.hpp)
COMMON_HEADER_SOURCE = $(wildcard ./src/common/*.hpp)

CXXFLAGS := $(shell pkg-config --cflags $(LIBS))
CXXFLAGS += -Wall -Wextra -Werror -O3 -flto -std=c++17
LDFLAGS := $(shell pkg-config --libs $(LIBS)) -pthread

ifeq ($(DEBUG),true)
  CXXFLAGS += -DDEBUG
endif

$(BUILD_DIR)/demo: $(BUILD_DIR)/libammonite.so $(COMMON_OBJECTS) $(OBJECT_DIR)/demo.o
	@mkdir -p "$(BUILD_DIR)"
	$(CXX) -o "$(BUILD_DIR)/demo" $(COMMON_OBJECTS) $(OBJECT_DIR)/demo.o $(CXXFLAGS) "-L$(BUILD_DIR)" $(LDFLAGS) -lammonite $(RPATH)

$(BUILD_DIR)/libammonite.so: $(AMMONITE_OBJECTS)
	@mkdir -p "$(OBJECT_DIR)"
	$(CXX) -shared -o "$@" $(AMMONITE_OBJECTS) $(CXXFLAGS)

$(AMMONITE_OBJECTS): $(AMMONITE_OBJECTS_SOURCE) $(AMMONITE_HEADER_SOURCE)
	@mkdir -p "$(OBJECT_DIR)"
	$(CXX) $(subst $(OBJECT_DIR),src/ammonite,$(subst .o,.cpp,$(@))) -c $(CXXFLAGS) -o "$@"

$(COMMON_OBJECTS): $(COMMON_OBJECTS_SOURCE) $(COMMON_HEADER_SOURCE)
	@mkdir -p "$(OBJECT_DIR)"
	$(CXX) $(subst $(OBJECT_DIR),src/common,$(subst .o,.cpp,$(@))) -c $(CXXFLAGS) -o "$@"

$(OBJECT_DIR)/demo.o: ./src/demo.cpp $(AMMONITE_HEADER_SOURCE) $(COMMON_HEADER_SOURCE)
	@mkdir -p "$(OBJECT_DIR)"
	$(CXX) ./src/demo.cpp -c $(CXXFLAGS) -o "$@"

.PHONY: build local-build install uninstall clean cache
build:
	$(MAKE) "$(BUILD_DIR)/demo"
local-build:
	RPATH="-Wl,-rpath=$(BUILD_DIR)" $(MAKE) build
install:
	@mkdir -p "$(INSTALL_DIR)"
	install "$(BUILD_DIR)/libammonite.so" "$(INSTALL_DIR)/libammonite.so"
uninstall:
	rm -rf "$(INSTALL_DIR)/libammonite.so"*
clean: cache
	rm -rfv "$(BUILD_DIR)"
cache:
	rm -rfv "$(CACHE_DIR)"
icons:
	./scripts/clean-svgs.py
	inkscape "--export-filename=./assets/icons/icon.png" -w "64" -h "64" "./assets/icons/icon.svg" > /dev/null 2>&1
	optipng -o7 -strip all --quiet "./assets/icons/icon.png"
