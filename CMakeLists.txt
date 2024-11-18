cmake_minimum_required(VERSION 3.10)

project(tasky-task-tracker VERSION 1.0)

add_executable(tasky main.cpp)

configure_file(TaskyConfig.h.in TaskyConfig.h)

# Проверяем, задана ли переменная XDG_CONFIG_FILE
if("$ENV{XDG_CONFIG_FILE}" STREQUAL "")
    set(CONFIG_DIR "$ENV{HOME}/.config/tasky")
else()
    set(CONFIG_DIR "$ENV{XDG_CONFIG_FILE}/tasky")
endif()

# Создаем директорию

execute_process(COMMAND mkdir -p ${CONFIG_DIR})

# Создаем файлы

execute_process(COMMAND touch "${CONFIG_DIR}/data.json")
execute_process(COMMAND touch "${CONFIG_DIR}/issued_ID.txt")

# Добавляем в /usr/bin

# Проверяем, существует ли /usr/bin/tasky
execute_process(
    COMMAND test -e /usr/bin/tasky
    RESULT_VARIABLE TASKY_EXISTS
)

# Если файл не существует, создаем жесткую ссылку
if(NOT TASKY_EXISTS EQUAL 0)
    add_custom_command(TARGET tasky POST_BUILD COMMAND sudo ln ${CMAKE_CURRENT_BINARY_DIR}/tasky /usr/bin/tasky)
endif()

target_include_directories(tasky PRIVATE ./build)