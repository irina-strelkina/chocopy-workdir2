#!/bin/bash

_OLD_CHOCOPY_PATH=$PATH

CHPY_WORKDIR=$(readlink -f ../)

PATH=${CHPY_WORKDIR}/chocopy-llvm-build-debug/bin:$PATH
PATH=${CHPY_WORKDIR}/llvm-build-debug/bin:$PATH

function chocopy-llvm-exit() {
  if [[ -n "$_OLD_CHOCOPY_PATH" ]]; then
    PATH=$_OLD_CHOCOPY_PATH
  fi
  unset _OLD_CHOCOPY_PATH
  unset chocopy-llvm-exit
}
