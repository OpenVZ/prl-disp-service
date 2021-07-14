#!/bin/sh

# This script is used to build prl-disp-service in the Docker container
# Usage:
# ./docker-build.sh debug
# ./docker-build.sh release
# ./docker-build.sh test
# mode is release by default

MODE=${1:-release}

if [[ $MODE != "debug" ]] && [[ $MODE != "release" ]] && [[ $MODE != "test" ]]; then
    echo "Use debug/release/test mode"
    exit 1
fi

if [[ $MODE == "debug" ]] || [[ $MODE == "release" ]]; then
    ./Gen.py
    (cd Libraries/Transponster && qmake-qt4 && make $MODE)
    (cd Dispatcher && qmake-qt4 && make  -j`nproc` $MODE)
fi

if [[ $MODE == "test" ]]; then
    cd Tests
    qmake-qt4
    make
fi
