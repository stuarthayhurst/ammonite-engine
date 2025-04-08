SHELL = bash -O globstar
TIDY ?= clang-tidy

BUILD_DIR ?= build
CACHE_DIR = cache
PREFIX_DIR ?= /usr/local
INSTALL_DIR ?= $(PREFIX_DIR)/lib
HEADER_DIR ?= $(PREFIX_DIR)/include
PKG_CONF_DIR ?= $(INSTALL_DIR)/pkgconfig
LIBRARY_NAME = libammonite.so.$(shell pkg-config --modversion ammonite.pc)

AMMONITE_OBJECTS_SOURCE = $(shell ls ./src/ammonite/**/*.cpp)
AMMONITE_HEADERS_SOURCE = $(shell ls ./src/ammonite/**/*.hpp)
AMMONITE_INCLUDE_HEADERS_SOURCE += $(shell ls ./src/include/ammonite/**/*.hpp)

HELPER_OBJECTS_SOURCE = $(shell ls ./src/helper/**/*.cpp)
HELPER_HEADERS_SOURCE = $(shell ls ./src/helper/**/*.hpp)

DEMO_OBJECTS_SOURCE = $(shell ls ./src/demos/**/*.cpp)
DEMO_HEADERS_SOURCE = $(shell ls ./src/demos/**/*.hpp)

ROOT_OBJECTS_SOURCE = $(shell ls ./src/*.cpp)

ALL_OBJECTS_SOURCE = $(AMMONITE_OBJECTS_SOURCE) $(HELPER_OBJECTS_SOURCE) $(DEMO_OBJECTS_SOURCE) \
                     $(ROOT_OBJECTS_SOURCE)
ALL_HEADERS_SOURCE = $(AMMONITE_HEADERS_SOURCE) $(HELPER_HEADERS_SOURCE) $(DEMO_HEADERS_SOURCE) \
                     $(AMMONITE_INCLUDE_HEADERS_SOURCE)
LINT_FILES = $(subst ./src,$(OBJECT_DIR),$(subst .cpp,.cpp.linted,$(ALL_OBJECTS_SOURCE)))
LINT_FILES += $(subst ./src,$(OBJECT_DIR),$(subst .hpp,.hpp.linted,$(ALL_HEADERS_SOURCE)))

OBJECT_DIR = $(BUILD_DIR)/objects
AMMONITE_OBJECTS = $(subst ./src,$(OBJECT_DIR),$(subst .cpp,.o,$(AMMONITE_OBJECTS_SOURCE)))
HELPER_OBJECTS = $(subst ./src,$(OBJECT_DIR),$(subst .cpp,.o,$(HELPER_OBJECTS_SOURCE)))
DEMO_OBJECTS = $(subst ./src,$(OBJECT_DIR),$(subst .cpp,.o,$(DEMO_OBJECTS_SOURCE)))

#Global arguments
CXXFLAGS += -Wall -Wextra -Werror -std=c++23 -flto=auto -O3

ifeq ($(FAST),true)
  CXXFLAGS += -march=native -DFAST
endif

ifeq ($(DEBUG),true)
  CXXFLAGS += -DAMMONITE_DEBUG -g -fno-omit-frame-pointer -fsanitize=address,undefined
  CHECK_LEAKS = true
endif

ifeq ($(CHECK_LEAKS),true)
  LDFLAGS += -fsanitize=leak
endif

ifeq ($(CHECK_THREADS),true)
  CXXFLAGS += -fsanitize=thread
endif

ifeq ($(USE_LLVM_CPP),true)
  CXXFLAGS += -stdlib=libc++
  LDFLAGS += -stdlib=libc++
endif

#Sync these with ammonite.pc
REQUIRES_PRIVATE = glm glfw3 glew stb assimp
FLAGS_PRIVATE = -lm -latomic -pthread

#Library arguments
LIBRARY_CXXFLAGS := $(CXXFLAGS) -fpic
LIBRARY_LDFLAGS := $(LDFLAGS) "-Wl,-soname,$(LIBRARY_NAME)" \
                   $(shell pkg-config --libs $(REQUIRES_PRIVATE))

#Client arguments
ifneq ($(USE_SYSTEM),true)
  PROJECT_ROOT = $(dir $(realpath $(firstword $(MAKEFILE_LIST))))
  PKG_CONF_ARGS = "--define-variable=libdir=$(BUILD_DIR)" \
                  "--define-variable=includedir=$(PROJECT_ROOT)src/include" \
                  "--with-path=$(PROJECT_ROOT)"
endif

CLIENT_CXXFLAGS := $(CXXFLAGS) $(shell pkg-config $(PKG_CONF_ARGS) --cflags ammonite)
CLIENT_LDFLAGS := $(LDFLAGS) $(shell pkg-config $(PKG_CONF_ARGS) --libs ammonite)

# --------------------------------
# Client build recipes
# --------------------------------

$(BUILD_DIR)/demo: $(BUILD_DIR)/$(LIBRARY_NAME) $(HELPER_OBJECTS) $(DEMO_OBJECTS) $(OBJECT_DIR)/demoLoader.o
	@mkdir -p "$(BUILD_DIR)"
	$(CXX) -o "$(BUILD_DIR)/demo" $(HELPER_OBJECTS) $(DEMO_OBJECTS) $(OBJECT_DIR)/demoLoader.o $(CLIENT_CXXFLAGS) $(CLIENT_LDFLAGS)
$(BUILD_DIR)/threadTest: $(BUILD_DIR)/$(LIBRARY_NAME) $(OBJECT_DIR)/threadTest.o
	@mkdir -p "$(BUILD_DIR)"
	$(CXX) -o "$(BUILD_DIR)/threadTest" $(OBJECT_DIR)/threadTest.o $(CLIENT_CXXFLAGS) $(CLIENT_LDFLAGS)
$(OBJECT_DIR)/helper/%.o: ./src/helper/%.cpp $(HELPER_HEADERS_SOURCE)
	@mkdir -p "$$(dirname $@)"
	$(CXX) "$<" -c $(CLIENT_CXXFLAGS) -o "$@"
$(OBJECT_DIR)/demos/%.o: ./src/demos/%.cpp $(DEMO_HEADERS_SOURCE) $(AMMONITE_HEADERS_SOURCE) $(AMMONITE_INCLUDE_HEADERS_SOURCE)
	@mkdir -p "$$(dirname $@)"
	$(CXX) "$<" -c $(CLIENT_CXXFLAGS) -o "$@"
$(OBJECT_DIR)/%.o: ./src/%.cpp $(AMMONITE_HEADERS_SOURCE) $(HELPER_HEADERS_SOURCE) $(AMMONITE_INCLUDE_HEADERS_SOURCE)
	@mkdir -p "$(OBJECT_DIR)"
	$(CXX) "$<" -c $(CLIENT_CXXFLAGS) -o "$@"


# --------------------------------
# Library build recipes
# --------------------------------

$(BUILD_DIR)/libammonite.so: $(AMMONITE_OBJECTS)
	@mkdir -p "$(OBJECT_DIR)"
	$(CXX) -shared -o "$@" $(AMMONITE_OBJECTS) $(LIBRARY_CXXFLAGS) $(LIBRARY_LDFLAGS)
	@if [[ "$(DEBUG)" != "true" ]]; then \
	  strip --strip-unneeded "$(BUILD_DIR)/libammonite.so"; \
	fi
$(BUILD_DIR)/$(LIBRARY_NAME): $(BUILD_DIR)/libammonite.so
	@rm -fv "$(BUILD_DIR)/$(LIBRARY_NAME)"
	@ln -sv "libammonite.so" "$(BUILD_DIR)/$(LIBRARY_NAME)"
$(OBJECT_DIR)/ammonite/%.o: ./src/ammonite/%.cpp $(AMMONITE_HEADERS_SOURCE) $(AMMONITE_INCLUDE_HEADERS_SOURCE)
	@mkdir -p "$$(dirname $@)"
	$(CXX) "$<" -c $(LIBRARY_CXXFLAGS) -o "$@"


# --------------------------------
# Shared linting recipes
# --------------------------------

$(BUILD_DIR)/compile_flags.txt: Makefile
	@mkdir -p "$(BUILD_DIR)"
	@rm -fv "$(BUILD_DIR)/compile_flags.txt"
	@for arg in $(LIBRARY_CXXFLAGS) $(CLIENT_CXXFLAGS); do \
		echo $$arg >> "$(BUILD_DIR)/compile_flags.txt"; \
	done
$(OBJECT_DIR)/%.linted: ./src/% $(BUILD_DIR)/compile_flags.txt .clang-tidy $(ALL_HEADERS_SOURCE)
	$(TIDY) --quiet -p "$(BUILD_DIR)" "$<"
	@mkdir -p "$$(dirname $@)"
	@touch "$@"


.PHONY: build demo threads debug library headers install uninstall clean lint cache icons


# --------------------------------
# Client phony recipes
# --------------------------------
build: demo threads
demo: $(BUILD_DIR)/demo
	@if [[ "$(DEBUG)" != "true" ]]; then \
	  strip --strip-unneeded "$(BUILD_DIR)/demo"; \
	fi
threads: $(BUILD_DIR)/threadTest
	@if [[ "$(DEBUG)" != "true" ]]; then \
	  strip --strip-unneeded "$(BUILD_DIR)/threadTest"; \
	fi
debug: clean
	@DEBUG="true" $(MAKE) build


# --------------------------------
# Library phony recipes
# --------------------------------

library: $(BUILD_DIR)/$(LIBRARY_NAME)


# --------------------------------
# Installer phony recipes
# --------------------------------

headers:
	@rm -rf "$(HEADER_DIR)/ammonite"
	@cp -rv "src/include/ammonite" "$(HEADER_DIR)/ammonite"
	@mkdir -p "$(PKG_CONF_DIR)"
	install "ammonite.pc" "$(PKG_CONF_DIR)/ammonite.pc"
install:
	@mkdir -p "$(INSTALL_DIR)/ammonite"
	install "$(BUILD_DIR)/libammonite.so" "$(INSTALL_DIR)/ammonite/$(LIBRARY_NAME)"
	@ln -sfv "$(LIBRARY_NAME)" "$(INSTALL_DIR)/ammonite/libammonite.so"
	ldconfig "$(INSTALL_DIR)/ammonite"
uninstall:
	@rm -fv "$(INSTALL_DIR)/ammonite/libammonite.so"*
	@rm -fv "$(PKG_CONF_DIR)/ammonite.pc"
	@if [[ -d "$(INSTALL_DIR)/ammonite" ]]; then rm -di "$(INSTALL_DIR)/ammonite"; fi
	@if [[ -d "$(HEADER_DIR)/ammonite" ]]; then rm -rf "$(HEADER_DIR)/ammonite"; fi


# --------------------------------
# Utility / support phony recipes
# --------------------------------

clean: cache
	@rm -rfv "$(BUILD_DIR)"
lint: $(LINT_FILES)
cache:
	@rm -rfv "$(CACHE_DIR)"
icons:
	for res in 256 128 64 32; do \
	  inkscape "--export-filename=./assets/icons/icon-$$res.png" -w "$$res" -h "$$res" "./assets/icons/icon.svg" > /dev/null 2>&1; \
	done
	optipng -o7 -strip all --quiet "./assets/icons/icon"*".png"
