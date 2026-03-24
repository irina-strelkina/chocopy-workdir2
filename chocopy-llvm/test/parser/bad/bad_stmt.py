# RUN: %chocopy-llvm %s 2>&1 | FileCheck %s.err

1 + 2
3 == 4 or (not False && True)
5 + 6
7 << 8
