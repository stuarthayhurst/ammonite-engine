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

HELPER_OBJECTS_SOURCE = $(wildcard ./src/helper/*.cpp)
HELPER_HEADER_SOURCE = $(wildcard ./src/helper/*.hpp)

DEMO_OBJECTS_SOURCE = $(wildcard ./src/demos/*.cpp)
DEMO_HEADER_SOURCE = $(wildcard ./src/demos/*.hpp)

OBJECT_DIR = $(BUILD_DIR)/objects
AMMONITE_OBJECTS = $(subst ./src,$(OBJECT_DIR),$(subst .cpp,.o,$(AMMONITE_OBJECTS_SOURCE)))
HELPER_OBJECTS = $(subst ./src,$(OBJECT_DIR),$(subst .cpp,.o,$(HELPER_OBJECTS_SOURCE)))
DEMO_OBJECTS = $(subst ./src,$(OBJECT_DIR),$(subst .cpp,.o,$(DEMO_OBJECTS_SOURCE)))

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

$(BUILD_DIR)/demo: $(BUILD_DIR)/$(LIBRARY_NAME) $(HELPER_OBJECTS) $(DEMO_OBJECTS) $(OBJECT_DIR)/demo.o
	@mkdir -p "$(BUILD_DIR)"
	$(CXX) -o "$(BUILD_DIR)/demo" $(HELPER_OBJECTS) $(DEMO_OBJECTS) $(OBJECT_DIR)/demo.o $(CXXFLAGS) "-L$(BUILD_DIR)" -lammonite $(LDFLAGS)

$(BUILD_DIR)/libammonite.so: $(AMMONITE_OBJECTS)
	@mkdir -p "$(OBJECT_DIR)"
	$(CXX) -shared -o "$@" $(AMMONITE_OBJECTS) $(CXXFLAGS) "-Wl,-soname,$(LIBRARY_NAME)"

$(BUILD_DIR)/$(LIBRARY_NAME): $(BUILD_DIR)/libammonite.so
	rm -f "$(BUILD_DIR)/$(LIBRARY_NAME)"
	ln -s "libammonite.so" "$(BUILD_DIR)/$(LIBRARY_NAME)"

$(OBJECT_DIR)/ammonite/%.o: ./src/ammonite/%.cpp $(AMMONITE_HEADER_SOURCE)
	@mkdir -p "$$(dirname $@)"
	$(CXX) $(subst $(OBJECT_DIR),./src,$(subst .o,.cpp,$(@))) -c $(CXXFLAGS) -fpic -o "$@"

$(OBJECT_DIR)/helper/%.o: ./src/helper/%.cpp $(HELPER_HEADER_SOURCE)
	@mkdir -p "$$(dirname $@)"
	$(CXX) $(subst $(OBJECT_DIR),./src,$(subst .o,.cpp,$(@))) -c $(CXXFLAGS) -o "$@"

$(OBJECT_DIR)/demos/%.o: ./src/demos/%.cpp $(DEMO_HEADER_SOURCE)
	@mkdir -p "$$(dirname $@)"
	$(CXX) $(subst $(OBJECT_DIR),./src,$(subst .o,.cpp,$(@))) -c $(CXXFLAGS) -o "$@"

$(OBJECT_DIR)/demo.o: ./src/demo.cpp $(AMMONITE_HEADER_SOURCE) $(HELPER_HEADER_SOURCE)
	@mkdir -p "$(OBJECT_DIR)"
	$(CXX) ./src/demo.cpp -c $(CXXFLAGS) -o "$@"

.PHONY: build debug library headers install uninstall clean cache icons
build:
	@$(MAKE) "$(BUILD_DIR)/demo"
	@if [[ "$(DEBUG)" != "true" ]]; then \
	  strip --strip-unneeded "$(BUILD_DIR)/libammonite.so" "$(BUILD_DIR)/demo"; \
	fi
debug: clean
	@DEBUG="true" $(MAKE) build
library: $(BUILD_DIR)/$(LIBRARY_NAME)
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
