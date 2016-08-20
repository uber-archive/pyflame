import os
import sys
import time


def main():
    sys.stdout.write('%d\n' % (os.getpid(),))
    sys.stdout.flush()
    while True:
        time.sleep(0.1)
        target = time.time() + 0.1
        while time.time() < target:
            pass


if __name__ == '__main__':
    main()
