# RUN: %chocopy-llvm --run-sema %s 2>&1 | FileCheck %s.err

class A(object):
    def __init__(self:"A"):
        return 1 # Bad

A()
