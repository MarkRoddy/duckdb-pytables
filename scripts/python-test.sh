#!/bin/bash

set -e;


cd pythonpkgs/

if [ ! -d testenv ]; then
    python3.9 -m venv testenv;
    source testenv/bin/activate
    pip install --upgrade pip
    cd ducktables;
    pip install -r requirements.txt;
else
    source testenv/bin/activate
    cd ducktables;
fi

# Run unit tests
python -m unittest discover -s tests/ducktables/

