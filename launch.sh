#!/bin/bash

buildDir="build"

if [[ ! -f "$buildDir/demo" ]]; then
  echo "$buildDir/demo doesn't exist, did you forget to build the demo?" > /dev/stderr
  exit 1
fi

LD_LIBRARY_PATH="$buildDir" $buildDir/demo $@
