#!/bin/bash
gconftool-2 --set /desktop/gnome/url-handlers/tundra/command '/opt/realxtend-tundra/run-viewer.sh' --type String
gconftool-2 --set /desktop/gnome/url-handlers/tundra/enabled --type Boolean true

cd /opt/realxtend-tundra2
export LD_LIBRARY_PATH=.:./modules/core
./Tundra --config viewer.xml "$@"
