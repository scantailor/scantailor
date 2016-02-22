# Install script for directory: /Users/pod/scantailor

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/Users/pod/scantailor/scantailor")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/scantailor" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/scantailor")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/usr/local/Cellar/qt/4.8.7_2/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/scantailor")
  endif()
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/Users/pod/scantailor/scantailor-cli")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/scantailor-cli" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/scantailor-cli")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/usr/local/Cellar/qt/4.8.7_2/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/scantailor-cli")
  endif()
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/scantailor/translations" TYPE FILE FILES
    "/Users/pod/scantailor/scantailor_bg.qm"
    "/Users/pod/scantailor/scantailor_cs.qm"
    "/Users/pod/scantailor/scantailor_de.qm"
    "/Users/pod/scantailor/scantailor_es.qm"
    "/Users/pod/scantailor/scantailor_fr.qm"
    "/Users/pod/scantailor/scantailor_hr.qm"
    "/Users/pod/scantailor/scantailor_it.qm"
    "/Users/pod/scantailor/scantailor_ja.qm"
    "/Users/pod/scantailor/scantailor_pl.qm"
    "/Users/pod/scantailor/scantailor_pt_BR.qm"
    "/Users/pod/scantailor/scantailor_ru.qm"
    "/Users/pod/scantailor/scantailor_sk.qm"
    "/Users/pod/scantailor/scantailor_uk.qm"
    "/Users/pod/scantailor/scantailor_zh_CN.qm"
    "/Users/pod/scantailor/scantailor_zh_TW.qm"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/Users/pod/scantailor/crash_reporter/cmake_install.cmake")
  include("/Users/pod/scantailor/dewarping/cmake_install.cmake")
  include("/Users/pod/scantailor/foundation/cmake_install.cmake")
  include("/Users/pod/scantailor/math/cmake_install.cmake")
  include("/Users/pod/scantailor/imageproc/cmake_install.cmake")
  include("/Users/pod/scantailor/interaction/cmake_install.cmake")
  include("/Users/pod/scantailor/zones/cmake_install.cmake")
  include("/Users/pod/scantailor/tests/cmake_install.cmake")
  include("/Users/pod/scantailor/ui/cmake_install.cmake")
  include("/Users/pod/scantailor/filters/fix_orientation/cmake_install.cmake")
  include("/Users/pod/scantailor/filters/page_split/cmake_install.cmake")
  include("/Users/pod/scantailor/filters/deskew/cmake_install.cmake")
  include("/Users/pod/scantailor/filters/select_content/cmake_install.cmake")
  include("/Users/pod/scantailor/filters/page_layout/cmake_install.cmake")
  include("/Users/pod/scantailor/filters/output/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/Users/pod/scantailor/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
