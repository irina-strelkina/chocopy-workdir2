# RUN: %chocopy-llvm --run-sema %s 2>&1 | FileCheck %s.err

x:[int] = None

x = [1, 2, 3]
x[True]   # Bad
False[0]  # Bad
[][0]     # Bad
x[True] = x[False] = 1
