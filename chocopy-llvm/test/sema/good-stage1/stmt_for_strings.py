# RUN: %chocopy-llvm --run-sema --ast-dump %s 2>&1 | diff %s.ast -

s:str = "Hello"

for s in s:
    s[0]
