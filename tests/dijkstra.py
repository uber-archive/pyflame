import argparse
import logging
import os
import random
import sys
import threading


log = logging.getLogger('dijkstra')


class Graph(object):
    """Representation of a sparse graph."""

    def __init__(self, width, height):
        self.width = width
        self.height = height
        self.initial = None
        self.goal = None
        self.filled = set()

    @classmethod
    def generate(cls, width, height, count):
        graph = cls(width, height)
        for _ in xrange(count):
            while True:
                x, y = graph.random_unfilled()
                if (x, y) not in graph:
                    break
            graph.fill_node(x, y)
            possibilities = []
            for xx in (-1, 0, 1):
                for yy in (-1, 0, 1):
                    possibilities.append((xx, yy))
            added = 0
            random.shuffle(possibilities)
            for px, py in possibilities:
                xx = x + px
                yy = y + py
                if not graph.valid(xx, yy):
                    continue
                if (xx, yy) not in graph:
                    graph.fill_node(xx, yy)
                    added += 1
                    if added == 3:
                        break
                    x = xx
                    y = yy
        graph.initial = graph.random_unfilled()
        while True:
            goal = graph.random_unfilled()
            if goal != graph.initial:
                graph.goal = goal
                break
        return graph

    def random_unfilled(self):
        while True:
            x = random.randint(0, self.width - 1)
            y = random.randint(0, self.height - 1)
            if (x, y) not in self.filled:
                return (x, y)

    def fill_node(self, x, y):
        self.filled.add((x, y))

    def valid(self, x, y):
        if x < 0 or y < 0:
            return False
        if x >= self.width or y >= self.height:
            return False
        return True

    def dist(self, x, y):
        gx, gy = self.goal
        dx = gx - x
        dy = gy - y
        return dx*dx + dy*dy

    def __str__(self):
        return '%s(%d, %d, %s) initial=%s goal=%s' % (
            self.__class__.__name__, self.width, self.height,
            sorted(self.filled), self.initial, self.goal)

    def __contains__(self, elem):
        return elem in self.filled


def dijkstra(graph):
    solution = None
    via = {graph.initial: None}
    candidates = []
    x, y = graph.initial
    for xx in (-1, 0, 1):
        for yy in (-1, 0, 1):
            px = x + xx
            py = y + yy
            point = (px, py)
            if graph.valid(px, py) and point not in graph and point not in via:
                d = graph.dist(px, py)
                candidates.append((d, point))
                via[point] = graph.initial
    while candidates:
        candidates.sort(reverse=True)
        d, point = candidates.pop()
        if d == 0:
            solution = [point]
            while True:
                next_point = via[point]
                solution.append(next_point)
                if next_point == graph.initial:
                    break
                else:
                    point = next_point
            solution.reverse()
            break
        else:
            x, y = point
            for xx in (-1, 0, 1):
                for yy in (-1, 0, 1):
                    px = x + xx
                    py = y + yy
                    new_point = (px, py)
                    if graph.valid(px, py)\
                       and new_point not in graph\
                       and new_point not in via:
                        d = graph.dist(px, py)
                        candidates.append((d, new_point))
                        via[new_point] = point
    return solution


def run():
    """Run Dijkstra's algorithm."""
    graph = Graph.generate(100, 100, 80)
    log.info('initial = %s', graph.initial)
    log.info('goal = %s', graph.goal)
    solution = dijkstra(graph)
    solution_len = 0 if solution is None else len(solution)
    log.info('solution = %s, len = %d', solution, solution_len)


def run_times(quiet, times):
    """Run Dijkstra's algorithm in a loop."""
    if not quiet:
        sys.stdout.write('%d\n' % (os.getpid(),))
        sys.stdout.flush()
    if times <= 0:
        while True:
            run()
    else:
        for _ in xrange(times):
            run()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-q', '--quiet', action='store_true',
                        help='Be quiet')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Be verbose')
    parser.add_argument('-t', '--threads', type=int, default=1,
                        help='Number of threads')
    parser.add_argument('-n', '--num', type=int, default=0,
                        help='Number of iterations')
    args = parser.parse_args()

    logging.basicConfig()
    if args.verbose:
        log.setLevel(logging.DEBUG)

    if args.threads == 1:
        run_times(args.quiet, args.num)
    else:
        threads = []
        for _ in xrange(args.threads):
            t = threading.Thread(target=run_times, args=(args.quiet, args.num))
            t.start()
            threads.append(t)
        for i, t in enumerate(threads):
            log.info('joined thread %d', i)
            t.join()


if __name__ == '__main__':
    main()
