#!/bin/sh

if [ -z "$1" ]; then
  echo "Usage: update.sh /path/to/boost/tree"
  echo "Example: update.sh ~/boost_1_33_0"
  exit
fi

find boost libs -type f ! -regex '.*/\(\.\|CVS/\|Makefile\|CMake\|SConscript\).*' \
-exec sh -c "if [ -e '$1/{}' ]; then cp -fp '$1/{}' '{}'; chmod u+w '{}'; else rm -f '{}'; fi" \;
