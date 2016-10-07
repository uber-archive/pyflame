#!/bin/sh
sudo apt-get install autotools-dev g++ pkg-config

if [ "$PYTHONVERSION" = "python3" ]; then
    sudo apt-get install python3-dev
else
    sudo apt-get install python-dev
fi
