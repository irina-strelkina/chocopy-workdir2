# RUN: %chocopy-llvm --run-sema %s 2>&1 | FileCheck %s.err

not "Bad"
-True
-None
not []
