cmake_minimum_required (VERSION 2.8)
project (CMTK4Slicer)
include(ExternalProject)

#http://cmake.org/gitweb?p=cmake.git;a=blob;f=Modules/ExternalProject.cmake



# --------------------------------------------------------------------------
# Find Slicer3

if(NOT Slicer3_SOURCE_DIR)
  find_package(Slicer3 REQUIRED)
  include(${Slicer3_USE_FILE})
  slicer3_set_default_install_prefix_for_external_projects()
endif(NOT Slicer3_SOURCE_DIR)

set(CMTK_SOURCE_DIR CMTK)
set(CMTK_BINARY_DIR CMTK-build)
set(CMTK_INSTALL_DIR CMTK-install)

message("######### ALL: ${CMAKE_CURRENT_BINARY_DIR}/${CMTK_BINARY_DIR}/bin/registration")
message("######### CMTK_BINARY_DIR: ${CMTK_BINARY_DIR}")
message("######### Slicer3_INSTALL_PLUGINS_BIN_DIR: ${Slicer3_INSTALL_PLUGINS_BIN_DIR}")
message("######### CMAKE_CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}")

set(PROJECTNAME CMTK)
ExternalProject_Add(${PROJECTNAME}
  SVN_REPOSITORY "https://www.nitrc.org/svn/cmtk/trunk/core"
#  SVN_USERNAME slicerbot
#  SVN_PASSWORD slicer
  CMAKE_ARGS
    ${LOCAL_CMAKE_BUILD_OPTIONS}
    -DCMAKE_INSTALL_PREFIX:PATH=${CMTK_INSTALL_DIR}
)

#file(GLOB CMTKFILES RELATIVE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECTNAME}-prefix/src/${PROJECTNAME}-build/${PROJECTNAME}-install/bin/" "*")


install(
#  FILES ${CMTKFILES}
#  FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECTNAME}-prefix/src/${PROJECTNAME}-build/${PROJECTNAME}-install/bin/registration
  DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${PROJECTNAME}-prefix/src/${PROJECTNAME}-build/${PROJECTNAME}-install/bin/
  DESTINATION ${Slicer3_INSTALL_PLUGINS_BIN_DIR}
  FILES_MATCHING PATTERN "*"
  PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
              GROUP_EXECUTE GROUP_READ
              WORLD_EXECUTE WORLD_READ
)

install(
  DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${PROJECTNAME}-prefix/src/${PROJECTNAME}-build/${PROJECTNAME}-install/lib/
  DESTINATION ${Slicer3_INSTALL_PLUGINS_BIN_DIR}
  FILES_MATCHING PATTERN "*"
  PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
              GROUP_EXECUTE GROUP_READ
              WORLD_EXECUTE WORLD_READ
)


