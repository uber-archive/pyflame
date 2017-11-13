#!/bin/bash
#
# Run the Pyflame test suite.
#
# If invoked without arguments, this will make a best effort to run the test
# suite against python2 and python3. If you would like to force the test suite
# to run against a specific version of python, invoke with the python
# interpreter names as arguments. These should be strings suitable for passing
# to virtualenv -p.

set -e

ENVDIR="./.test_env"
trap 'rm -rf ${ENVDIR}' EXIT

VERBOSE=0
export PYMAJORVERSION

while getopts ":hvx" opt; do
  case $opt in
    h)
      echo "Usage: $0 [-h] [-x] python..."
      exit 1
      ;;
    v)
      VERBOSE=1
      ;;
    x)
      set -x
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      ;;
  esac
done

shift "$((OPTIND-1))"

exists() {
  command -v "$1" &>/dev/null
}

pytest() {
  if [ "$VERBOSE" -eq 0 ]; then
    py.test -q "$@"
  else
    py.test -v "$@"
  fi
}

# Run tests using pip; $1 = python version
run_pip_tests() {
  local activated
  if [ -z "${VIRTUAL_ENV}" ]; then
    rm -rf "${ENVDIR}"
    if ! virtualenv -q -p "$1" "${ENVDIR}" &>/dev/null; then
      echo "Error: failed to create virtualenv"
      return 1
    fi

    # shellcheck source=/dev/null
    . "${ENVDIR}/bin/activate"
    activated=1
  else
    echo "Warning: reusing virtualenv"
  fi

  PYMAJORVERSION=$(python -c 'import sys; print(sys.version_info[0])')
  echo "Running test suite against interpreter $("$1" --version 2>&1)"

  find tests/ -name '*.pyc' -delete
  pip install -q pytest
  pytest -v tests/
  if [ "$activated" -eq 1 ]; then
    deactivate
  fi
}

# Make a best effort to run the tests against some Python version.
try_pip_tests() {
  if command -v "$1" &>/dev/null; then
    run_pip_tests "$1"
  else
    echo "skipping $1 tests (no such command)"
  fi
}

# Tests run when building RPMs are not allowed to use virtualenv.
run_rpm_tests() {
  for pytest in py.test-2 py.test-2.7; do
    if exists "$pytest"; then
      PYMAJORVERSION=2 "$pytest" -v tests/
      break
    fi
  done

  for pytest in py.test-3 py.test-3.4; do
    if exists "$pytest"; then
      PYMAJORVERSION=3 "$pytest" -v tests/
      break
    fi
  done
}

if [ $# -eq 0 ]; then
  PYMAJORVERSION=2 try_pip_tests python2
  PYMAJORVERSION=3 try_pip_tests python3
elif [ "$1" = "rpm" ]; then
  run_rpm_tests
else
  for py in "$@"; do
    run_pip_tests "$py"
  done
fi
