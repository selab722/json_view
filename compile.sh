#!/bin/bash

cmake -S . -B build

cmake --build build --parallel --config Release

# cmake --install build
