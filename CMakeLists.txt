# CMakeLists.txt
#
# In order to build with cmake, setup your favourite root
# version. Then do
#
#  mkdir build
#  cd build
#  cmake ..
#  make -j4
#
# After this, you can use the workspace combiner like normal.

cmake_minimum_required (VERSION 3.0)
project (workspaceCombiner)

find_package( Boost QUIET 1.60 HINTS ${ALRB_BOOST_ROOT} COMPONENTS program_options regex)
if (NOT ${Boost_FOUND})
  message("did not find Boost via cmake, setting by hand from ALRB_BOOST_ROOT=$ENV{ALRB_BOOST_ROOT}")
  set(Boost_INCLUDE_DIRS "$ENV{ALRB_BOOST_ROOT}/include")
  set(Boost_LIBRARIES "-lboost_regex -lboost_program_options -L$ENV{ALRB_BOOST_ROOT}/lib")
endif()

find_package( AnalysisBase QUIET) 

IF(${AnalysisBase_FOUND})
  # Set up the usage of CTest:
  IF(NOT ${HAS_PARENT})
    atlas_ctest_setup() # Set up the project: 
    atlas_project( workspaceCombiner 1.0.0 
      USE AnalysisBase ${AnalysisBase_VERSION} ) 
    
    # Generate an environment setup script: 
    lcg_generate_env( SH_FILE ${CMAKE_BINARY_DIR}/${ATLAS_PLATFORM}/env_setup.sh )
    install( FILES ${CMAKE_BINARY_DIR}/${ATLAS_PLATFORM}/env_setup.sh DESTINATION . ) 
    
    # Set up the usage of CPack: 
    set(CMAKE_INSTALL_PREFIX /InstallArea/x86_64-slc6-gcc49-opt)
    atlas_cpack_setup()
  ENDIF()

  # register this as a package to ASG
  atlas_subdir( workspaceCombiner )
ENDIF()

include_directories ("${PROJECT_SOURCE_DIR}")
find_package( ROOT REQUIRED COMPONENTS Core RIO MathCore XMLIO XMLParser HistPainter Graf Gpad HistFactory RooFitCore RooFit Hist RooStats )
FOREACH(incfile ${ROOT_USE_FILE})
  include(${incfile})
ENDFOREACH()

# set some variables for easier handling
set(workspaceCombinerLinkDef interface/manager_LinkDef.h)
file(GLOB workspaceCombinerSources RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src/[A-Za-z]*.cxx)
file(GLOB workspaceCombinerHeaders RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/interface/[A-Za-z]*.h)
file(GLOB workspaceCombinerHeaders_Options RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/interface/[A-Za-z]*Options.h)
list(REMOVE_ITEM workspaceCombinerHeaders ${workspaceCombinerLinkDef} ${workspaceCombinerHeaders_Options})

# register all the files and directories
include_directories ("${ROOT_INCLUDE_DIRS}")
include_directories ("${Boost_INCLUDE_DIRS}")
include_directories ("${CMAKE_CURRENT_SOURCE_DIR}" interface)

if(EXISTS ${PROJECT_SOURCE_DIR}/RooFitExtensions)
  message("adding RooFitExtensions")
  add_subdirectory(RooFitExtensions)
  set (RooFitExtensions_LIBRARIES "RooFitExtensions")
endif()

# execute_process(COMMAND ln -s ${CMAKE_BINARY_DIR}/manager ${PROJECT_SOURCE_DIR})
set(SETUP cmake_setup.sh)
file(WRITE ${SETUP} "#!/bin/bash\n")
file(APPEND ${SETUP} "# this is an auto-generated setup script\n" )

if(${AnalysisBase_FOUND})
  # this section reflects the standard ASG way of configuring CMake
  # it is executed when compiling within an ASG environment

  atlas_platform_id( BINARY_TAG )
  
  find_package( GTest )
  set(CMAKE_INSTALL_PREFIX /InstallArea/x86_64-slc6-gcc62-opt)
  atlas_add_root_dictionary( workspaceCombiner workspaceCombinerCintDict
    ROOT_HEADERS ${workspaceCombinerHeaders} ${workspaceCombinerLinkDef}
    EXTERNAL_PACKAGES ROOT )
  atlas_add_library( workspaceCombiner
    ${workspaceCombinerHeaders} ${workspaceCombinerSources} ${workspaceCombinerCintDict}
    PUBLIC_HEADERS workspaceCombiner
    PRIVATE_INCLUDE_DIRS ${ROOT_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS} 
    PRIVATE_LINK_LIBRARIES ${ROOT_LIBRARIES} ${Boost_LIBRARIES} 
    )
  
  atlas_add_executable (manager app/manager.cxx)
  target_link_libraries (manager workspaceCombiner ${ROOT_LIBRARIES} ${Boost_LIBRARIES} ${RooFitExtensions_LIBRARIES})
  file(APPEND ${SETUP} "export PATH=$PATH:${CMAKE_BINARY_DIR}/${BINARY_TAG}/bin\n" )
  file(APPEND ${SETUP} "export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${CMAKE_BINARY_DIR}/${BINARY_TAG}/lib\n" )  
ELSE()
  
  # generate the dictionary source code
  ROOT_GENERATE_DICTIONARY(G__workspaceCombiner ${workspaceCombinerHeaders} LINKDEF ${workspaceCombinerLinkDef})
  
  # register the shared object to include both sources and dictionaries
  add_library( workspaceCombiner SHARED ${workspaceCombinerSources} G__workspaceCombiner.cxx)
  
  # link everything together at the end
  target_link_libraries( workspaceCombiner ${ROOT_LIBRARIES} )
  
  # compile the manager
  add_executable (manager app/manager.cxx)
  target_link_libraries (manager workspaceCombiner ${ROOT_LIBRARIES} ${Boost_LIBRARIES} ${RooFitExtensions_LIBRARIES})
  file(APPEND ${SETUP} "export PATH=$PATH:${CMAKE_CURRENT_BINARY_DIR}\n" )
ENDIF()



message("before running, 'source ${PROJECT_SOURCE_DIR}/cmake_setup.sh'") 
