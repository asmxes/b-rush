#!/bin/bash

if [ ! -f requirements.txt ]; then
    echo "[ ! ] requirements.txt not found"
    echo "Make sure required python libs are saved in requirements.txt, then run the script again"
    exit 1
fi

python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
