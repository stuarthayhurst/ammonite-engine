#!/bin/bash

buildDir="build"
NEW_LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$buildDir"

target=""
targetArgs=()

loopRequested="false"
valgrindRequested="false"

#Process arguments, filtering out arguments handled by launch.sh
for arg in "$@"; do
  #Pick a target binary
  if [[ "$arg" == "--threads" ]]; then
    target="$buildDir/threadTest"
    continue
  elif [[ "$arg" == "--maths" ]]; then
    target="$buildDir/mathsTest"
    continue
  fi

  #Enable looping the target
  if [[ "$arg" == "--loop" ]]; then
    loopRequested="true"
    continue
  fi

  #Run the target with valgrind
  if [[ "$arg" == "--valgrind" ]]; then
    valgrindRequested="true"
    continue
  fi

  targetArgs+=("$arg")
done

#Default to running the demo, disable loop mode if triggered
if [[ "$target" == "" ]]; then
  target="$buildDir/demo"
  loopRequested="false"
fi

#Check that the selected target exists
if [[ ! -f "$target" ]]; then
  echo "'$target' doesn't exist, did you forget to build it?" > /dev/stderr
  exit 1
fi

#Pick a path to valgrind, if requested
USE_VALGRIND_PATH=""
if [[ "$valgrindRequested" == "true" ]]; then
  if [[ "$VALGRIND" != "" ]]; then
    USE_VALGRIND_PATH="$VALGRIND"
  else
    USE_VALGRIND_PATH="valgrind"
  fi
fi

#Run the target in a loop or just once
if [[ "$loopRequested" == "true" ]]; then
  while true; do
    if ! LD_LIBRARY_PATH="$NEW_LD_LIBRARY_PATH" "$target" "${targetArgs[@]}"; then
      exit 1
    fi
  done
else
  #Use valgrind if configured to
  if [[ "$USE_VALGRIND_PATH" == "" ]]; then
    LD_LIBRARY_PATH="$NEW_LD_LIBRARY_PATH" "$target" "${targetArgs[@]}"
  else
    LD_LIBRARY_PATH="$NEW_LD_LIBRARY_PATH" "$USE_VALGRIND_PATH" --leak-check=full "$target" "${targetArgs[@]}"
  fi
fi
