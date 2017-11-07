cmake_minimum_required(VERSION 2.8)

project(caf_latency_benchmarks)
message(STATUS "CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")

# The version number.
set (caf_latency_benchmarks_VERSION_MAJOR 1)
set (caf_latency_benchmarks_VERSION_MINOR 0)

# path to caf core & io headers
set(CAF_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../install/caf/include")

include_directories("${CAF_INCLUDE_DIRS}" "include")

message(STATUS "CAF_INCLUDE_DIRS=${CAF_INCLUDE_DIRS}\n")


if(WIN32)
  add_library(caf_core_static.lib STATIC IMPORTED)
  add_library(caf_io_static.lib STATIC IMPORTED)
  set_target_properties(caf_core_static.lib PROPERTIES IMPORTED_LOCATION_DEBUG "${CAF_LIBRARIES} ${CMAKE_CURRENT_SOURCE_DIR}/../install/caf/lib/caf_core_static.lib")
  set_target_properties(caf_core_static.lib PROPERTIES IMPORTED_LOCATION_RELEASE "${CAF_LIBRARIES} ${CMAKE_CURRENT_SOURCE_DIR}/../install/caf/lib/caf_core_static.lib")
  set_target_properties(caf_core_static.lib PROPERTIES IMPORTED_LOCATION_MINSIZEREL "${CAF_LIBRARIES} ${CMAKE_CURRENT_SOURCE_DIR}/../install/caf/lib/caf_core_static.lib")
  set_target_properties(caf_core_static.lib PROPERTIES IMPORTED_LOCATION_RELWITHDEBINFO "${CAF_LIBRARIES} ${CMAKE_CURRENT_SOURCE_DIR}/../install/caf/lib/caf_core_static.lib")
  set_target_properties(caf_io_static.lib PROPERTIES IMPORTED_LOCATION_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/../install/caf/lib/caf_io_static.lib")
  set_target_properties(caf_io_static.lib PROPERTIES IMPORTED_LOCATION_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/../install/caf/lib/caf_io_static.lib")
  set_target_properties(caf_io_static.lib PROPERTIES IMPORTED_LOCATION_MINSIZEREL "${CMAKE_CURRENT_SOURCE_DIR}/../install/caf/lib/caf_io_static.lib")
  set_target_properties(caf_io_static.lib PROPERTIES IMPORTED_LOCATION_RELWITHDEBINFO "${CMAKE_CURRENT_SOURCE_DIR}/../install/caf/lib/caf_io_static.lib")

 SET(CAF_LIBRARIES caf_core_static.lib caf_io_static.lib)
elseif( CMAKE_BUILD_TYPE STREQUAL "Debug"  )
  SET(CAF_LIBRARIES debug ${CAF_LIBRARIES} "${CMAKE_CURRENT_SOURCE_DIR}/../install/caf/lib/libcaf_io_static.a" "${CMAKE_CURRENT_SOURCE_DIR}/../install/caf/lib/libcaf_core_static.a")
else()
  SET(CAF_LIBRARIES optimized ${CAF_LIBRARIES} "${CMAKE_CURRENT_SOURCE_DIR}/../install/caf/lib/libcaf_io_static.a" "${CMAKE_CURRENT_SOURCE_DIR}/../install/caf/lib/libcaf_core_static.a")
endif()
      
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
  set(EXTRA_FLAGS "-ftemplate-depth=512 -ftemplate-backtrace-limit=0")
endif()

if(NOT WIN32)
  set(EXTRA_FLAGS "${EXTRA_FLAGS} -pthread")
  add_definitions(-DHAS_REMOTE_API=0)
endif()

add_definitions(-DDATE_BUILD_LIB)
set(EXTRA_FLAGS "${EXTRA_FLAGS} -fPIC")

if (WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  set(LD_FLAGS ${LD_FLAGS} ws2_32 iphlpapi)
  if (NOT "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
  endif()
endif()

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-variable -Wno-missing-braces -Wno-unused-local-typedef -Wno-gnu-string-literal-operator-template")
endif()
 

message(STATUS "EXTRA_FLAGS=${EXTRA_FLAGS}\n")
message(STATUS "CMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}\n")
      
# check if the user provided CXXFLAGS, set defaults otherwise
if(NOT CMAKE_CXX_FLAGS)
  set(CMAKE_CXX_FLAGS                   "-std=c++14 -Wextra -Wall")
endif()

if(NOT CMAKE_CXX_FLAGS_DEBUG)
  set(CMAKE_CXX_FLAGS_DEBUG             "-O0 -g")
endif()
if(NOT CMAKE_CXX_FLAGS_RELEASE)
  set(CMAKE_CXX_FLAGS_RELEASE           "-O3 -DNDEBUG")
endif()
if(NOT CMAKE_CXX_FLAGS_RELWITHDEBINFO)
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO    "-O2 -g")
endif()
if(NOT CMAKE_CXX_FLAGS_MINSIZEREL)
  set(CMAKE_CXX_FLAGS_MINSIZEREL        "-Os")
endif()
# set build default build type to RelWithDebInfo if not set
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()
# needed by subprojects
set(LD_FLAGS ${LD_FLAGS} ${CMAKE_LD_LIBS})

file(GLOB_RECURSE ALL_FILES
	 RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
	 ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
	 ${CMAKE_CURRENT_SOURCE_DIR}/*.hh
	 ${CMAKE_CURRENT_SOURCE_DIR}/*.h
	 ${CMAKE_CURRENT_SOURCE_DIR}/*.md
	 ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

list(REMOVE_ITEM ALL_FILES build/CMakeFiles/${CMAKE_VERSION}/CompilerIdCXX/CMakeCXXCompilerId.cpp)
 
add_executable(caf_latency_benchmarks ${ALL_FILES})
 
if(MSVC)
    set_property(TARGET caf_latency_benchmarks	APPEND PROPERTY LINK_FLAGS "/DEBUG")
endif()

# Filter for VS
if(WIN32) 
  function(assign_source_group)
    foreach(_source IN ITEMS ${ARGN})
        if (IS_ABSOLUTE "${_source}")
            file(RELATIVE_PATH _source_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_source}")
        else()
            set(_source_rel "${_source}")
        endif()
        get_filename_component(_source_path "${_source_rel}" PATH)
        string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
        source_group("${_source_path_msvc}" FILES "${_source}")
    endforeach()
  endfunction(assign_source_group)
  
  assign_source_group(${ALL_FILES} ${DATE_FILES})
endif()

message(STATUS "CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}\n")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}\n")
message(STATUS "ALL_FILES=${ALL_FILES}\n")
message(STATUS "DATE_FILES=${DATE_FILES}\n")
message(STATUS "LD_FLAGS=${LD_FLAGS}\n")
message(STATUS "CAF_LIBRARIES=${CAF_LIBRARIES}\n")
message(STATUS "WSLIB=${WSLIB}\n")
message(STATUS "CMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}\n")

find_package (Threads)
message(STATUS "CMAKE_THREAD_LIBS_INIT=${CMAKE_THREAD_LIBS_INIT}\n")

target_link_libraries(caf_latency_benchmarks
                        ${CAF_LIBRARIES}
                        ${CMAKE_THREAD_LIBS_INIT}
                        ${LD_FLAGS}
                        ${WSLIB})
						

############ INSTALL BINARIES

install(TARGETS caf_latency_benchmarks DESTINATION bin)

############ INSTALL PDB
  
if (WIN32) 
	install(DIRECTORY ${PROJECT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/
			DESTINATION bin FILES_MATCHING PATTERN *.pdb)
endif()

########################
