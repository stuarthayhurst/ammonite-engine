CC=g++
SHELL=bash

LIBS=glm glfw3 glew
BUILD_DIR=build

CFLAGS:=$(shell pkg-config --cflags $(LIBS))
CFLAGS+=-Wall -Wextra -O3 -flto
LDFLAGS:=$(shell pkg-config --libs $(LIBS)) -pthread

$(BUILD_DIR)/main:
	mkdir -p "$(BUILD_DIR)"
	$(CC) main.cpp common/*.cpp $(CFLAGS) $(LDFLAGS) -o $@

.PHONY: build clean

build:
	$(MAKE) "$(BUILD_DIR)/main"
clean:
	rm -rfv "$(BUILD_DIR)"
