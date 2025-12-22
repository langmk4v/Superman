#!/bin/sh

find ./include ./src -type f -print0 \
  | xargs -0 wc -l \
  | awk '{sum += $1} END {print sum}'