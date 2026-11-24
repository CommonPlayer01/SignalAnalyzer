# CMake generated Testfile for 
# Source directory: C:/Users/work/Desktop/qt-demo-cmake/SignalAnalyzer-cmake/SignalCoreLib
# Build directory: C:/Users/work/Desktop/qt-demo-cmake/SignalAnalyzer-cmake/build-vs142/SignalCoreLib
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(SignalCoreLibSmokeTest "C:/Users/work/Desktop/qt-demo-cmake/SignalAnalyzer-cmake/build-vs142/bin/Debug/SignalCoreLibSmokeTest.exe")
  set_tests_properties(SignalCoreLibSmokeTest PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/work/Desktop/qt-demo-cmake/SignalAnalyzer-cmake/SignalCoreLib/CMakeLists.txt;89;add_test;C:/Users/work/Desktop/qt-demo-cmake/SignalAnalyzer-cmake/SignalCoreLib/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(SignalCoreLibSmokeTest "C:/Users/work/Desktop/qt-demo-cmake/SignalAnalyzer-cmake/build-vs142/bin/Release/SignalCoreLibSmokeTest.exe")
  set_tests_properties(SignalCoreLibSmokeTest PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/work/Desktop/qt-demo-cmake/SignalAnalyzer-cmake/SignalCoreLib/CMakeLists.txt;89;add_test;C:/Users/work/Desktop/qt-demo-cmake/SignalAnalyzer-cmake/SignalCoreLib/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(SignalCoreLibSmokeTest "C:/Users/work/Desktop/qt-demo-cmake/SignalAnalyzer-cmake/build-vs142/bin/MinSizeRel/SignalCoreLibSmokeTest.exe")
  set_tests_properties(SignalCoreLibSmokeTest PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/work/Desktop/qt-demo-cmake/SignalAnalyzer-cmake/SignalCoreLib/CMakeLists.txt;89;add_test;C:/Users/work/Desktop/qt-demo-cmake/SignalAnalyzer-cmake/SignalCoreLib/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(SignalCoreLibSmokeTest "C:/Users/work/Desktop/qt-demo-cmake/SignalAnalyzer-cmake/build-vs142/bin/RelWithDebInfo/SignalCoreLibSmokeTest.exe")
  set_tests_properties(SignalCoreLibSmokeTest PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/work/Desktop/qt-demo-cmake/SignalAnalyzer-cmake/SignalCoreLib/CMakeLists.txt;89;add_test;C:/Users/work/Desktop/qt-demo-cmake/SignalAnalyzer-cmake/SignalCoreLib/CMakeLists.txt;0;")
else()
  add_test(SignalCoreLibSmokeTest NOT_AVAILABLE)
endif()
