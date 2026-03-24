# RUN: %chocopy-llvm --run-sema %s 2>&1 | FileCheck %s.err

x:int = 1
y:bool = True

x = False
y = 2
z = 3
x = z = 4
x = z = None
