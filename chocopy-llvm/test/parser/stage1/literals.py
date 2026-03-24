# RUN: %chocopy-llvm %s -ast-dump | diff %s.ast -

True
False
1
None
"This is a string"
[1, 2, 3]
