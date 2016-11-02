#!/bin/bash

set -ex

ENVDIR="./test_env"

# Run tests using pip; $1 = python version
run_pip_tests() {
    virtualenv -p "$1" "${ENVDIR}"
    trap "rm -rf ${ENVDIR}" EXIT

    . "${ENVDIR}/bin/activate"
    pip install --upgrade pip
    pip install pytest
    py.test tests/

    # clean up the trap
    rm -rf "${ENVDIR}" EXIT
    trap "" EXIT
}

# See if we can run the pip tests with this Python version
try_pip_tests() {
    if which "$1" &>/dev/null; then
        run_pip_tests "$1"
    fi
}

# This runs the tests for building an RPM
run_fedora_tests() {
    py.test-2 tests/
    py.test-3 tests/
}

if [ "$1" = "fedora" ]; then
    # If the first arg is fedora, don't use Pip
    run_fedora_tests
elif [ $# -eq 1 ]; then
    # Run the tests for a particular version of python
    run_pip_tests "$1"
elif [ -n "$PYTHONVERSION" ]; then
    # Run the tests for $PYTHONVERSION
    run_pip_tests "$PYTHONVERSION"
else
    # Try various places where we might find Python
    for py in python{,2,3}; do
        try_pip_tests "$py"
    done
fi
