SET(KIT FastMarching)
SET(CMAKE_TESTDRIVER_BEFORE_TESTMAIN "DEBUG_LEAKS_ENABLE_EXIT_ERROR();" )
CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx
  vtkPichonFastMarchingTest.cxx
  EXTRA_INCLUDE TestingMacros.h
  )

SET (TestsToRun ${Tests})
REMOVE (TestsToRun ${KIT}CxxTests.cxx)

SET(LIBRARY_NAME ${PROJECT_NAME})

# --------------------------------------------------------------------------
# Testing

SET(SHORT_TEST_TIMEOUT 300)  ## Sometimes when the GUI crashes, it won't exit properly, and then the tests have to wait until they reach 1500 seconds before they time out

INCLUDE_DIRECTORIES(${Slicer3_SOURCE_DIR}/Base/Logic)
ADD_EXECUTABLE(${KIT}CxxTests ${Tests})
TARGET_LINK_LIBRARIES(${KIT}CxxTests 
  ${lib_name} 
  SlicerBaseLogic
  )

SET( BaseGUI_TESTS ${CXX_TEST_PATH}/${KIT}CxxTests)

IF(WIN32)
  SET(BaseGUI_TESTS ${CXX_TEST_PATH}/${CMAKE_BUILD_TYPE}/${KIT}CxxTests)
ENDIF(WIN32)

MACRO( SIMPLE_TEST  TESTNAME )
  ADD_TEST( ${TESTNAME} ${LAUNCH_EXE} ${BaseGUI_TESTS} ${TESTNAME} 
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )
ENDMACRO( SIMPLE_TEST  )

#ADD_EXECUTABLE(vtkPichonFastMarchingTest vtkPichonFastMarchingTest.cxx)
#TARGET_LINK_LIBRARIES(vtkPichonFastMarchingTest vtkIO vtkGraphics SlicerBaseLogic)

SIMPLE_TEST(vtkPichonFastMarchingTest)
