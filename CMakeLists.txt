cmake_minimum_required(VERSION 3.16)

project(myPython LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core LinguistTools)


set(TS_FILES myPython_ru_BY.ts)

add_executable(myPython
  main.cpp
  myPython_ru_BY.xml
  headers/Lexer.h
  headers/Parser.h
  sources/Lexer.cpp
  sources/Parser.cpp
  headers/Interpreter.h
  sources/Interpreter.cpp
  headers/Environment.h
  sources/Environment.cpp
  headers/Value.h
  sources/Value.cpp
)


target_include_directories(myPython PRIVATE
        headers
        externals/replxx/include
)


target_link_libraries(myPython PRIVATE Qt6::Core)

if(COMMAND qt_create_translation)
    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

include(GNUInstallDirs)
install(TARGETS myPython
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
