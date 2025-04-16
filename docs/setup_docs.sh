#!/bin/bash

# Setup script for Lotus documentation

# Exit on error
set -e

# Check for python3
if command -v python3 &> /dev/null; then
    PYTHON=python3
elif command -v python &> /dev/null; then
    PYTHON=python
else
    echo "Error: Neither python3 nor python is installed or in PATH"
    exit 1
fi

# Create and activate a virtual environment
echo "Creating Python virtual environment..."
$PYTHON -m venv .venv
source .venv/bin/activate

# Verify activation worked
if [ -z "$VIRTUAL_ENV" ]; then
    echo "Error: Virtual environment activation failed"
    exit 1
fi

# Install Sphinx and theme
echo "Installing Sphinx and dependencies..."
python -m pip install -r requirements.txt

# Create necessary directories for Sphinx
echo "Setting up documentation directories..."
mkdir -p source/_static
mkdir -p source/_templates
mkdir -p build

# Copy logo if needed 
if [ -f logo.jpg ]; then
    echo "Copying logo file..."
    mkdir -p source/_static
    cp -f logo.jpg source/_static/ || echo "Warning: Could not copy logo.jpg to source/_static/"
fi

# Clean build directory first
echo "Cleaning build directory..."
if [ -f Makefile ]; then
    make clean || echo "Warning: make clean failed, continuing anyway"
fi

# Build HTML documentation
echo "Building HTML documentation..."
if [ -f Makefile ]; then
    make html || echo "Warning: Documentation build failed"
else
    echo "Error: Makefile not found"
fi

# Deactivate virtual environment
echo "Deactivating virtual environment..."
deactivate || true  # Don't fail if deactivate doesn't exist

echo "Documentation setup complete!"
echo "To view the documentation, open build/html/index.html in your browser."
echo "To reactivate the environment later, run: source .venv/bin/activate" 