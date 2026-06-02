#!/bin/bash

#Run a built object using the locally built library
#Use --threads to run the thread tests
#Use --maths to run the maths tests
#Don't specify either of the above to run the graphical demo

#Use --loop to repeatedly run a target (ignored for the default demo)
#Use --valgrind to run the selected target through valgrind (ignored for loop mode)
#  - Set USE_VALGRIND_PATH to use a specific path to the valgrind binary
#Any unrecognised arguments will be passed to the target

buildDir="build"
NEW_LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$buildDir"

target=""
targetArgs=()

loopRequested="false"
valgrindRequested="false"

#Match arguments, filtering out arguments handled by launch.sh
for arg in "$@"; do
  case "$arg" in
    --threads)
      target="$buildDir/threadTest"
      ;;
    --maths)
      target="$buildDir/mathsTest"
      ;;
    --loop)
      loopRequested="true"
      ;;
    --valgrind)
      valgrindRequested="true"
      ;;
    *)
      targetArgs+=("$arg")
      ;;
  esac
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
