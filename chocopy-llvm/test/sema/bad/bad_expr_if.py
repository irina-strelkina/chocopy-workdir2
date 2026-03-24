# RUN: %chocopy-llvm --run-sema %s 2>&1 | FileCheck %s.err

x: int = 0
x = "Hello" if 2 > 3 else 3
