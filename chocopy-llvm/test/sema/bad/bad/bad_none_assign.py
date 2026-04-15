# RUN: %chocopy-llvm --run-sema %s 2>&1 | FileCheck %s.err

class A(object):
    x:int = 1

y:[bool] = None
z:str = ""
a:A = None

a = A()

# Cannot assign None to integer/bool/str types
z = None
a.x = None
y[0] = None

