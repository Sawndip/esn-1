include(FindPackageHandleStandardArgs)

if(NOT ESN_USE_SYSTEM_GTEST)
    add_library(gtest SHARED IMPORTED)

    if(MSVC)
        set_property(TARGET gtest PROPERTY IMPORTED_IMPLIB
            "${CMAKE_BINARY_DIR}/gtest/src/gtest-project-build/${CMAKE_CFG_INTDIR}/gtest.lib")
    else()
        set_property(TARGET gtest PROPERTY IMPORTED_LOCATION
            "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libgtest.so")
    endif()

    add_dependencies(gtest gtest-project)

    set(GTEST_LIBRARIES gtest)
    set(GTEST_INCLUDE_DIRS
        "${CMAKE_BINARY_DIR}/gtest/src/gtest-project/include")
else()
    find_library(
        GTEST_LIBRARIES gtest
        HINTS
            ENV GTEST_ROOT
            ${GTEST_ROOT}
    )
    find_path(
        GTEST_INCLUDE_DIRS gtest/gtest.h
        HINTS
            $ENV{GTEST_ROOT}/include
            ${GTEST_ROOT}/include
    )
endif()

find_package_handle_standard_args(
    GTest DEFAULT_MSG GTEST_LIBRARIES GTEST_INCLUDE_DIRS)
