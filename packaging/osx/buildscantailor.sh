#!/bin/bash

OURDIR=`dirname $0`
OURDIR=`cd $OURDIR; pwd`
STSRC=`cd $OURDIR/../..; pwd`
STHOME=`cd $OURDIR/../../..; pwd`
echo -e "Building ScanTailor - Base Direcotry: $STHOME\n\n"

export BUILDDIR=$STHOME/scantailor-deps-build
export STBUILDDIR=$STHOME/scantailor-build
export CMAKE_PREFIX_PATH=$BUILDDIR
export CMAKE_INCLUDE_PATH=$BUILDDIR
export CMAKE_LIBRARY_PATH=$BUILDDIR
export CMAKE_INSTALL_PREFIX=$BUILDDIR
mkdir -p $BUILDDIR

# For Tiger (10.4), we must use gcc-4.0
export CC=gcc-4.0
export CPP=cpp-4.0
export CXX=g++-4.0
export LD=g++-4.0

export MAC_OS_X_VERSION_MIN_REQUIRED=10.4
export MACOSX_DEPLOYMENT_TARGET=10.4
export MYCFLAGS="-m32 -arch i386 -arch x86_64 -arch ppc -mmacosx-version-min=10.4"
export MYLDFLAGS="-m32 -isysroot /Developer/SDKs/MacOSX10.4u.sdk"


function build_lib {
  libname=$1
  libdir=$2
 
  LIB_DYLIB=`find $BUILDDIR -iname $1`
  if [ "x$LIB_DYLIB" == "x" ]
  then
    echo "Library $1 not built, trying to build..."
    LIB_DIR=`find $STHOME -type d -maxdepth 1 -iname "$2"`
    if [ "x$LIB_DIR" == "x" ]
    then
      echo "Cannot find library source code.  Check in $STHOME for $2 directory."
      exit 0
    fi
    cd $LIB_DIR
    # make clean
    if [ ! -r Makefile ]
    then
      env CFLAGS="$MYCFLAGS" CXXFLAGS="$MYCFLAGS" LDFLAGS="$MYLDFLAGS" ./configure --prefix=$BUILDDIR --disable-dependency-tracking --enable-shared --enable-static
    fi
    make
    make install
    cd $STHOME
  fi
  return 0
}

# Check for and build dependency libraries if needed
build_lib "libtiff.dylib" "tiff-[0-9]*.[0-9]*.[0-9]*"
build_lib "libjpeg.dylib" "jpeg-[0-9]*"
build_lib "libpng.dylib" "libpng-[0-9]*.[0-9]*.[0-9]*"

# BOOST
#curl -L -o  boost_1_43_0.tar.gz "http://sourceforge.net/projects/boost/files/boost/1.43.0/boost_1_43_0.tar.gz/download"
#tar xzvvf boost_1_43_0.tar.gz
export BOOST_DIR=`find $STHOME -type d -iname boost_[0-9]*_[0-9]*_[0-9]*`
if [ "x$BOOST_DIR" == "x" ]
then
   echo "Cannot find BOOST libraries.  Check in $STHOME for boost_x_x_x directory."
   exit 0
fi
BOOST_LIB1=`find $BUILDDIR/lib -iname libboost_signals.a`
BOOST_LIB2=`find $BUILDDIR/lib -iname libboost_system.a`
BOOST_LIB3=`find $BUILDDIR/lib -iname libboost_prg_exec_monitor.a`
BOOST_LIB4=`find $BUILDDIR/lib -iname libboost_test_exec_monitor.a`
BOOST_LIB5=`find $BUILDDIR/lib -iname libboost_unit_test_framework.a`
if [ "x$BOOST_LIB1" == "x" ] || \
   [ "x$BOOST_LIB2" == "x" ] || \
   [ "x$BOOST_LIB3" == "x" ] || \
   [ "x$BOOST_LIB4" == "x" ] || \
   [ "x$BOOST_LIB5" == "x" ]
then
  cd $BOOST_DIR
  [ ! -x ./bjam ] && ./bootstrap.sh --prefix=$BUILDDIR --with-libraries=test,system,signals
  echo ./bjam --toolset=darwin-4.0 --prefix=$BUILDDIR --user-config=$OURDIR/user-config.jam --build-dir=$BUILDDIR --with-test --with-system --with-signals link=static runtime-link=static architecture=combined address-model=32 macosx-version=10.4 macosx-version-min=10.4 --debug-configuration install
  ./bjam --toolset=darwin-4.0 --prefix=$BUILDDIR --user-config=$OURDIR/user-config.jam --build-dir=$BUILDDIR --with-test --with-system --with-signals link=static runtime-link=static architecture=combined address-model=32 macosx-version=10.4 macosx-version-min=10.4 --debug-configuration install
fi
export BOOST_ROOT=$BUILDDIR
cd $STHOME

# SCANTAILOR
cd $STSRC
# make clean
# rm CMakeCache.txt
# needed in case scantailor source is not updated to compile with new boost (>=1_34) test infrastructure
[ ! -f $STSRC/imageproc/tests/main.cpp.old ] && sed -i '.old' -e '1,$ s%^#include <boost/test/auto_unit_test\.hpp>%#include <boost/test/included/unit_test.hpp>%g' $STSRC/imageproc/tests/main.cpp
[ ! -f $STSRC/tests/main.cpp.old ] && sed -i '.old' -e '1,$ s%^#include <boost/test/auto_unit_test\.hpp>%#include <boost/test/included/unit_test.hpp>%g' $STSRC/tests/main.cpp
[ ! -f CMakeCache.txt ] && cmake -DCMAKE_FIND_FRAMEWORK=LAST -DCMAKE_OSX_ARCHITECTURES="ppc;i386" -DCMAKE_OSX_DEPLOYMENT_TARGET="10.4" -DCMAKE_OSX_SYSROOT="/Developer/SDKs/MacOSX10.4u.sdk" -DPNG_INCLUDE_DIR=$BUILDDIR .
make
$OURDIR/makeapp.sh $STBUILDDIR $STSRC $BUILDDIR
cd $STHOME

exit 0
