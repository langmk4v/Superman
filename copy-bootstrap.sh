#!/bin/bash

# copy source files:
# ~/sources/fire/FireReference/src/* --> ./bootstrap/

rm -rf ./bootstrap
mkdir ./bootstrap
cp -r ~/sources/fire/FireReference/src/* ./bootstrap/
