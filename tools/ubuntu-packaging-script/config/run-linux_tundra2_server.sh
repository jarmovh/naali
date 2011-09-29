#!/bin/bash
cd /opt/realxtend-tundra2
export LD_LIBRARY_PATH=.:./modules/core
./Tundra --server --config plugins.xml "$@"
