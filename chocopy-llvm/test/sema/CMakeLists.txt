set(CHOCOPY_TOOLS_BINARY_DIR ${CHOCOPY_BINARY_DIR}/bin)

configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/sema-rules/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/sema-rules/lit.site.cfg.py
  MAIN_CONFIG
  ${CMAKE_CURRENT_SOURCE_DIR}/sema-rules/lit.cfg.py
)

add_lit_testsuite(check-chpy-sema-basic-rules "Running the Chocopy sema basic-rules tests"
  ${CMAKE_CURRENT_BINARY_DIR}/sema-rules
)

configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/bad/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/bad/lit.site.cfg.py
  MAIN_CONFIG
  ${CMAKE_CURRENT_SOURCE_DIR}/bad/lit.cfg.py
)

add_lit_testsuite(check-chpy-sema-bad "Running the Chocopy sema bad tests"
  ${CMAKE_CURRENT_BINARY_DIR}/bad
)

configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/good-stage1/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/good-stage1/lit.site.cfg.py
  MAIN_CONFIG
  ${CMAKE_CURRENT_SOURCE_DIR}/good-stage1/lit.cfg.py
)

add_lit_testsuite(check-chpy-sema-good-stage1 "Running the Chocopy sema good-stage1 tests"
  ${CMAKE_CURRENT_BINARY_DIR}/good-stage1
)

configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/good-stage2/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/good-stage2/lit.site.cfg.py
  MAIN_CONFIG
  ${CMAKE_CURRENT_SOURCE_DIR}/good-stage2/lit.cfg.py
)

add_lit_testsuite(check-chpy-sema-good-stage2 "Running the Chocopy sema good-stage2 tests"
  ${CMAKE_CURRENT_BINARY_DIR}/good-stage2
)

configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/good-stage3/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/good-stage3/lit.site.cfg.py
  MAIN_CONFIG
  ${CMAKE_CURRENT_SOURCE_DIR}/good-stage3/lit.cfg.py
)

add_lit_testsuite(check-chpy-sema-good-stage3 "Running the Chocopy sema good-stage3 tests"
  ${CMAKE_CURRENT_BINARY_DIR}/good-stage3
)

add_custom_target(check-chpy-sema)
add_dependencies(check-chpy-sema
  check-chpy-sema-basic-rules
  check-chpy-sema-bad
  check-chpy-sema-good-stage1
  check-chpy-sema-good-stage2
  check-chpy-sema-good-stage3
)
