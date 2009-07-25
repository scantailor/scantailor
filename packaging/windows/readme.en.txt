This document describes building the Windows version of Scan Tailor.

Both Visual C++ (part of Visual Studio) and MinGW compilers are supported.
Theoretically, any version of Visual C++ starting from Visual C++ 2003 should
work.  In practice, Visual C++ 2008 Express Edition is known to work and no
others were tested.  As for MinGW, the version shipped with Qt is known
to work.
The official builds are built with Visual C++ with the main reason being
the built-in crash reporter only supporting Visual C++ under Windows.
For unofficial builds the crash reporter is useless and should not be enabled,
because only the person who built the particular version will have symbolic
data used to transform the crash report to a human readable form.
Generally, it's easier to build Scan Tailor using MinGW, but it's easier to
develop it using Visual C++.

Initially this document was only covering MinGW, while bits related to
Visual C++ were added later and marked with [VC++].


                                Downloading Prerequisites

First, download the following software.  Unless stated otherwise, take the
latest stable version.

1. CMake 2.6.x (tested with 2.6.2)
   Homepage: http://www.cmake.org
2. jpeg-6b
   Homepage: http://www.ijg.org/
   The file we need will be named jpegsrc.v6b.tar.gz or similarly.
3. zlib
   Homepage: http://www.zlib.net/
   We need a file named like zlib-x.x.x.tar.gz, where x.x.x represents
   the version number.
4. libpng
   Homepage: http://www.libpng.org/pub/png/libpng.html
   We need a file named like libpng-x.x.x.tar.gz, where x.x.x represents
   the version number.
5. libtiff
   Homepage: http://www.remotesensing.org/libtiff/
   Because libtiff is updated rarely, but vulnerabilities in it are found often,
   it's better to patch it right away.  In that case, take it from here:
   http://packages.debian.org/source/sid/tiff
   There you will find both the original libtiff and a patch for it.
   The process of applying this patch is described later in this document.
   If you aren't going to distribute your Scan Tailor build and aren't going
   to open files from untrusted sources, then you don't really need patching it.
6. Qt 4.x.x (tested with 4.5.0)
   Homepage: http://trolltech.com/
   Download Qt -> Open Source -> Application Development -> Qt for Windows: C++
   From there download the mingw version rather than the source-only version.
   By doing so you will get the correct version of MinGW installed, and you'll
   save some time compiling it.  Note that you will have to rebuild Qt libraries
   anyway, but at least you won't have to rebuild the programs that come with
   Qt.  Still, the source-only Qt is supported, though in that case you would
   have to download and install MinGW manually.
   [VC++]
   You would need the Visual C++ itself, which you can download from here:
   http://www.microsoft.com/Express/vc/
   You can take either source or the binary versions of Qt, including the one
   for MinGW one, which should still save you some compiling time.  The build
   was tested with the source version of Qt.
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
   
   If you took a Qt version without an installer, unpack it here as well.
   
   If you don't know how to unpack .tar.gz files, I suggest this tool:
   http://www.7-zip.org/

3. Create two more subdirectories there:
     | scantailor-build
     | scantailor-deps-build

4. If you took Qt with installer, install it now, telling it to download
   and install MinGW as well.  Otherwise, download the MinGW installer from
   http://mingw.sourceforge.net/ and install MinGW.

5. Install CMake.

6. [VC++] Just skip this step.
   
   Now we need to make sure CMake sees the MinGW bin directory in PATH.
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
   [VC++]
   Select your version of Visual C++ (Visual Studio) instead of "MinGW Makefiles"

7. We will be building Scan Tailor's dependencies here.  This step is the
   longest one (may take a few hours), but fortunately it only has to be done
   once.  When building newer versions of Scan Tailor, you won't need to
   redo this step.
   
   [VC++]
   Go to C:\build\scantailor-deps-build and open file
   "Scan Tailor Dependencies.sln".  It will open in Visual Studio.  Select
   The desired build type (Release, Debug, MinSizeRel, RelWithDebInfo)
   and do Build -> Build Solution.  If you don't know which build type
   to choose, go with Release.
   
   Make sure the building process finishes without errors.  Warnings may
   be ignored.
   
   [MinGW]
   Launch the Qt Command Promt from the Start menu.
   Enter the following there:

   C:
   cd C:\build\scantailor-deps-build
   mingw32-make 2> log.txt

   The "2> log.txt" part will write errors to a file rather than to the command
   prompt window.  That's useful for figuring out what went wrong.

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
   [VC++]
   Select your version of Visual C++ (Visual Studio) instead of "MinGW Makefiles"

9. Now we are going to build Scan Tailor itself.  On subsequent build of the
   same (possiblity modified) version, you can start right from this step.
   For building a different version, start from step 8.
   
   [VC++]
   Go to C:\build\scantailor-build and open file "Scan Tailor.sln".
   It will open in Visual Studio.  Select the same build type as on step 7,
   then do Build -> Build Solution.
   
   [MinGW]
   Back to the Qt Command Promt, give the following commands:

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
   [VC++]
   Select your version of Visual C++ (Visual Studio) instead of "MinGW Makefiles"

5. [VC++]
   Go to C:\build\tiff-patch-dir and open file "patch_libtiff.sln".
   It will open in Visual Studio.  From there, do Build -> Build Solution.

   [MinGW]
   Do the following from the Qt Command Prompt:

   C:
   cd C:\build
   cd tiff-patch-dir
   mingw32-make

   If no errors were reported, you have successfully patched your libtiff.
   If you ever need to patch it again, first revert it to the original version,
   the one from the .tar.gz file.