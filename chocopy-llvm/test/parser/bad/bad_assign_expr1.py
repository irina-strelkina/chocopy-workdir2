# RUN: %chocopy-llvm %s 2>&1 | FileCheck %s.err

x = (y = 2)
