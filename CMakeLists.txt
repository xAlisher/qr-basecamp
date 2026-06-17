cmake_minimum_required(VERSION 3.14)
project(QrPlugin LANGUAGES CXX)

if(DEFINED ENV{LOGOS_MODULE_BUILDER_ROOT})
    include($ENV{LOGOS_MODULE_BUILDER_ROOT}/cmake/LogosModule.cmake)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LogosModule.cmake")
    include(cmake/LogosModule.cmake)
else()
    message(FATAL_ERROR "LogosModule.cmake not found — set LOGOS_MODULE_BUILDER_ROOT")
endif()

logos_module(
    NAME qr
    SOURCES
        src/QrPlugin.h
        src/QrPlugin.cpp
        src/qrcodegen.hpp
        src/qrcodegen.cpp
    INCLUDE_DIRS
        src
)
