#!/bin/bash

# set -e;

if [ 1 != $# ]; then
    echo "usage: $0 python-version";
    exit 1;
fi

python${PYTHON_VERSION} /tmp/curlbash.py

# Check if your command was successful
if [ $? -eq 0 ]; then
    echo "curlbash did not detect that libpython was missing as expected"
    exit 1
fi

