This document describes building the Windows version of Scan Tailor.

Earlier versions of Scan Tailor supported both Visual Studio and MinGW
compilers. MinGW support was dropped at some point, in order to reduce
maintenance effort. Furthermore, Scan Tailor started to use some C++11
features, making Visual Studio versions below 2012 not supported.


                                Downloading Prerequisites

First, download the following software.  Unless stated otherwise, take the
latest stable version.

1. Visual Studio Express 2012 for Windows Desktop.
   Homepage: http://www.microsoft.com/visualstudio/eng/products/visual-studio-express-products
2. CMake >= 2.8.9
   Homepage: http://www.cmake.org
3. jpeg library
   Homepage: http://www.ijg.org/
   The file we need will be named jpegsrc.v9.tar.gz or similarly.
4. zlib
   Homepage: http://www.zlib.net/
   We need a file named like zlib-x.x.x.tar.gz, where x.x.x represents
   the version number.
5. libpng
   Homepage: http://www.libpng.org/pub/png/libpng.html
   We need a file named like libpng-x.x.x.tar.gz, where x.x.x represents
   the version number.
6. libtiff
   Homepage: http://www.remotesensing.org/libtiff/
   Because libtiff is updated rarely, but vulnerabilities in it are found often,
   it's better to patch it right away.  In that case, take it from here:
   http://packages.debian.org/source/sid/tiff
   There you will find both the original libtiff and a set of patches for it.
   The process of patching libtiff is described later in this document.
   If you aren't going to distribute your Scan Tailor build and aren't going
   to open files from untrusted sources, then you don't really need patching it.
7. Qt 5.x.x (tested with 5.0.2)
   Homepage: http://qt-project.org/
   Either a source-only or any of the binary versions will do. In either case
   a custom build of Qt will be made, though a binary version will result in
   less things to build.
8. ActivePerl (necessary to build Qt5)
   Homepage: http://www.activestate.com/activeperl/downloads
   A 32-bit version is suggested. Whether or not 64-bit version would work
   is unclear. When installing make sure that "Add Perl to the PATH environment
   variable" option is set.
9. Boost (tested with 1.53.0)
   Homepage: http://boost.org/
   You can download it in any file format, provided you know how to unpack it.
10. NSIS 2.x (tested with 2.46)
   Homepage: http://nsis.sourceforge.net/


                                    Instructions

1. Create a build directory. Its full path should have no spaces. From now on
   this document will be assuming the build directory is C:\build

2. Unpack jpeg, libpng, libtiff, zlib, boost, boost jam, and scantailor
   itself to the build directory.  You should get a directory structure like
   this:
   C:\build
     | boost_1_53_0
     | jpeg-9
     | libpng-1.6.2
     | scantailor-0.10.0
     | tiff-4.0.2
     | zlib-1.2.8
   
   If you went for a source-only version of Qt, unpack it here as well.
   Otherwise, install Qt into whatever directory its installer suggests.
   IMPORTANT: Tell the installer to install Source Components as well.
   
   If you don't know how to unpack .tar.gz files, try this tool:
   http://www.7-zip.org/

3. Install Visual Studio, ActivePerl and CMake.

4. Create two more subdirectories there:
     | scantailor-build
     | scantailor-deps-build

5. Launch CMake and specify the following:

   Where is the source code: C:\build\scantailor-0.10.0\packaging\windows\build-deps
   Where to build the binaries: C:\build\scantailor-deps-build

   Click "Configure". Select the project type "Visual Studio 11" or
   "Visual Studio 11 Win64" for 64-bit builds. Keep in mind that 64-bit
   builds are only possible on a 64-bit version of Windows. Visual Studio 11
   is the same as Visual Studio 2012. If any of the paths weren't found,
   enter them manually, then click "Configure" again. If everything went right,
   the "Generate" button will become clickable. Click it. Sometimes it's
   necessary to click "Configure" more than once before "Generate" becomes
   clickable.

6. We will be building Scan Tailor's dependencies here.  This step is the
   longest one (may take a few hours), but fortunately it only has to be done
   once.  When building newer versions of Scan Tailor, you won't need to
   redo this step.
   
   Go to C:\build\scantailor-deps-build and open file
   "Scan Tailor Dependencies.sln".  It will open in Visual Studio.
   IMPORTANT: Set the build type to RelWithDebInfo. If you leave Debug
   (which is the default), your builds won't run on other computers.
   Even with build type set to RelWithDebInfo, some libraries (Qt, boost)
   will be built in both debug and release configurations. When building
   Scan Tailor itself, the appropriate library configuration will be
   selected automatically.
   
   Now do Build -> Build Solution.
   
   Make sure the building process finishes without errors. Warnings may
   be ignored.

7. Launch CMake again and specify following:

   Where is the source code: C:\build\scantailor-0.10.0
   Where to build the binaries: C:\build\scantailor-build

   Click "Configure", then "Generate", just like on step 5.

8. Now we are going to build Scan Tailor itself.  On subsequent build of the
   same (possiblity modified) version, you can start right from this step.
   For building a different version, start from step 7.
   
   Go to C:\build\scantailor-build and open file "Scan Tailor.sln".
   It will open in Visual Studio. Select the desired build type. Debug builds
   won't work on other computers.
   
   Now do Build -> Build Solution
   
   Make sure the building process finishes without errors. Warnings may
   be ignored.

   If everything went right, the installer named scantailor-VERSION-install.exe
   will appear in C:\build\scantailor-build. The VERSION part of the name will
   be replaced by the actual version, taken from the version.h file in the root
   of the source directory.


                               Patching libtiff

These instructions assume you've got Debian's patches for libtiff from:
http://packages.debian.org/source/sid/tiff
There you will find both the original libtiff sources (filename like
tiff_4.0.2orig.tar.gz) and a patch set for it (filename like
tiff_4.0.2-6.debian.tar.gz).  Download both and follow the instructions:

1. Get the command line patch utility from here:
   http://gnuwin32.sourceforge.net/packages/patch.htm

   Better use the version with the installer.  In that case CMake will
   find the location of patch.exe by itself.

2. Extract the original libtiff sources into C:\build to get a directory
   structure like this:
   C:\build
     | tiff-4.0.2
     +-- build
       | config
       | contrib
       | ...

   Then extract the patchset inside the tiff directory, to get the "debian"
   directory on the same level as "build", "config" and "contrib".

3. Create another subdirectory under C:\build
   Call it "tiff-patch-dir".

4. Launch CMake and specify the following:

   Where is the source code:    C:\build\scantailor-0.10.0\packaging\windows\patch_libtiff
   Where to build the binaries: C:\build\tiff-patch-dir

   Click "Configure" then "Generate", just like in previous section, step 5.

5. Go to C:\build\tiff-patch-dir and open file "patch_libtiff.sln".
   It will open in Visual Studio. The build type doesn't matter in this case.
   
   Now do Build -> Build Solution.

   If no errors were reported, you have successfully patched your libtiff.
   If you ever need to patch it again, first revert it to the original version,
   the one from the .tar.gz file and delete the "debian" subdirectory.
