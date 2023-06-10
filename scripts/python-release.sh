#!/bin/bash

set -e;

echo "Entering python land..."
cd pythonpkgs/

echo "Checking that we have a python..."
which python3.8

# Create a virtual environment
echo "Creating our virtual environment"
python3.8 -m venv myenv
echo "Sourcing the env"
source myenv/bin/activate

cd ducktables;

# Upgrade pip
echo "Upgrading pip..."
pip install --upgrade pip
pip install wheel


python3.8 setup.py sdist bdist_wheel


# Install Twine to interact with PyPi
pip install twine

twine upload dist/*
