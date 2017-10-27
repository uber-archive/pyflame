# How To Contribute To Pyflame

We welcome patches for Pyflame---some of the most interesting features have come
from users of Pyflame. There are a few guidelines you should follow when
submitting a pull request.

For all pull requests, please make sure the test suite passes before submitting
a pull request. You can run the test suite with `make check`.

For C++ changes:

 * We ask you to stick to the [Google C++ Style
   Guide](http://google.github.io/styleguide/cppguide.html).
 * Run `clang-format` to reformat your code. This tool will automatically format
   the source files to put them into the correct style for Pyflame.

For Python (i.e. test suite) changes:

 * Conform to [PEP-8](https://www.python.org/dev/peps/pep-0008/).
 * Format your code using [yapf](https://github.com/google/yapf).
