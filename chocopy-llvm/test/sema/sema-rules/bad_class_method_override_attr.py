# RUN: %chocopy-llvm --run-sema %s 2>&1 | FileCheck %s.err

class A(object):
	f:int = 3

class B(A):
	def f(self:B) -> int:
		return 3

A()
