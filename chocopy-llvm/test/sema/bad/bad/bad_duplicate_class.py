# RUN: %chocopy-llvm --run-sema %s 2>&1 | FileCheck %s.err

class A(object):
    x:int = 1

z:bool = True

# Duplicate
class A(object):
    x:int = 1

# Duplicate
class str(object):
    x:int = 1

# Duplicate
class z(object):
    x:int = 1

A()
