# RUN: %chocopy-llvm %s -ast-dump | diff %s.ast -

class Foo(object):
    x:int = 1

f = Foo()
