#!/bin/bash

# Linux
if [[ "$OSTYPE" == "linux-gnu"* ]]; then

    sudo apt update
    sudo apt install libglib2.0-dev

# MAC
elif [[ "$OSTYPE" == "darwin"* ]]; then

    brew install glib

    
# Nenhum dos dois
else
    exit 1
fi

echo "Instalação feita"
