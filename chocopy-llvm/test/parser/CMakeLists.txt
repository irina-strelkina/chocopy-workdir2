set(CHOCOPY_TOOLS_BINARY_DIR ${CHOCOPY_BINARY_DIR}/bin)

configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/bad/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/bad/lit.site.cfg.py
  MAIN_CONFIG
  ${CMAKE_CURRENT_SOURCE_DIR}/bad/lit.cfg.py
)

configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/stage1/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/stage1/lit.site.cfg.py
  MAIN_CONFIG
  ${CMAKE_CURRENT_SOURCE_DIR}/stage1/lit.cfg.py
)

configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/stage2/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/stage2/lit.site.cfg.py
  MAIN_CONFIG
  ${CMAKE_CURRENT_SOURCE_DIR}/stage2/lit.cfg.py
)

add_lit_testsuite(check-chpy-parser-stage1 "Running the Chocopy stage1 parser tests"
  ${CMAKE_CURRENT_BINARY_DIR}/stage1
)

add_lit_testsuite(check-chpy-parser-stage2 "Running the Chocopy stage2 parser tests"
  ${CMAKE_CURRENT_BINARY_DIR}/stage2
)

add_lit_testsuite(check-chpy-parser-bad "Running the Chocopy bad parser tests"
  ${CMAKE_CURRENT_BINARY_DIR}/bad
)

add_custom_target(check-chpy-parser)
add_dependencies(check-chpy-parser
  check-chpy-parser-bad
  check-chpy-parser-stage1
  check-chpy-parser-stage2)