SHELL := bash -O globstar
TIDY ?= clang-tidy

BUILD_DIR ?= build
OBJECT_DIR := $(BUILD_DIR)/objects
CACHE_DIR ?= cache
PREFIX_DIR ?= /usr/local
INSTALL_DIR ?= $(PREFIX_DIR)/lib
HEADER_DIR ?= $(PREFIX_DIR)/include
PKG_CONF_INSTALL_DIR ?= $(INSTALL_DIR)/pkgconfig
LIBRARY_VERSION := $(shell pkg-config --modversion data/ammonite.pc)
LIBRARY_NAME := libammonite.so.$(LIBRARY_VERSION)

AMMONITE_OBJECTS_SOURCE := $(shell ls ./src/ammonite/**/*.cpp)
AMMONITE_HEADERS_SOURCE := $(shell ls ./src/ammonite/**/*.hpp)
AMMONITE_INCLUDE_HEADERS_SOURCE += $(shell ls ./src/include/ammonite/**/*.hpp)

HELPER_OBJECTS_SOURCE := $(shell ls ./src/helper/**/*.cpp)
HELPER_HEADERS_SOURCE := $(shell ls ./src/helper/**/*.hpp)

DEMO_OBJECTS_SOURCE := $(shell ls ./src/demos/**/*.cpp)
DEMO_HEADERS_SOURCE := $(shell ls ./src/demos/**/*.hpp)

TEST_OBJECTS_SOURCE := $(shell ls ./src/tests/**/*.cpp)
TEST_HEADERS_SOURCE := $(shell ls ./src/tests/**/*.hpp)

ROOT_OBJECTS_SOURCE := $(shell ls ./src/*.cpp)

LINT_OBJECTS_SOURCE := $(ROOT_OBJECTS_SOURCE) $(AMMONITE_OBJECTS_SOURCE) \
                       $(HELPER_OBJECTS_SOURCE) $(DEMO_OBJECTS_SOURCE)
LINT_HEADERS_SOURCE := $(AMMONITE_HEADERS_SOURCE) $(AMMONITE_INCLUDE_HEADERS_SOURCE) \
                       $(HELPER_HEADERS_SOURCE) $(DEMO_HEADERS_SOURCE)

DEBUG_LINT_STRING := linted
ifeq ($(DEBUG),true)
  DEBUG_LINT_STRING := debug.linted
endif

LINT_FILES := $(subst ./src,$(OBJECT_DIR),$(subst .hpp,.hpp.$(DEBUG_LINT_STRING),$(LINT_HEADERS_SOURCE)))
LINT_FILES += $(subst ./src,$(OBJECT_DIR),$(subst .cpp,.cpp.$(DEBUG_LINT_STRING),$(LINT_OBJECTS_SOURCE)))
TEST_LINT_FILES := $(subst ./src,$(OBJECT_DIR),$(subst .hpp,.hpp.$(DEBUG_LINT_STRING),$(TEST_HEADERS_SOURCE)))
TEST_LINT_FILES += $(subst ./src,$(OBJECT_DIR),$(subst .cpp,.cpp.$(DEBUG_LINT_STRING),$(TEST_OBJECTS_SOURCE)))

AMMONITE_OBJECTS := $(subst ./src,$(OBJECT_DIR),$(subst .cpp,.o,$(AMMONITE_OBJECTS_SOURCE)))
HELPER_OBJECTS := $(subst ./src,$(OBJECT_DIR),$(subst .cpp,.o,$(HELPER_OBJECTS_SOURCE)))
DEMO_OBJECTS := $(subst ./src,$(OBJECT_DIR),$(subst .cpp,.o,$(DEMO_OBJECTS_SOURCE)))
TEST_OBJECTS := $(subst ./src,$(OBJECT_DIR),$(subst .cpp,.o,$(TEST_OBJECTS_SOURCE)))
ROOT_OBJECTS := $(subst ./src,$(OBJECT_DIR),$(subst .cpp,.o,$(ROOT_OBJECTS_SOURCE)))

#Global arguments
CXXFLAGS += -Wall -Wextra -Werror -Wpedantic -std=c++23
CXXFLAGS += -fno-math-errno -flto=auto

ifndef ARCH
  CXXFLAGS += -march=native
else
  ifneq ($(ARCH),unset)
    CXXFLAGS += -march=$(ARCH)
  endif
endif

ifeq ($(FAST),true)
  CXXFLAGS += -DAMMONITE_FAST
endif

ifeq ($(USE_LLVM_CPP),true)
  CXXFLAGS += -stdlib=libc++
  LDFLAGS += -stdlib=libc++
endif


ifeq ($(DEBUG),true)
  CXXFLAGS += -DAMMONITE_DEBUG -g -Og -fno-omit-frame-pointer

  #Enable ASan and UBSan by default in debug mode if nothing incompatible is enabled
  ifeq (,$(filter true,$(CHECK_THREADS) $(CHECK_TYPES) $(CHECK_MEMORY)))
    ifndef CHECK_ADDRESS
      CHECK_ADDRESS := true
    endif
    ifndef CHECK_UNDEFINED
      CHECK_UNDEFINED := true
    endif
  endif
else
  CXXFLAGS += -O3
endif

ifneq ($(VALGRIND_SAFE),true)
  ifeq ($(CHECK_ADDRESS),true)
    CXXFLAGS += -fsanitize=address
  endif

  ifeq ($(CHECK_UNDEFINED),true)
    CXXFLAGS += -fsanitize=undefined
  endif

  ifeq ($(CHECK_THREADS),true)
    CXXFLAGS += -fsanitize=thread
  endif

  ifeq ($(CHECK_TYPES),true)
    CXXFLAGS += -fsanitize=type
  endif

  ifeq ($(CHECK_MEMORY),true)
    CXXFLAGS += -fsanitize=memory
  endif

  ifeq ($(CHECK_LEAKS),true)
    LDFLAGS += -fsanitize=leak
  endif
endif

#Fetch library dependencies and flags from ammonite.pc
REQUIRES_PRIVATE := $(shell sed -ne 's/^.*Requires.private: //p' data/ammonite.pc)
LDFLAGS_PRIVATE := $(shell sed -ne 's/^.*Libs.private: //p' data/ammonite.pc)
CFLAGS_PRIVATE := $(shell sed -ne 's/^.*Cflags.private: //p' data/ammonite.pc)

#Determine where to look for libtangle
ifneq ($(USE_SYSTEM),true)
  TANGLE_ROOT := $(dir $(realpath $(firstword $(MAKEFILE_LIST))))/libtangle
  TANGLE_PKG_CONF_ARGS := "--define-variable=tanglelibdir=$(TANGLE_ROOT)/build" \
                          "--define-variable=tangleincludedir=$(TANGLE_ROOT)/src/include" \
                          "--with-path=$(TANGLE_ROOT)/data"
  TANGLE_PKG_CONF_FILE := $(TANGLE_ROOT)/data/tangle.pc

  #Substitute tangle for the local tangle.pc file
  REQUIRES_PRIVATE := $(subst tangle,$(TANGLE_PKG_CONF_FILE),$(REQUIRES_PRIVATE))
endif

#Library arguments
LIBRARY_CXXFLAGS := $(CXXFLAGS) -fpic $(CFLAGS_PRIVATE) -DAMMONITE_VERSION=$(LIBRARY_VERSION) \
                    $(shell pkg-config $(TANGLE_PKG_CONF_ARGS) --cflags $(REQUIRES_PRIVATE))
LIBRARY_LDFLAGS := $(LDFLAGS) "-Wl,-soname,$(LIBRARY_NAME)" $(LDFLAGS_PRIVATE) \
                   $(shell pkg-config $(TANGLE_PKG_CONF_ARGS) --libs $(REQUIRES_PRIVATE))

#Client arguments
ifneq ($(USE_SYSTEM),true)
  PROJECT_ROOT := $(dir $(realpath $(firstword $(MAKEFILE_LIST))))
  CLIENT_PKG_CONF_ARGS := "--define-variable=ammonitelibdir=$(BUILD_DIR)" \
                          "--define-variable=ammoniteincludedir=$(PROJECT_ROOT)/src/include" \
                          "--with-path=$(PROJECT_ROOT)/data"
  CLIENT_PKG_CONF_FILE := data/ammonite.pc
else
  CLIENT_PKG_CONF_FILE := ammonite
endif

#Look in libtangle/build for libtangle when linking the client code against libammonite
ifneq ($(USE_SYSTEM),true)
  LOCAL_TANGLE_FLAG := -Wl,-rpath=libtangle/build
endif

#Only evaluate client arguments when used, to avoid errors when building libammonite for system installation
CLIENT_CXXFLAGS = $(CXXFLAGS) $(shell pkg-config $(CLIENT_PKG_CONF_ARGS) $(TANGLE_PKG_CONF_ARGS) --cflags $(CLIENT_PKG_CONF_FILE) $(TANGLE_PKG_CONF_FILE))
CLIENT_LDFLAGS = $(LDFLAGS) $(shell pkg-config $(CLIENT_PKG_CONF_ARGS) $(TANGLE_PKG_CONF_ARGS) --libs $(CLIENT_PKG_CONF_FILE) $(TANGLE_PKG_CONF_FILE)) $(LOCAL_TANGLE_FLAG)

#Recipe-specific client arguments
MATHSTEST_EXTRA_LDFLAGS := -lm
DEMO_EXTRA_LDFLAGS := -lm

#Helper to run the compiler or extract the command
EXTRACT_SCRIPT := python3 extract-command.py
EXTRACT := @function inline() { if [[ "$(DUMMY)" != "true" ]]; then echo "$(CXX) $$@"; $(CXX) $$@; else $(EXTRACT_SCRIPT) "$(BUILD_DIR)" $(CXX) $$@; fi }; inline

# --------------------------------
# Client build recipes
# --------------------------------

$(BUILD_DIR)/demo: $(BUILD_DIR)/$(LIBRARY_NAME) $(HELPER_OBJECTS) $(DEMO_OBJECTS) $(OBJECT_DIR)/demoLoader.o
	@mkdir -p "$(BUILD_DIR)"
	$(CXX) -o "$(BUILD_DIR)/demo" $(HELPER_OBJECTS) $(DEMO_OBJECTS) $(OBJECT_DIR)/demoLoader.o $(CLIENT_CXXFLAGS) $(CLIENT_LDFLAGS) $(DEMO_EXTRA_LDFLAGS)
$(BUILD_DIR)/mathsTest: $(BUILD_DIR)/$(LIBRARY_NAME) $(TEST_OBJECTS) $(OBJECT_DIR)/mathsTest.o
	@mkdir -p "$(BUILD_DIR)"
	$(CXX) -o "$(BUILD_DIR)/mathsTest" $(OBJECT_DIR)/mathsTest.o $(TEST_OBJECTS) $(CLIENT_CXXFLAGS) $(CLIENT_LDFLAGS) $(MATHSTEST_EXTRA_LDFLAGS)

#Recipe dependencies need to be mirrored in the corresponding lint targets
$(OBJECT_DIR)/helper/%.o: ./src/helper/%.cpp $(HELPER_HEADERS_SOURCE) $(AMMONITE_INCLUDE_HEADERS_SOURCE)
	@mkdir -p "$$(dirname $@)"
	$(EXTRACT) "$<" -c $(CLIENT_CXXFLAGS) -o "$@"
$(OBJECT_DIR)/demos/%.o: ./src/demos/%.cpp ./src/demos/%.hpp $(HELPER_HEADERS_SOURCE) $(AMMONITE_INCLUDE_HEADERS_SOURCE)
	@mkdir -p "$$(dirname $@)"
	$(EXTRACT) "$<" -c $(CLIENT_CXXFLAGS) -o "$@"
$(OBJECT_DIR)/tests/%.o: ./src/tests/%.cpp $(TEST_HEADERS_SOURCE) $(AMMONITE_INCLUDE_HEADERS_SOURCE)
	@mkdir -p "$$(dirname $@)"
	$(EXTRACT) "$<" -c $(CLIENT_CXXFLAGS) -o "$@"
$(OBJECT_DIR)/%.o: ./src/%.cpp $(DEMO_HEADERS_SOURCE) $(HELPER_HEADERS_SOURCE) $(AMMONITE_INCLUDE_HEADERS_SOURCE)
	@mkdir -p "$(OBJECT_DIR)"
	$(EXTRACT) "$<" -c $(CLIENT_CXXFLAGS) -o "$@"

# --------------------------------
# Library build recipes
# --------------------------------

$(BUILD_DIR)/libammonite.so: $(AMMONITE_OBJECTS)
	@mkdir -p "$(OBJECT_DIR)"
	$(CXX) -shared -o "$@" $(AMMONITE_OBJECTS) $(LIBRARY_CXXFLAGS) $(LIBRARY_LDFLAGS)
	@if [[ "$(DEBUG)" != "true" ]]; then \
	  strip --strip-unneeded "$@"; \
	fi
$(BUILD_DIR)/$(LIBRARY_NAME): $(BUILD_DIR)/libammonite.so
	@rm -fv "$(BUILD_DIR)/$(LIBRARY_NAME)"
	@ln -sv "libammonite.so" "$(BUILD_DIR)/$(LIBRARY_NAME)"

#Recipe dependencies need to be mirrored in the corresponding lint targets
$(OBJECT_DIR)/ammonite/%.o: ./src/ammonite/%.cpp $(AMMONITE_HEADERS_SOURCE) $(AMMONITE_INCLUDE_HEADERS_SOURCE)
	@mkdir -p "$$(dirname $@)"
	$(EXTRACT) "$<" -c $(LIBRARY_CXXFLAGS) -o "$@"


# --------------------------------
# Shared linting recipes
# --------------------------------

$(BUILD_DIR)/compile_commands.json:
	@DUMMY="true" $(MAKE) $(AMMONITE_OBJECTS) $(HELPER_OBJECTS) $(DEMO_OBJECTS) $(ROOT_OBJECTS) $(TEST_OBJECTS)

#Recipes should only differ in dependencies
#demos/% recipes have been split to give more granular dependencies
#include/% recipes are required, since headers can be linted individually
$(OBJECT_DIR)/include/%.$(DEBUG_LINT_STRING): ./src/include/% .clang-tidy $(AMMONITE_INCLUDE_HEADERS_SOURCE)
	$(TIDY) --quiet -p "$(BUILD_DIR)" "$<"
	@mkdir -p "$$(dirname $@)"
	@touch "$@"
$(OBJECT_DIR)/helper/%.$(DEBUG_LINT_STRING): ./src/helper/% .clang-tidy $(HELPER_HEADERS_SOURCE) $(AMMONITE_INCLUDE_HEADERS_SOURCE)
	$(TIDY) --quiet -p "$(BUILD_DIR)" "$<"
	@mkdir -p "$$(dirname $@)"
	@touch "$@"
$(OBJECT_DIR)/demos/%.cpp.$(DEBUG_LINT_STRING): ./src/demos/%.cpp .clang-tidy ./src/demos/%.hpp $(HELPER_HEADERS_SOURCE) $(AMMONITE_INCLUDE_HEADERS_SOURCE)
	$(TIDY) --quiet -p "$(BUILD_DIR)" "$<"
	@mkdir -p "$$(dirname $@)"
	@touch "$@"
$(OBJECT_DIR)/demos/%.hpp.$(DEBUG_LINT_STRING): ./src/demos/%.hpp .clang-tidy ./src/demos/%.hpp $(AMMONITE_INCLUDE_HEADERS_SOURCE)
	$(TIDY) --quiet -p "$(BUILD_DIR)" "$<"
	@mkdir -p "$$(dirname $@)"
	@touch "$@"
$(OBJECT_DIR)/tests/%.$(DEBUG_LINT_STRING): ./src/tests/% .clang-tidy $(TEST_HEADERS_SOURCE) $(AMMONITE_INCLUDE_HEADERS_SOURCE)
	$(TIDY) --quiet -p "$(BUILD_DIR)" "$<"
	@mkdir -p "$$(dirname $@)"
	@touch "$@"
$(OBJECT_DIR)/%.$(DEBUG_LINT_STRING): ./src/% .clang-tidy $(DEMO_HEADERS_SOURCE) $(HELPER_HEADERS_SOURCE) $(AMMONITE_INCLUDE_HEADERS_SOURCE)
	$(TIDY) --quiet -p "$(BUILD_DIR)" "$<"
	@mkdir -p "$$(dirname $@)"
	@touch "$@"

$(OBJECT_DIR)/ammonite/%.$(DEBUG_LINT_STRING): ./src/ammonite/% .clang-tidy $(AMMONITE_HEADERS_SOURCE) $(AMMONITE_INCLUDE_HEADERS_SOURCE)
	$(TIDY) --quiet -p "$(BUILD_DIR)" "$<"
	@mkdir -p "$$(dirname $@)"
	@touch "$@"


.PHONY: build tests all demo maths debug debug-all library headers install uninstall lint_compile_commands run_lint run_lint_tests lint lint_tests lint_all clean cache icons


# --------------------------------
# Client phony recipes
# --------------------------------

build: demo
tests: maths
all: demo tests
demo: $(BUILD_DIR)/demo
	@if [[ "$(DEBUG)" != "true" ]]; then \
	  strip --strip-unneeded "$<"; \
	fi
maths: $(BUILD_DIR)/mathsTest
	@if [[ "$(DEBUG)" != "true" ]]; then \
	  strip --strip-unneeded "$<"; \
	fi
debug:
	@DEBUG="true" $(MAKE) --no-print-directory build
debug-all:
	@DEBUG="true" $(MAKE) --no-print-directory all


# --------------------------------
# Library phony recipes
# --------------------------------

library: $(BUILD_DIR)/$(LIBRARY_NAME)


# --------------------------------
# Installer phony recipes
# --------------------------------

headers:
	@rm -rf "$(HEADER_DIR)/ammonite"
	@mkdir -p "$(HEADER_DIR)"
	@cp -rv "src/include/ammonite" "$(HEADER_DIR)/ammonite"
	@mkdir -p "$(PKG_CONF_INSTALL_DIR)"
	install "data/ammonite.pc" "$(PKG_CONF_INSTALL_DIR)/ammonite.pc"
	sed -e "s|prefix=/usr/local|prefix=$(PREFIX_DIR)|" "data/ammonite.pc" > "$(PKG_CONF_INSTALL_DIR)/ammonite.pc"
install:
	@mkdir -p "$(INSTALL_DIR)"
	install "$(BUILD_DIR)/libammonite.so" "$(INSTALL_DIR)/$(LIBRARY_NAME)"
	@ln -sfv "$(LIBRARY_NAME)" "$(INSTALL_DIR)/libammonite.so"
	ldconfig "$(INSTALL_DIR)"
uninstall:
	@rm -fv "$(INSTALL_DIR)/libammonite.so"*
	@rm -fv "$(PKG_CONF_INSTALL_DIR)/ammonite.pc"
	@if [[ -d "$(HEADER_DIR)/ammonite" ]]; then rm -rv "$(HEADER_DIR)/ammonite"; fi
	ldconfig


# --------------------------------
# Linting phony recipes
# --------------------------------

lint_compile_commands:
	@$(MAKE) -B --no-print-directory $(BUILD_DIR)/compile_commands.json
run_lint: $(LINT_FILES)
run_lint_tests: $(TEST_LINT_FILES)
lint: lint_compile_commands
	@$(MAKE) --no-print-directory run_lint
lint_tests: lint_compile_commands
	@$(MAKE) --no-print-directory run_lint_tests
lint_all: lint lint_tests

# --------------------------------
# Utility / support phony recipes
# --------------------------------

clean: cache
	@rm -rfv "$(BUILD_DIR)"
cache:
	@rm -rfv "$(CACHE_DIR)"
icons:
	for res in 256 128 64 32; do \
	  inkscape "--export-filename=./assets/icons/icon-$$res.png" -w "$$res" -h "$$res" "./assets/icons/icon.svg" > /dev/null 2>&1; \
	done
	optipng -o7 -strip all --quiet "./assets/icons/icon"*".png"
