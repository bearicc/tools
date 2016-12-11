#!/bin/bash

function loge() {
    echo "[ERROR] $1"
    exit 1
}

function logw() {
    echo "[WARNING] $1"
}

function remove() {
    SRC_PATH=$(readlink -f $1)
    SRC_NAME=$(basename "$SRC_PATH")
    DEST_DIR=$HOME/.Trash/$(date "+%Y-%m-%d")
    DEST_NAME=${SRC_NAME}
    DEST_PATH=${DEST_DIR}/${DEST_NAME}

    if [[ ! -e "$SRC_PATH" ]]; then
        loge "$SRC_PATH not exist!"
    fi

    if [[ ! -d "$DEST_DIR" ]]; then
        install -m 755 -d "$DEST_DIR"
    fi

    if [[ -e "${DEST_PATH}" ]]; then
        # logw "${DEST_PATH} already exist!"
        DEST_NAME=${SRC_NAME}_$(printf "%03d\n" $((`ls ${DEST_DIR}/${SRC_NAME}* 2>/dev/null|tail -1|awk -F_ '{print $2}'`+1)))
        DEST_PATH=${DEST_DIR}/${DEST_NAME}
    fi

    echo "${SRC_PATH} --> ${DEST_PATH}"
    mv "${SRC_PATH}" "${DEST_PATH}"
}

if [[ $# -lt 1 ]]; then
    echo "Usage: rm (file|dir)"
    exit 1
fi

for i in $@; do
    remove "$i"
done
