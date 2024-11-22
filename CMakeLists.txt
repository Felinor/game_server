# Минимальная версия CMake
cmake_minimum_required(VERSION 3.10)

# Название проекта
project(endpoint)

# Find includes in the build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Turn on automatic invocation of the MOC, UIC & RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Флаги компилятора
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# Версия стандарта C++
set(CMAKE_CXX_STANDARD 17)

# Поиск библиотек Qt5
find_package(Qt5 REQUIRED COMPONENTS Core WebSockets)

# Добавление исполняемого файла и исходных файлов
add_executable(endpoint 
    main.cpp
    gameserver.h
    gameserver.cpp
    ioccontainer.h
)

# Линковка с библиотеками Qt5
target_link_libraries(endpoint Qt5::Core Qt5::WebSockets)
