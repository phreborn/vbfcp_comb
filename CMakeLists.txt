cmake_minimum_required (VERSION 3.0)
project (workspaceCombiner)

find_package( Boost QUIET 1.60 HINTS ${ALRB_BOOST_ROOT})
if (NOT ${Boost_FOUND})
  message("did not find Boost via cmake, setting by hand from ALRB_BOOST_ROOT=$ENV{ALRB_BOOST_ROOT}")
  set(Boost_INCLUDE_DIRS "$ENV{ALRB_BOOST_ROOT}/include")
  set(Boost_LIBRARIES "-lboost_regex -lboost_program_options -L$ENV{ALRB_BOOST_ROOT}/lib")
endif()

find_package( ROOT REQUIRED COMPONENTS Core RIO MathCore Matrix HistFactory RooFitCore RooFit Hist RooStats )
FOREACH(incfile ${ROOT_USE_FILE})
  include(${incfile})
ENDFOREACH()

# set some variables for easier handling
set(workspaceCombinerLinkDef ${PROJECT_SOURCE_DIR}/interface/manager_LinkDef.h)
file(GLOB workspaceCombinerSources src/[A-Za-z]*.cxx)
file(GLOB workspaceCombinerHeaders workspaceCombiner/[A-Za-z]*.h)


if(EXISTS ${PROJECT_SOURCE_DIR}/RooFitExtensions)
  message("adding RooFitExtensions")
  add_subdirectory(RooFitExtensions)
  set (RooFitExtensions_LIBRARIES "RooFitExtensions")
endif()

# register all the files and directories
include_directories ("${ROOT_INCLUDE_DIRS}")
include_directories ("${Boost_INCLUDE_DIRS}")
include_directories ("${PROJECT_SOURCE_DIR}" interface)

# generate the dictionary source code
ROOT_GENERATE_DICTIONARY(G__workspaceCombiner ${workspaceCombinerHeaders} LINKDEF ${workspaceCombinerLinkDef})

# register the shared object to include both sources and dictionaries
add_library( workspaceCombiner SHARED ${workspaceCombinerSources} G__workspaceCombiner.cxx)

# link everything together at the end
target_link_libraries( workspaceCombiner ${ROOT_LIBRARIES} )

# compile the manager
add_executable (manager app/manager.cxx)
target_link_libraries (manager workspaceCombiner ${ROOT_LIBRARIES} ${Boost_LIBRARIES} ${RooFitExtensions_LIBRARIES})

# execute_process(COMMAND ln -s ${CMAKE_BINARY_DIR}/manager ${PROJECT_SOURCE_DIR})

set(SETUP cmake_setup.sh)
file(WRITE ${SETUP} "#!/bin/bash\n")
file(APPEND ${SETUP} "# this is an auto-generated setup script\n" )
file(APPEND ${SETUP} "export PATH=$PATH:${CMAKE_BINARY_DIR}\n" )

message("before running, 'source ${PROJECT_SOURCE_DIR}/cmake_setup.sh'") 