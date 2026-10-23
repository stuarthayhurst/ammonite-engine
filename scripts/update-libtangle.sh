#!/bin/bash

if ! ./scripts/update-submodules.sh; then
  echo "Failed to update submodules" 1>&2
fi

if ! ./scripts/build-libtangle.sh; then
  echo "Failed to rebuild libtangle" 1>&2
fi
