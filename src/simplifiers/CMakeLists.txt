add_library(simplifiers OBJECT "")

target_sources(simplifiers
PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/LA.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/SubstitutionSimplifier.h"
PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/LA.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/BoolRewriting.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/SubstitutionSimplifier.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/ArithSubstitutionSimplifier.cc"
)

install(FILES
        BoolRewriting.h
        SubstitutionSimplifier.h
        DESTINATION ${INSTALL_HEADERS_DIR}
)
