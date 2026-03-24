# RUN: %chocopy-llvm --run-sema %s 2>&1 | FileCheck %s.err

class x(object):
    pass

x:int = 5

pass
