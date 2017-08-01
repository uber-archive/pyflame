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


def spawn(count):
    print(os.getpid())
    # we are the child, do some stuff
    t0 = time.time()
    x = 0
    while time.time() < t0 + 0.1:
        x += 1

    # spawn a new process
    if count:
        pid = os.fork()
        if pid == 0:
            spawn(count - 1)
        else:
            os.waitpid(pid, 0)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-c', '--count', type=int, default=5, help='How many times to fork')
    args = parser.parse_args()

    sys.stdout.write(str(os.getpid()) + '\n')
    sys.stdout.flush()

    pid = os.fork()
    if pid == 0:
        spawn(args.count)
    else:
        os.waitpid(pid, 0)


if __name__ == '__main__':
    main()
