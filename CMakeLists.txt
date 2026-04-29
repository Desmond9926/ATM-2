cmake_minimum_required(VERSION 3.20)

project(ATMSystem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(MYSQL_ROOT "C:/Program Files/MySQL/MySQL Server 8.0")
set(MYSQL_INCLUDE_DIR "${MYSQL_ROOT}/include")
set(MYSQL_LIBRARY "${MYSQL_ROOT}/lib/libmysql.lib")
set(MYSQL_RUNTIME_DLL "${MYSQL_ROOT}/lib/libmysql.dll")

set(COMMON_SOURCES
    common/JsonUtil.cpp
    common/Protocol.cpp
)

set(COMMON_HEADERS
    common/JsonUtil.h
    common/Models.h
    common/Protocol.h
)

add_library(atm_common STATIC
    ${COMMON_SOURCES}
    ${COMMON_HEADERS}
)

target_include_directories(atm_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)

add_executable(atm_server
    server/AccountService.cpp
    server/AuthService.cpp
    server/Database.cpp
    server/PasswordUtil.cpp
    server/RequestHandler.cpp
    server/ServerMain.cpp
    server/TcpServer.cpp
    server/TransactionService.cpp
    server/AccountService.h
    server/AuthService.h
    server/Database.h
    server/PasswordUtil.h
    server/RequestHandler.h
    server/TcpServer.h
    server/TransactionService.h
)

target_include_directories(atm_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${MYSQL_INCLUDE_DIR}
)

target_link_libraries(atm_server PRIVATE
    atm_common
    bcrypt
    ${MYSQL_LIBRARY}
    ws2_32
)

if(EXISTS "${MYSQL_RUNTIME_DLL}")
    add_custom_command(TARGET atm_server POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${MYSQL_RUNTIME_DLL}"
            "$<TARGET_FILE_DIR:atm_server>/libmysql.dll"
    )
endif()

add_executable(atm_client
    client/ClientApp.cpp
    client/ClientMain.cpp
    client/ClientSocket.cpp
    client/ClientApp.h
    client/ClientSocket.h
)

target_include_directories(atm_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(atm_client PRIVATE
    atm_common
    ws2_32
)
