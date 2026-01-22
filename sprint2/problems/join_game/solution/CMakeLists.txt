cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

find_package(Boost 1.78.0 REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(game_server
	src/main.cpp
	src/http_server.cpp
	src/http_server.h
	src/sdk.h
	src/model.h
	src/model.cpp
	src/tagged.h
	src/boost_json.cpp
	src/json_loader.h
	src/json_loader.cpp
	src/request_handler.h
	src/json_keys.h
	src/json_serializer.h
	src/json_serializer.cpp
	src/api_handler.h
	src/api_handler.cpp
	src/http_types.h
	src/http_response.h
	src/http_response.cpp
	src/url_utils.h
	src/file_handler.h
	src/players.h
	src/players.cpp
	src/session.h
	src/session.cpp
	src/player_tokens.h
	src/player_tokens.cpp
)
target_link_libraries(game_server PRIVATE Threads::Threads)
