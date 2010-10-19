This document describes building the Mac OS X version of Scan Tailor.

This requires that you have installed Apple's developer tools.  These are
an optional install on the installation disks that come with your computer,
or can be downloaded (free registration may be required) Apple's web site
at http://developer.apple.com/mac


                                Downloading Prerequisites

First, download the following software.  Unless stated otherwise, take the
latest stable version.

1. CMake 2.8.x (tested with 2.8.2)
   Homepage: http://www.cmake.org
   I recommend just downloading the binary installer so you can just download,
   click, have it all installed and ready to go.  Be sure to have it install
   the command-line utilities.
2. Qt4 - Tested with Qt4.7beta2 - some odd problems when using 4.6 stable
   Homepage: http://qt.nokia.com
   From there: Download -> LGPL / Free -> Download Qt Carbon for Mac OS X
   I recommend just downloading the binary installer for Carbon.  Since we are
   building with dependencies for Tiger (10.4) and forward, we cannot build
   a fully 64-bit executable anyway.  The Qt Carbon build is ppc/i386 only.
3. jpeg-8b
   Homepage: http://www.ijg.org/
   The file we need will be named jpegsrc.v8b.tar.gz or similarly.
4. libpng (tested with v1.4.3)
   Homepage: http://www.libpng.org/pub/png/libpng.html
   We need a file named like libpng-x.x.x.tar.gz, where x.x.x represents
   the version number.
5. libtiff (tested with 3.9.4)
   Homepage: http://www.remotesensing.org/libtiff/
   The file should be named like tiff-3.9.4.tar.gz 
6. Boost (tested with 1.43.0)
   Homepage: http://boost.org/
   For Boost, I recommend the file named like boost_1_43_0.tar.gz.



                                    Instructions

1. Create a build directory.  Its full path should have no spaces. This can be
   in your home directory or even on your desktop.

2. Unpack jpeg-8b, libpng, libtiff, boost, and scantailor itself to this
   build directory.  You should get a directory structure like
   this:

   build
     | boost_1_43_0
     | jpeg-8b
     | libpng-1.4.3
     | scantailor-0.9.9
     | tiff-3.9.4

3. Two more subdirectories will be created there:
     | scantailor-build
     | scantailor-deps-build

4. Install Qt

5. Install CMake.

6. You can either locate the "buildscantailor.sh" file in the Finder
   (It is in scantailor/packaging/osx/ in the folder you created above)
   and right-click (or control-click) on it to "Open With...." Terminal.app,
   or you can open Terminal.app directly and run it from there.

   This will build all required dependencies and scantailor itself. It will
   only rebuild the parts needed each time, so once a particular piece is
   built you will not have to wait for it to be built again.
    
   Make sure the building process finishes without errors.  Warnings may
   be ignored.
   
   If everything went right, the application exists in the scantailor-build
   folder named ScanTailor.app with a distributable disk image in the same
   folder named ScanTailor-VERSION.dmg. The VERSION part of the name will
   be replaced by the actual version, taken from a file called "VERSION" in
   the root of the source directory.

