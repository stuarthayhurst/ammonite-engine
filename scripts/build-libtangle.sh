#!/bin/bash

if ! make -C libtangle clean "-j$(nproc)"; then
  echo "Failed to clean libtangle" 1>&2
fi

if ! make -C libtangle build "-j$(nproc)"; then
  echo "Failed to build libtangle" 1>&2
fi
