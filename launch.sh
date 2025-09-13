#!/bin/bash

buildDir="build"
NEW_LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$buildDir"

if [[ "$1" == "--loop" || "$1" == "--loops" ]]; then
  if [[ "$2" == "math" || "$2" == "maths" ]]; then
    target="$buildDir/mathsTest"
  else
    target="$buildDir/threadTest"
  fi
  while true; do
    if ! LD_LIBRARY_PATH="$NEW_LD_LIBRARY_PATH" "$target" "$@"; then
      exit 1
    fi
  done
elif [[ "$1" == "--thread" || "$1" == "--threads" ]]; then
  target="$buildDir/threadTest"
elif [[ "$1" == "--math" || "$1" == "--maths" ]]; then
  target="$buildDir/mathsTest"
else
  target="$buildDir/demo"
fi

if [[ ! -f "$target" ]]; then
  echo "$target doesn't exist, did you forget to build the demo?" > /dev/stderr
  exit 1
fi

LD_LIBRARY_PATH="$NEW_LD_LIBRARY_PATH" "$target" "$@"
