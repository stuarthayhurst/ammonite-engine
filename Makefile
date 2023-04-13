CXX ?= $(shell command -v g++)
SHELL = bash

LIBS = glm glfw3 glew stb assimp
BUILD_DIR = build
CACHE_DIR = cache
INSTALL_DIR ?= /usr/local/lib
HEADER_DIR ?= /usr/local/include
LIBRARY_NAME = libammonite.so.1

AMMONITE_OBJECTS_SOURCE = $(wildcard ./src/ammonite/*.cpp) \
			  $(wildcard ./src/ammonite/*/*.cpp) \
			  $(wildcard ./src/ammonite/*/*/*.cpp)
AMMONITE_HEADER_SOURCE = $(wildcard ./src/ammonite/*.hpp) \
			 $(wildcard ./src/ammonite/*/*.hpp) \
			 $(wildcard ./src/ammonite/*/*/*.hpp)

COMMON_OBJECTS_SOURCE = $(wildcard ./src/common/*.cpp)
COMMON_HEADER_SOURCE = $(wildcard ./src/common/*.hpp)

OBJECT_DIR = $(BUILD_DIR)/objects
AMMONITE_OBJECTS = $(subst ./src/ammonite,$(OBJECT_DIR),$(subst .cpp,.o,$(AMMONITE_OBJECTS_SOURCE)))
COMMON_OBJECTS = $(subst ./src/common,$(OBJECT_DIR),$(subst .cpp,.o,$(COMMON_OBJECTS_SOURCE)))

CXXFLAGS := $(shell pkg-config --cflags $(LIBS)) -fopenmp
CXXFLAGS += -Wall -Wextra -Werror -std=c++20 -flto=auto
LDFLAGS := $(shell pkg-config --libs $(LIBS)) -lstdc++ -pthread

ifeq ($(FAST),true)
  CXXFLAGS += -Ofast -march=native
else
  CXXFLAGS += -O3
endif

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
	@if [[ "$(DEBUG)" != "true" ]]; then \
	  strip --strip-unneeded "$(BUILD_DIR)/libammonite.so" "$(BUILD_DIR)/demo"; \
	fi
library: $(BUILD_DIR)/libammonite.so
	rm -f "$(BUILD_DIR)/$(LIBRARY_NAME)"
	ln -s "libammonite.so" "$(BUILD_DIR)/$(LIBRARY_NAME)"
headers:
	cp -r "./src/ammonite" "$(HEADER_DIR)/ammonite"
	rm -rf "$(HEADER_DIR)/ammonite/internal"
install:
	@mkdir -p "$(INSTALL_DIR)/ammonite"
	install "$(BUILD_DIR)/libammonite.so" "$(INSTALL_DIR)/ammonite/$(LIBRARY_NAME)"
	ldconfig "$(INSTALL_DIR)/ammonite"
uninstall:
	rm -f "$(INSTALL_DIR)/ammonite/libammonite.so"*
	if [[ -d "$(INSTALL_DIR)/ammonite" ]]; then rm -di "$(INSTALL_DIR)/ammonite"; fi
	if [[ -d "$(HEADER_DIR)/ammonite" ]]; then rm -rf "$(HEADER_DIR)/ammonite"; fi
clean: cache
	@rm -rfv "$(BUILD_DIR)"
cache:
	@rm -rfv "$(CACHE_DIR)"
icons:
	./scripts/clean-svgs.py
	inkscape "--export-filename=./assets/icons/icon.png" -w "64" -h "64" "./assets/icons/icon.svg" > /dev/null 2>&1
	optipng -o7 -strip all --quiet "./assets/icons/icon"*".png"
