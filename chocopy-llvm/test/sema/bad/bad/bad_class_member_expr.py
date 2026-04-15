# RUN: %chocopy-llvm --run-sema %s 2>&1 | FileCheck %s.err

x:int = 0
o:object = None

x.foo
o.bar
o.baz = 1
