CXX ?= $(shell command -v g++)
SHELL = bash

LIBS = glm glfw3 glew stb tinyobjloader
BUILD_DIR = build
CACHE_DIR = cache
#These two should always end in ammonite, otherwise filesystem damage is a possibility
INSTALL_DIR ?= /usr/local/lib/ammonite
HEADER_DIR ?= /usr/local/include/ammonite
LIBRARY_NAME = libammonite.so.1

OBJECT_DIR = $(BUILD_DIR)/objects
AMMONITE_OBJECTS_SOURCE = $(wildcard ./src/ammonite/*.cpp) $(wildcard ./src/ammonite/*/*.cpp)
AMMONITE_OBJECTS = $(subst ./src/ammonite,$(OBJECT_DIR),$(subst .cpp,.o,$(AMMONITE_OBJECTS_SOURCE)))
COMMON_OBJECTS_SOURCE = $(wildcard ./src/common/*.cpp)
COMMON_OBJECTS = $(subst ./src/common,$(OBJECT_DIR),$(subst .cpp,.o,$(COMMON_OBJECTS_SOURCE)))
AMMONITE_HEADER_SOURCE = $(wildcard ./src/ammonite/*.hpp) $(wildcard ./src/ammonite/*/*.hpp)
COMMON_HEADER_SOURCE = $(wildcard ./src/common/*.hpp)

CXXFLAGS := $(shell pkg-config --cflags $(LIBS)) -fopenmp
CXXFLAGS += -Wall -Wextra -Werror -O3 -flto -std=c++17
LDFLAGS := $(shell pkg-config --libs $(LIBS)) -pthread

ifeq ($(DEBUG),true)
  CXXFLAGS += -DDEBUG -g
endif

$(BUILD_DIR)/demo: library $(COMMON_OBJECTS) $(OBJECT_DIR)/demo.o
	@mkdir -p "$(BUILD_DIR)"
	$(CXX) -o "$(BUILD_DIR)/demo" $(COMMON_OBJECTS) $(OBJECT_DIR)/demo.o $(CXXFLAGS) "-L$(BUILD_DIR)" -lammonite $(LDFLAGS) $(RPATH)

$(BUILD_DIR)/libammonite.so: $(AMMONITE_OBJECTS)
	@mkdir -p "$(OBJECT_DIR)"
	$(CXX) -shared -o "$@" $(AMMONITE_OBJECTS) $(CXXFLAGS) "-Wl,-soname,$(LIBRARY_NAME)"

$(AMMONITE_OBJECTS): $(AMMONITE_OBJECTS_SOURCE) $(AMMONITE_HEADER_SOURCE)
	@mkdir -p "$$(dirname $@)"
	$(CXX) $(subst $(OBJECT_DIR),src/ammonite,$(subst .o,.cpp,$(@))) -c $(CXXFLAGS) -fpic -o "$@"

$(COMMON_OBJECTS): $(COMMON_OBJECTS_SOURCE) $(COMMON_HEADER_SOURCE)
	@mkdir -p "$(OBJECT_DIR)"
	$(CXX) $(subst $(OBJECT_DIR),src/common,$(subst .o,.cpp,$(@))) -c $(CXXFLAGS) -o "$@"

$(OBJECT_DIR)/demo.o: ./src/demo.cpp $(AMMONITE_HEADER_SOURCE) $(COMMON_HEADER_SOURCE)
	@mkdir -p "$(OBJECT_DIR)"
	$(CXX) ./src/demo.cpp -c $(CXXFLAGS) -o "$@"

.PHONY: build debug system-build library headers install uninstall clean cache icons
build:
	RPATH="-Wl,-rpath=$(BUILD_DIR)" $(MAKE) system-build
debug: clean
	DEBUG="true" $(MAKE) build
system-build:
	$(MAKE) "$(BUILD_DIR)/demo"
	if [[ "$(DEBUG)" != "true" ]]; then \
	  strip --strip-unneeded "$(BUILD_DIR)/libammonite.so" "$(BUILD_DIR)/demo"; \
	fi
library: $(BUILD_DIR)/libammonite.so
	rm -f "$(BUILD_DIR)/$(LIBRARY_NAME)"
	ln -s "libammonite.so" "$(BUILD_DIR)/$(LIBRARY_NAME)"
headers:
	cp -r "./src/ammonite" "$(HEADER_DIR)"
install:
	@mkdir -p "$(INSTALL_DIR)"
	install "$(BUILD_DIR)/libammonite.so" "$(INSTALL_DIR)/$(LIBRARY_NAME)"
	ldconfig "$(INSTALL_DIR)"
uninstall:
	rm -f "$(INSTALL_DIR)/libammonite.so"*
	if [[ -d "$(INSTALL_DIR)" ]]; then rm -di "$(INSTALL_DIR)"; fi
	if [[ -d "$(HEADER_DIR)" ]]; then rm -rf "$(HEADER_DIR)"; fi
clean: cache
	@rm -rfv "$(BUILD_DIR)"
cache:
	@rm -rfv "$(CACHE_DIR)"
icons:
	./scripts/clean-svgs.py
	inkscape "--export-filename=./assets/icons/icon.png" -w "64" -h "64" "./assets/icons/icon.svg" > /dev/null 2>&1
	optipng -o7 -strip all --quiet "./assets/icons/icon"*".png"
