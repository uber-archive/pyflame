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
pip --no-cache-dir install --upgrade pip
pip --no-cache-dir install pytest
py.test tests/
exit $?
