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

import os
import sys
import time
import threading


def do_sleep():
    while True:
        time.sleep(0.1)
        target = time.time() + 0.1
        while time.time() < target:
            pass


def sleep_a():
    do_sleep()


def sleep_b():
    do_sleep()


def main():
    sys.stdout.write('%d\n' % (os.getpid(), ))
    sys.stdout.flush()
    thread_a = threading.Thread(target=sleep_a)
    thread_a.start()
    thread_b = threading.Thread(target=sleep_b)
    thread_b.start()


if __name__ == '__main__':
    main()
