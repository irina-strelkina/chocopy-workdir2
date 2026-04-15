# RUN: %chocopy-llvm --run-sema %s 2>&1 | FileCheck %s.err

1 and 2
1 or 2
True - False
True < False
1 + True
False + 0
1 == True
False != 0
1 is 1
True is False
None + None
None == None
None is None
