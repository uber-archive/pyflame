#!/bin/bash

set -e

ENVDIR="test_env"

if [ -z "$PYTHONVERSION" ]; then
    PYTHONVERSION=python2
fi

if [ $# -eq 1 ]; then
    PYTHONVERSION=$1
fi

trap "rm -rf ${ENVDIR}" EXIT

virtualenv -p "$PYTHONVERSION" "${ENVDIR}"
. "${ENVDIR}/bin/activate"
pip install --upgrade pip
pip install pytest
py.test tests/
exit $?
