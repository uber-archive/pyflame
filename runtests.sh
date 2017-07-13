#!/bin/bash

set -ex

ENVDIR="./test_env"

# Run tests using pip; $1 = python version
run_pip_tests() {
  virtualenv -p "$1" "${ENVDIR}"
  trap 'rm -rf ${ENVDIR}' EXIT

  . "${ENVDIR}/bin/activate"
  pip install --upgrade pip
  pip install pytest

  find tests/ -name '*.pyc' -delete
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

# RPM tests are not allowed to use a virtualenv.
run_rpm_tests() {
  py.test-2 tests/
  py.test-3 tests/
}

if [ $# -eq 0 ]; then
  try_pip_tests python
  try_pip_tests python3
elif [ "$1" = "rpm" ]; then
  run_rpm_tests
else
  for py in "$@"; do
    run_pip_tests "$py"
  done
fi
