#!/bin/bash

set -e;

if [ 0 == $# ]; then
    echo "usage: $0 python-version";
    exit 1;
fi

echo "Entering python land..."
cd pythonpkgs/

echo "Checking that we have a python..."
which python$1;

# Create a virtual environment
echo "Creating our virtual environment"
python$1 -m venv myenv
echo "Sourcing the env"
source myenv/bin/activate

cd ducktables;

# Upgrade pip
echo "Upgrading pip..."
pip install --upgrade pip
pip install wheel


python3.9 setup.py sdist bdist_wheel


# Install Twine to interact with PyPi
pip install twine

twine upload dist/*
