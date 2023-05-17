#!/bin/bash

set -e;

echo "Entering python land..."
cd pythonpkgs/

echo "Checking that we have a python..."
which python

# Create a virtual environment
echo "Creating our virtual environment"
python -m venv myenv
echo "Sourcing the env"
source myenv/bin/activate

cd ducktables;

# Upgrade pip
echo "Upgrading pip..."
pip install --upgrade pip

# Install dependencies
echo "Installing dependencies"
pip install -r requirements.txt

# Run unit tests
echo "Running tests..."
python -m unittest discover -s tests

# Build the package
echo "Creating package..."
python setup.py sdist

# Deactivate the virtual environment
deactivate

# Clean up
rm -rf myenv
