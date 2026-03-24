# RUN: %chocopy-llvm --run-sema %s 2>&1 | FileCheck %s.err

class A(object):
    x:int = 1

    def __init__(self:"A", x:int): # Bad override
        pass

A(1)

