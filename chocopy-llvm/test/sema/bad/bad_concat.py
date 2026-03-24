# RUN: %chocopy-llvm --run-sema %s 2>&1 | FileCheck %s.err

"Hello" + ["World"]
1 + [2]
[] + [1]
