CMAKE_MINIMUM_REQUIRED(VERSION 3.14)
project(fire CXX)
message("project name is automaically set as: ${PROJECT_NAME}")

set(PROJECT_VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

set(FIRE_SOURCE_FILES
  src/Builtins.cpp
  src/Compiler.cpp
  src/Driver.cpp
  src/Error.cpp
  src/fs_impl.cpp
  src/FSWrap.cpp
  src/IR.cpp
  src/Lexer.cpp
  src/Lower.cpp
  src/main.cpp
  src/Object.cpp
  src/Parser.cpp
  src/Sema_NameResolver.cpp
  src/Sema_Scopes.cpp
  src/Sema_TypeChecker.cpp
  src/Sema.cpp
  src/SourceFile.cpp
  src/strconv.cpp
  src/string.cpp
  src/Token.cpp
  src/TypeInfo.cpp
  src/Utils.cpp
)

set(FIRE_HEADER_FILES
    include/BuiltinFunc.hpp
    include/defs.hpp
    include/Driver.hpp
    include/Error.hpp
    include/FileSystem.hpp
    include/fs_impl.hpp
    include/IR.hpp
    include/Lexer.hpp
    include/logger.h
    include/Lower.hpp
    include/Node.hpp
    include/Object.hpp
    include/Parser.hpp
    include/Sema.hpp
    include/SourceFile.hpp
    include/strconv.hpp
    include/string.hpp
    include/Token.hpp
    include/TypeInfo.hpp
    include/Utils.hpp
    include/VM.hpp
)

add_executable(fire ${FIRE_SOURCE_FILES} ${FIRE_HEADER_FILES})

target_include_directories(fire PUBLIC include)
