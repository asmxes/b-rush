#!/bin/bash

if [ ! -d venv ]; then
    echo "[ ! ]  venv not found, run setup.sh first"
    exit 1
fi

source venv/bin/activate

if [ -z "$VIRTUAL_ENV" ]; then
    echo "[ ! ] failed to activate venv"
    exit 1
fi

python serve.py