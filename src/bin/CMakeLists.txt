add_executable(OpenSMT-bin opensmt.cc Interpret.cc)

set_target_properties(OpenSMT-bin PROPERTIES
    NO_SYSTEM_FROM_IMPORTED true # For some reason on MAC OS build in travis, the .cc files from this target were compiled with -isystem /usr/include and that was messing up the search order of include paths; This seems to fix it
    OUTPUT_NAME opensmt
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

set(PRIVATE_SOURCES_TO_ADD
        "${CMAKE_CURRENT_SOURCE_DIR}/Interpret.cc"
        )
set(PUBLIC_SOURCES_TO_ADD
        "${CMAKE_CURRENT_SOURCE_DIR}/Interpret.h"
        )
if (ENABLE_LINE_EDITING)
    if (NOT USE_READLINE)
        find_package(LibEdit REQUIRED)
        target_link_libraries(OpenSMT-bin PUBLIC LibEdit::LibEdit)
    else()
        find_package(Readline REQUIRED)
        target_link_libraries(OpenSMT-bin PUBLIC Readline::Readline)
    endif()
endif()

target_link_libraries(OpenSMT-bin PUBLIC OpenSMT-static)


install(TARGETS OpenSMT-bin RUNTIME DESTINATION bin)
