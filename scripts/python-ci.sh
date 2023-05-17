#!/bin/bash

set -e;


cd pythonpkgs/

# Create a virtual environment
python -m venv myenv
source myenv/bin/activate

cd ducktables;

# Upgrade pip
pip install --upgrade pip

# Install dependencies
pip install -r requirements.txt

# Run unit tests
python -m unittest discover -s tests

# Build the package
python setup.py sdist

# Deactivate the virtual environment
deactivate

# Clean up
rm -rf myenv
