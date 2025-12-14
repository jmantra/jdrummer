
# MiniBPM

A simple beats-per-minute estimator for fixed-tempo music audio,
implemented in C++ with few external dependencies.

Written by Chris Cannam, chris.cannam@breakfastquay.com.
Published by Particular Programs Ltd t/a Breakfast Quay.
Copyright 2007-2025 Particular Programs Ltd.

* About MiniBPM: https://breakfastquay.com/minibpm/
* Code repository: https://hg.sr.ht/~breakfastquay/minibpm


## Licence

MiniBPM is distributed under the GNU General Public License. See the
file COPYING for more information.

If you wish to distribute code using MiniBPM under terms other than
those of the GNU General Public License, you must obtain a commercial
licence from us before doing so. In particular, you may not legally
distribute through any Apple App Store unless you have a commercial
licence.  See http://breakfastquay.com/minibpm/ for licence terms.

If you have obtained a valid commercial licence, your licence
supersedes this README and the enclosed COPYING file and you may
redistribute and/or modify MiniBPM under the terms described in that
licence. Please refer to your licence agreement for more details.


## Building MiniBPM

Just add `MiniBpm.h` and `MiniBpm.cpp` (found in the `src/` directory
here) to your code project. The code is C++98 standard. MiniBPM uses
some classes from the C++ standard library, but has no other
dependencies.

A suite of unit tests is provided using the Boost test
framework. (Note that these test the implementation of the method --
they do not give you any indication of how good the method itself is.)
To run them, run `make` or equivalent in the `tests/` directory.

