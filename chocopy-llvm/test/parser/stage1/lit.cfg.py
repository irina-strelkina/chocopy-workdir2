# -*- Python -*-

# Configuration file for the 'lit' test runner.

import os
import sys
import re
import platform
import subprocess

import lit.util
import lit.formats
from lit.llvm import llvm_config
from lit.llvm.subst import FindTool
from lit.llvm.subst import ToolSubst

# name: The name of this test suite.
config.name = "CHOCOPY-LLVM-STAGE1"

config.suffixes = ['.py']
config.excludes = [ 'lit.cfg.py' ]

# testFormat: The test format to use to interpret tests.
config.test_format = lit.formats.ShTest(not llvm_config.use_lit_shell)

config.test_exec_root = os.path.join(config.chpy_obj_root, "test", "parser")

# test_source_root: The root path where tests are located.
config.test_source_root = os.path.dirname(__file__)

# Tweak the PATH to include the tools dir.
llvm_config.with_environment("PATH", config.chpy_tools_dir, append_path=True)
llvm_config.with_environment("PATH", config.llvm_tools_dir, append_path=True)

tools = [
  ToolSubst("%chocopy-llvm", FindTool("chocopy-llvm"))
]

search_dirs = [config.chpy_tools_dir, config.llvm_tools_dir]
llvm_config.add_tool_substitutions(tools=tools, search_dirs=search_dirs)
