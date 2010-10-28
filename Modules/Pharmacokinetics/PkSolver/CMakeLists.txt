Project(PkSolver)

ADD_LIBRARY( PkSolver PkSolver.cxx )
slicer3_set_modules_output_path(PkSolver)

TARGET_LINK_LIBRARIES( PkSolver ITKBasicFilters ITKCommon ITKIO ITKNumerics )

SUBDIRS ( exec )

install(TARGETS PkSolver
  RUNTIME DESTINATION ${Slicer3_INSTALL_MODULES_BIN_DIR} COMPONENT RuntimeLibraries 
  LIBRARY DESTINATION ${Slicer3_INSTALL_MODULES_LIB_DIR} COMPONENT RuntimeLibraries
  ARCHIVE DESTINATION ${Slicer3_INSTALL_MODULES_LIB_DIR} COMPONENT Development
  )

slicer3_install_modules(PkSolver)

