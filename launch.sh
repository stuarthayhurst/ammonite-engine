#!/bin/bash

buildDir="build"

if [[ "$1" == "--loop" ]]; then
  target="$buildDir/threadTest"
  while true; do
    if ! LD_LIBRARY_PATH="$buildDir" "$target" "$@"; then
      exit 1
    fi
  done
fi

if [[ "$1" == "--threads" ]]; then
  target="$buildDir/threadTest"
else
  target="$buildDir/demo"
fi

if [[ ! -f "$target" ]]; then
  echo "$target doesn't exist, did you forget to build the demo?" > /dev/stderr
  exit 1
fi

LD_LIBRARY_PATH="$buildDir" "$target" "$@"
