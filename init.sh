#!/bin/bash

set -e

echo "🧹 Cleaning..."
make clean

echo "🔨 Building..."
make

echo "🚀 Running..."
./scratch_clone
