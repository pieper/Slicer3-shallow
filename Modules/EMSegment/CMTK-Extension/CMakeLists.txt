cmake_minimum_required (VERSION 2.8)
project (CMTK4Slicer)
include(ExternalProject)

#http://cmake.org/gitweb?p=cmake.git;a=blob;f=Modules/ExternalProject.cmake

# --------------------------------------------------------------------------
# Find Slicer3

if(NOT Slicer3_SOURCE_DIR)
#  find_package(Slicer3 REQUIRED)
#  include(${Slicer3_USE_FILE})
#  slicer3_set_default_install_prefix_for_external_projects()
endif(NOT Slicer3_SOURCE_DIR)

set(CMTK_SOURCE_DIR CMTK-source)
set(CMTK_BINARY_DIR CMTK-build)
set(CMTK_INSTALL_DIR CMTK-install)

ExternalProject_Add(CMTK
  SVN_REPOSITORY "https://www.nitrc.org/svn/cmtk/trunk/core"
  SVN_USERNAME slicerbot
  SVN_PASSWORD slicer
  SOURCE_DIR ${CMTK_SOURCE_DIR}
  BINARY_DIR ${CMTK_BINARY_DIR}
#  DEPENDS  ${ITK_DEPEND}
#  CMAKE_GENERATOR ${gen}
  CMAKE_ARGS
    ${LOCAL_CMAKE_BUILD_OPTIONS}
#    -DITK_DIR:PATH=${ITK_DIR}
    -DCMAKE_INSTALL_PREFIX:PATH=${CMTK_INSTALL_DIR}
#  INSTALL_DIR ${CMTK_INSTALL_DIR}
)

install(
  FILES ${CMTK_BINARY_DIR}/bin/registration
  DESTINATION /tmp/Slicer3.6.2
 # DESTINATION ${Slicer3_INSTALL_PLUGINS_BIN_DIR}
)
