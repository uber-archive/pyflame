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
import threading


def do_sleep():
    while True:
        pass


def main():
    sys.stdout.write('%d\n' % (os.getpid(),))
    sys.stdout.flush()
    thread = threading.Thread(target=do_sleep)
    thread.start()
    do_sleep()


if __name__ == '__main__':
    main()
