#!/bin/bash

set -ex

ENVDIR="./test_env"

if [ $# -eq 1 ]; then
    PYTHONVERSION=$1
elif [ -z "$PYTHONVERSION" ]; then
    PYTHONVERSION=python2
fi

trap "rm -rf ${ENVDIR}" EXIT

virtualenv -p "$PYTHONVERSION" "${ENVDIR}"
. "${ENVDIR}/bin/activate"
pip install --upgrade pip
pip install pytest
py.test tests/
exit $?
