This document describes building the Windows version of Scan Tailor.

First, download the following software.  Unless stated otherwise, take the
latest stable version.

1. CMake 2.6.x (tested with 2.6.2)
   Homepage: http://www.cmake.org
2. jpeg-6b
   Homepage: http://www.ijg.org/
3. zlib
   Homepage: http://www.zlib.net/
4. libpng
   Homepage: http://www.libpng.org/pub/png/libpng.html
5. libtiff
   Homepage: http://www.remotesensing.org/libtiff/
   Because libtiff is updated rarely, but vulnerabilities in it are found often,
   it's better to patch it right away.  In that case, take it from here:
   http://packages.debian.org/source/sid/tiff
   There you will find both the original libtiff and a patch for it.
   The process of applying this patch is described later in this document.
   If you aren't going to distribute your Scan Tailor build and aren't going
   to open files from untrusted sources, then you don't really need patching it.
6. Qt 4.x.x (tested with 4.4.2)
   Homepage: http://trolltech.com/
   Download Qt -> Open Source -> Application Development -> Qt for Windows: C++
   From there download the mingw version rather than the source-only version.
   By doing so you will get the correct version of MinGW installed, and you'll
   save some time compiling it.  Note that you will have to rebuild Qt libraries
   anyway, but at least you won't have to rebuild the programs that come with
   Qt.  Still, the source-only Qt plus a manual MinGW installation is supported,
   but this document assumes you are using the mingw version.
7. Boost (tested with 1.38.0) and Boost Jam (tested with 3.1.17)
   Homepage: http://boost.org/
   For Boost, you can download any of the files, provided you know how to unpack it.
   For Boost Jam, take the file with "ntx86" suffix.
8. NSIS 2.x (tested with 2.42)
   Homepage: http://nsis.sourceforge.net/



                                    Instructions

1. Create a build directory.  Its full path should have no spaces.  I suggest
   C:\build

2. Unpack jpeg-6b, libpng, libtiff, zlib, boost, boost jam, and scantailor
   itself to the build directory.  You should get a directory structure like
   this:
   C:\build
     | boost_1_38_0
     | boost-jam-3.1.17-1-ntx86
     | jpeg-6b
     | libpng-1.2.31
     | scantailor-0.9.0
     | tiff-3.8.2
     | zlib-1.2.3

   If you don't know how to unpack .tar.gz files, I suggest this tool:
   http://www.7-zip.org/

3. Create two more subdirectories there:
     | scantailor-build
     | scantailor-deps-build

4. Install Qt and tell it to download and install MinGW.

5. Install CMake.

6. Now we need to make sure CMake sees the MinGW bin directory in PATH.
   There are two alternative ways to achieve that:
   1.  Go to Control Panel (-> Performance and Maintenance) -> System
       -> Advanced -> Environment Variables and add something like
       ";C:\MinGW\bin" (without quotes) to the end of the Path variable.
   2.  You may just launch CMake from the Qt Command Prompt, which takes
       care about adjusting the PATH variable.  Unfortunately, in this case
       you will have to write the full path to the CMake executable, like:
       C:\Program Files\CMake 2.6\bin\CMakeSetup.exe
       Using just CMakeSetup.exe won't work even if you've chosen to add
       CMake to PATH when installing it.  That's because Qt Command Prompt
       Overwrites the PATH variable completely.  Of course you can edit
       the qtvars.bat file and put the CMake directory to PATH there.

6. Launch CMake and specify the following:

   Source directory: C:\build\scantailor-0.9.0\packaging\windows\build-deps
   Binary directory: C:\build\scantailor-deps-build

   Click "Configure".  Select the project type "MinGW Makefiles".  If any
   paths were not found, enter them manually, then click "Configure" again.
   If everything went right, the "Generate" button will become clickable.
   Click it.  Sometimes it's necessary to click "Configure" more than once
   before "Generate" becomes clickable.

7. Launch the Qt Command Promt from the Start menu.
   Enter the following there:

   C:
   cd C:\build\scantailor-deps-build
   mingw32-make 2> log.txt

   The "2> log.txt" part will write errors to a file rather than to the command
   prompt window.  That's useful for figuring out what went wrong.

   The last step will take a lot of time - a couple of hours or even more.
   Note that it will ask you to accept the Qt license after a few minutes.

   When this step completes, check the log.txt file to make sure nothing
   went wrong.  You would have errors at the end of that file in that case.
   Warnings may be ignored.

8. Launch CMake again and specify following:

   Source directory: C:\build\scantailor-0.9.0
   Binary directory: C:\build\scantailor-build

   Click "Configure".  Select the project type "MinGW Makefiles".  If any
   paths were not found, enter them manually, then click "Configure" again.
   If everything went right, the "Generate" button will become clickable.
   Click it.  Sometimes it's necessary to click "Configure" more than once
   before "Generate" becomes clickable.

9. Back to the Qt Command Promt, give the following commands:

   C:
   cd C:\build\scantailor-build
   mingw32-make 2> log.txt

   If everything went right, the installer named scantailor-VERSION-install.exe
   will appear in the current directory.  The VERSION part of the name will
   be replaced by the actual version, taken from a file called "VERSION" in
   the root of the source directory.


                               Patching libtiff

These instructions assume you've got the patch file from Debian:
http://packages.debian.org/source/sid/tiff
There you will find both the original libtiff sources and a separate patch
in .diff.gz format.  Note that debian packages the original source archive
inside another archive.  Extract the inner one and uncompress it to the
build directory (C:\build).  The patch they distribute is not a simple one.
It brings more patches that need to be applied in a specific order.
Fortunately, we have a CMake script that simplifies things a lot.

Here are the instructions:

1. Get the command line patch utility from here:
   http://gnuwin32.sourceforge.net/packages/patch.htm

   Better use the version with the installer.  In that case CMake will
   find the location of patch.exe by itself.

2. Download and uncompress the patch.  Uncompressed it will have the .diff
   extension.  Better copy it to the build directory (C:\build) to allow CMake
   to find it automatically.

3. Create another subdirectory under C:\build
   Call it "tiff-patch-dir".

4. Launch CMake and specify the following:

   Source directory: C:\build\scantailor-0.9.0\packaging\windows\patch_libtiff
   Binary directory: C:\build\tiff-patch-dir

  Click "Configure".  Select the project type "MinGW Makefiles".  If any
   paths were not found, enter them manually, then click "Configure" again.
   If everything went right, the "Generate" button will become clickable.
   Click it.  Sometimes it's necessary to click "Configure" more than once
   before "Generate" becomes clickable.

5. Do the following from the Qt Command Prompt:

   C:
   cd C:\build
   cd tiff-patch-dir
   mingw32-make

   If no errors were reported, you have successfully patched your libtiff.
   If you ever need to patch it again, first revert it to the original version,
   the one from the .tar.gz file.