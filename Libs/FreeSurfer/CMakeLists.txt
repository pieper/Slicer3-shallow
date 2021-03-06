project(FreeSurfer)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)
MARK_AS_ADVANCED(CMAKE_BACKWARDS_COMPATIBILITY)

 
find_package(VTK REQUIRED)
include(${VTK_USE_FILE})

if(NOT DEFINED BUILD_SHARED_LIBS)
  option(BUILD_SHARED_LIBS "Build with shared libraries." ON)
endif(NOT DEFINED BUILD_SHARED_LIBS)
 
# --------------------------------------------------------------------------
# Include dirs

set(include_dirs
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  )

include_directories(${include_dirs})

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/FreeSurferConfigure.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/FreeSurferConfigure.h
  )

file(GLOB headers "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
install(
  FILES ${headers} 
  "${CMAKE_CURRENT_BINARY_DIR}/FreeSurferConfigure.h" 
  DESTINATION include/${PROJECT_NAME} COMPONENT Development
  )

# --------------------------------------------------------------------------
# Sources

set(FreeSurfer_SRCS
  vtkFSIO.cxx
  vtkFSSurfaceReader.cxx
  vtkFSSurfaceAnnotationReader.cxx
  vtkFSSurfaceScalarReader.cxx
  vtkFSSurfaceWFileReader.cxx
  vtkFSSurfaceLabelReader.cxx
  vtkFSLookupTable.cxx
  )

# --------------------------------------------------------------------------
# Wrapping
IF (VTK_WRAP_TCL)
  include("${VTK_CMAKE_DIR}/vtkWrapTcl.cmake")
  vtk_wrap_tcl3(FreeSurfer FreeSurfer_TCL_SRCS "${FreeSurfer_SRCS}" "")
ENDIF (VTK_WRAP_TCL)

# --------------------------------------------------------------------------
# Build and install the library

set(lib_name FreeSurfer)
SET(srcs ${FreeSurfer_SRCS})

IF (VTK_WRAP_TCL)
  SET(srcs ${srcs} ${FreeSurfer_TCL_SRCS} )
ENDIF (VTK_WRAP_TCL)

add_library(${lib_name} ${srcs})

SET(VTK_LIBRARIES
  vtkCommon
  vtkIO
  )
  
#SET(libs
#  )

#TARGET_LINK_LIBRARIES(${lib_name} ${libs})

FOREACH(c ${VTK_LIBRARIES})
  SET(tcl_wrap)
  IF (VTK_WRAP_TCL)
    SET(tcl_wrap TCL)
  ENDIF()
  TARGET_LINK_LIBRARIES(${lib_name} ${c}${tcl_wrap})
ENDFOREACH(c)

# Apply user-defined properties to the library target.
IF(Slicer3_LIBRARY_PROPERTIES)
  SET_TARGET_PROPERTIES(${lib_name} PROPERTIES
     ${Slicer3_LIBRARY_PROPERTIES}
  )
ENDIF(Slicer3_LIBRARY_PROPERTIES)

install(TARGETS ${lib_name}
  RUNTIME DESTINATION bin COMPONENT RuntimeLibraries 
  LIBRARY DESTINATION lib/${PROJECT_NAME} COMPONENT RuntimeLibraries
  ARCHIVE DESTINATION lib/${PROJECT_NAME} COMPONENT Development
  )

# --------------------------------------------------------------------------
# Create Python wrappers if VTK is wrapped in Python
#

IF(VTK_WRAP_PYTHON AND BUILD_SHARED_LIBS)

  SET(KIT_PYTHON_LIBRARIES ${VTK_LIBRARIES})

  INCLUDE(${VTK_CMAKE_DIR}/vtkWrapPython.cmake)
  VTK_WRAP_PYTHON3(${lib_name}Python PYTHON_SRCS "${FreeSurfer_SRCS}")
  ADD_LIBRARY(${lib_name}PythonD ${PYTHON_SRCS})
  ADD_LIBRARY(${lib_name}Python MODULE ${lib_name}PythonInit.cxx)
  
  TARGET_LINK_LIBRARIES(${lib_name}PythonD ${lib_name})
  
  FOREACH(c ${KIT_PYTHON_LIBRARIES})
    TARGET_LINK_LIBRARIES(${lib_name}PythonD ${c}PythonD)
  ENDFOREACH(c)

  TARGET_LINK_LIBRARIES(${lib_name}Python ${lib_name}PythonD)

  ## Python modules on Windows must have the extension ".pyd"
  IF(WIN32 AND NOT CYGWIN)
    SET_TARGET_PROPERTIES(${lib_name}Python PROPERTIES SUFFIX ".pyd")
  ENDIF(WIN32 AND NOT CYGWIN)

  INSTALL(TARGETS ${lib_name}PythonD ${lib_name}Python
    RUNTIME DESTINATION bin COMPONENT RuntimeLibraries 
    LIBRARY DESTINATION lib/${PROJECT_NAME} COMPONENT RuntimeLibraries
    ARCHIVE DESTINATION lib/${PROJECT_NAME} COMPONENT Development)

ENDIF(VTK_WRAP_PYTHON AND BUILD_SHARED_LIBS)

# --------------------------------------------------------------------------
# Install support files

# Should this location change, update Libs\MRML\vtkMRMLFreeSurferProceduralColorNode.cxx

# Copy the resource files to the build directory

set(RESFILES
  FreeSurferColorLUT.txt
  Simple_surface_labels2002.txt
  )
foreach(file ${RESFILES})
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/${file}
    ${CMAKE_BINARY_DIR}/share/${PROJECT_NAME}/${file}
    COPYONLY
    )
endforeach(file)
install(
  FILES ${RESFILES}
  DESTINATION share/${PROJECT_NAME}
  )

# --------------------------------------------------------------------------
# Testing (requires some of the examples)

# IF(BUILD_TESTING)
#  ADD_SUBDIRECTORY(Testing)
# ENDIF(BUILD_TESTING)

# Install the testing data

file(GLOB_RECURSE PNGFILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "Testing/*.png")
file(GLOB_RECURSE TCLFILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "Testing/*.tcl")
file(GLOB_RECURSE CURVFILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "Testing/*.curv")
file(GLOB_RECURSE ORIGFILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "Testing/*.orig")

foreach(file 
    ${PNGFILES}
    ${TCLFILES}
    ${CURVFILES}
    ${ORIGFILES}
    )
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/${file}
    ${CMAKE_BINARY_DIR}/share/${PROJECT_NAME}/${file}
    COPYONLY
    )
endforeach(file) 

install(DIRECTORY
  Testing
  DESTINATION share/${PROJECT_NAME} COMPONENT Development
  PATTERN ".svn" EXCLUDE
  )
