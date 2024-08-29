SHELL = bash -O globstar

LIBS = glm glfw3 glew stb assimp
BUILD_DIR = build
CACHE_DIR = cache
INSTALL_DIR ?= /usr/local/lib
HEADER_DIR ?= /usr/local/include
LIBRARY_NAME = libammonite.so.1

AMMONITE_OBJECTS_SOURCE = $(shell ls ./src/ammonite/**/*.cpp)
AMMONITE_HEADER_SOURCE = $(shell ls ./src/ammonite/**/*.hpp)
AMMONITE_HEADER_INSTALL := $(subst src/ammonite,$(HEADER_DIR)/ammonite,$(AMMONITE_HEADER_SOURCE))

HELPER_OBJECTS_SOURCE = $(shell ls ./src/helper/**/*.cpp)
HELPER_HEADER_SOURCE = $(shell ls ./src/helper/**/*.hpp)

DEMO_OBJECTS_SOURCE = $(shell ls ./src/demos/**/*.cpp)
DEMO_HEADER_SOURCE = $(shell ls ./src/demos/**/*.hpp)

ALL_OBJECTS_SOURCE = $(AMMONITE_OBJECTS_SOURCE) $(HELPER_OBJECTS_SOURCE) $(DEMO_OBJECTS_SOURCE)

OBJECT_DIR = $(BUILD_DIR)/objects
AMMONITE_OBJECTS = $(subst ./src,$(OBJECT_DIR),$(subst .cpp,.o,$(AMMONITE_OBJECTS_SOURCE)))
HELPER_OBJECTS = $(subst ./src,$(OBJECT_DIR),$(subst .cpp,.o,$(HELPER_OBJECTS_SOURCE)))
DEMO_OBJECTS = $(subst ./src,$(OBJECT_DIR),$(subst .cpp,.o,$(DEMO_OBJECTS_SOURCE)))

CXXFLAGS += $(shell pkg-config --cflags $(LIBS))
CXXFLAGS += -Wall -Wextra -Werror -std=c++23 -flto=auto
LDFLAGS := $(shell pkg-config --libs $(LIBS)) -lstdc++ -lm -latomic -pthread

ifeq ($(FAST),true)
  CXXFLAGS += -march=native -DFAST
else
  CXXFLAGS += -O3
endif

ifeq ($(DEBUG),true)
  CXXFLAGS += -DDEBUG -g
endif

$(BUILD_DIR)/demo: $(BUILD_DIR)/$(LIBRARY_NAME) $(HELPER_OBJECTS) $(DEMO_OBJECTS) $(OBJECT_DIR)/demo.o
	@mkdir -p "$(BUILD_DIR)"
	$(CXX) -o "$(BUILD_DIR)/demo" $(HELPER_OBJECTS) $(DEMO_OBJECTS) $(OBJECT_DIR)/demo.o $(CXXFLAGS) "-L$(BUILD_DIR)" -lammonite $(LDFLAGS)

$(BUILD_DIR)/threadDemo: $(BUILD_DIR)/$(LIBRARY_NAME) $(OBJECT_DIR)/threadDemo.o
	@mkdir -p "$(BUILD_DIR)"
	$(CXX) -o "$(BUILD_DIR)/threadDemo" $(OBJECT_DIR)/threadDemo.o $(CXXFLAGS) "-L$(BUILD_DIR)" -lammonite $(LDFLAGS)

$(BUILD_DIR)/libammonite.so: $(AMMONITE_OBJECTS)
	@mkdir -p "$(OBJECT_DIR)"
	$(CXX) -shared -o "$@" $(AMMONITE_OBJECTS) $(CXXFLAGS) "-Wl,-soname,$(LIBRARY_NAME)"
	@if [[ "$(DEBUG)" != "true" ]]; then \
	  strip --strip-unneeded "$(BUILD_DIR)/libammonite.so"; \
	fi

$(BUILD_DIR)/$(LIBRARY_NAME): $(BUILD_DIR)/libammonite.so
	@rm -fv "$(BUILD_DIR)/$(LIBRARY_NAME)"
	@ln -sv "libammonite.so" "$(BUILD_DIR)/$(LIBRARY_NAME)"

$(OBJECT_DIR)/ammonite/%.o: ./src/ammonite/%.cpp $(AMMONITE_HEADER_SOURCE)
	@mkdir -p "$$(dirname $@)"
	$(CXX) $(subst $(OBJECT_DIR),./src,$(subst .o,.cpp,$(@))) -c $(CXXFLAGS) -fpic -o "$@"

$(OBJECT_DIR)/helper/%.o: ./src/helper/%.cpp $(HELPER_HEADER_SOURCE)
	@mkdir -p "$$(dirname $@)"
	$(CXX) $(subst $(OBJECT_DIR),./src,$(subst .o,.cpp,$(@))) -c $(CXXFLAGS) -o "$@"

$(OBJECT_DIR)/demos/%.o: ./src/demos/%.cpp $(DEMO_HEADER_SOURCE) $(AMMONITE_HEADER_SOURCE)
	@mkdir -p "$$(dirname $@)"
	$(CXX) $(subst $(OBJECT_DIR),./src,$(subst .o,.cpp,$(@))) -c $(CXXFLAGS) -o "$@"

$(OBJECT_DIR)/demo.o: ./src/demo.cpp $(AMMONITE_HEADER_SOURCE) $(HELPER_HEADER_SOURCE)
	@mkdir -p "$(OBJECT_DIR)"
	$(CXX) ./src/demo.cpp -c $(CXXFLAGS) -o "$@"
$(OBJECT_DIR)/threadDemo.o: ./src/threadDemo.cpp $(AMMONITE_HEADER_SOURCE)
	@mkdir -p "$(OBJECT_DIR)"
	$(CXX) ./src/threadDemo.cpp -c $(CXXFLAGS) -o "$@"

$(BUILD_DIR)/compile_flags.txt: $(ALL_OBJECTS_SOURCE)
	@mkdir -p "$(BUILD_DIR)"
	@rm -fv "$(BUILD_DIR)/compile_flags.txt"
	@for arg in $(CXXFLAGS); do \
		echo $$arg >> "$(BUILD_DIR)/compile_flags.txt"; \
	done

.PHONY: build demo threads debug library headers install uninstall clean lint cache icons $(AMMONITE_HEADER_INSTALL)
build: demo threads
demo: $(BUILD_DIR)/demo
	@if [[ "$(DEBUG)" != "true" ]]; then \
	  strip --strip-unneeded "$(BUILD_DIR)/demo"; \
	fi
threads: $(BUILD_DIR)/threadDemo
	@if [[ "$(DEBUG)" != "true" ]]; then \
	  strip --strip-unneeded "$(BUILD_DIR)/threadDemo"; \
	fi
debug: clean
	@DEBUG="true" $(MAKE) build
library: $(BUILD_DIR)/$(LIBRARY_NAME)
headers: $(AMMONITE_HEADER_INSTALL)
$(AMMONITE_HEADER_INSTALL):
	@targetFile="/$@"; \
	targetDir="$$(dirname $$targetFile)"; \
	origFile="$${targetFile//'$(HEADER_DIR)'/src}"; \
	if [[ "$$targetDir/" != */internal/* ]] && \
	   [[ "$$targetDir/" != */core/* ]]; then \
	  mkdir -p "$$targetDir" && \
	  cp -v "$$origFile" "$$targetFile"; \
	fi
install:
	@mkdir -p "$(INSTALL_DIR)/ammonite"
	install "$(BUILD_DIR)/libammonite.so" "$(INSTALL_DIR)/ammonite/$(LIBRARY_NAME)"
	ldconfig "$(INSTALL_DIR)/ammonite"
uninstall:
	@rm -fv "$(INSTALL_DIR)/ammonite/libammonite.so"*
	@if [[ -d "$(INSTALL_DIR)/ammonite" ]]; then rm -di "$(INSTALL_DIR)/ammonite"; fi
	@if [[ -d "$(HEADER_DIR)/ammonite" ]]; then rm -rf "$(HEADER_DIR)/ammonite"; fi
clean: cache
	@rm -rfv "$(BUILD_DIR)"
lint: $(BUILD_DIR)/compile_flags.txt
	clang-tidy -p "$(BUILD_DIR)" $(ALL_OBJECTS_SOURCE)
cache:
	@rm -rfv "$(CACHE_DIR)"
icons:
	for res in 256 128 64 32; do \
	  inkscape "--export-filename=./assets/icons/icon-$$res.png" -w "$$res" -h "$$res" "./assets/icons/icon.svg" > /dev/null 2>&1; \
	done
	optipng -o7 -strip all --quiet "./assets/icons/icon"*".png"
