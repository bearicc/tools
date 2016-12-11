#!/bin/bash

DIR=$(date '+%Y-%m-%d')

if [[ ! -d "$HOME/.Trash/${DIR}" ]]; then
    mkdir "$HOME/.Trash/${DIR}"
fi
