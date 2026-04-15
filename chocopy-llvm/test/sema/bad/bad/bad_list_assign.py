# RUN: %chocopy-llvm --run-sema %s 2>&1 | FileCheck %s.err

x:[int] = None
y:[object] = None

x = [1, 2, 3]
y = x
y = [1]
