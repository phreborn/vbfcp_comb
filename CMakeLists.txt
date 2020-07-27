##################################################################################################
# Package: workspaceCombiner #####################################################################
cmake_minimum_required( VERSION 2.8 )

# Declare the package name:
project( workspaceCombiner )

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_INSTALL_LIBDIR ${PROJECT_SOURCE_DIR}/lib)
set(TARGET_INSTALL_AREA ${PROJECT_SOURCE_DIR})
set(SHARED_INSTALL_AREA ${PROJECT_SOURCE_DIR}/lib)

file(MAKE_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
file(MAKE_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

# You need to tell CMake where to find the ROOT installation. This can be done in a number of ways:
#   - ROOT built with classic configure/make use the provided $ROOTSYS/etc/cmake/FindROOT.cmake
#   - ROOT built with CMake. Add in CMAKE_PREFIX_PATH the installation prefix for ROOT
list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})

# require ROOT
find_package( ROOT REQUIRED COMPONENTS RIO RooFitCore RooFit HistFactory RooStats MathCore MathMore Physics Minuit Foam XMLParser )
include(${ROOT_USE_FILE})

# require Boost
list(APPEND CMAKE_PREFIX_PATH $ENV{BOOST_HOME})
find_package(Boost REQUIRED COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIRS})

# require RooFitExtensions
list(APPEND CMAKE_PREFIX_PATH $ENV{RooFitExtensions_DIR})
find_package(RooFitExtensions REQUIRED CONFIG)
include_directories(${RooFitExtensions_INCLUDE_DIRS})
link_directories(${RooFitExtensions_LIBRARY_DIRS})

include_directories(${PROJECT_SOURCE_DIR}/inc)

set(workspaceCombinerHeaders "")
set(workspaceCombinerSources "")

file(GLOB incPaths inc/*)
file(GLOB srcPaths src/*)

foreach (_incPath ${incPaths})
	get_filename_component(_incName ${_incPath} NAME)
    	if(NOT ${_incName} MATCHES "manager_LinkDef.h")
    	       list(APPEND workspaceCombinerHeaders ${_incName})
	endif()
endforeach()

foreach (_srcPath ${srcPaths})
	get_filename_component(_srcName ${_srcPath} NAME)
	list(APPEND workspaceCombinerSources "src/${_srcName}")
endforeach()

# generate the dictionary source code
ROOT_GENERATE_DICTIONARY(G__workspaceCombiner ${workspaceCombinerHeaders} LINKDEF inc/manager_LinkDef.h)

add_library( workspaceCombiner SHARED ${workspaceCombinerSources} G__workspaceCombiner.cxx)
target_link_libraries( workspaceCombiner ${ROOT_LIBRARIES} ${RooFitExtensions_LIBRARIES})

#add executables

file(GLOB exePaths app/*)

set(workspaceCombinerApp "")

foreach (_exePath ${exePaths})
	get_filename_component(_exeName ${_exePath} NAME_WE)
	get_filename_component(_exeFile ${_exePath} NAME)
	add_executable( ${_exeName} app/${_exeFile} )
    	target_link_libraries( ${_exeName} workspaceCombiner Boost::program_options )
	list(APPEND workspaceCombinerApp ${_exeName})
endforeach()

install(TARGETS ${workspaceCombinerApp} DESTINATION bin)
install(TARGETS workspaceCombiner LIBRARY DESTINATION lib)
install(FILES ${PROJECT_SOURCE_DIR}/lib/libworkspaceCombiner_rdict.pcm ${PROJECT_SOURCE_DIR}/lib/libworkspaceCombiner.rootmap DESTINATION lib)
