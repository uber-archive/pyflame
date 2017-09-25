# Copyright 2016 Uber Technologies, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import argparse
import os
import sys
import time


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-t', '--time', default=0, type=int, help='How long to run for')
    parser.add_argument(
        '-f', '--fork', action='store_true', help='Fork child processes')
    args = parser.parse_args()

    if not args.fork:
        sys.stdout.write('%d\n' % (os.getpid(), ))
        sys.stdout.flush()
    t0 = time.time()
    while True:
        time.sleep(0.1)
        target = time.time() + 0.1
        while time.time() < target:
            pass
        if args.fork:
            pid = os.fork()
            if pid == 0:
                sys.exit(0)
            else:
                os.waitpid(pid, 0)
        if args.time and time.time() - t0 >= args.time:
            break


if __name__ == '__main__':
    main()
