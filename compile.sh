#!/bin/bash
MULTIPIMVERSION=1.0
MULTIPIMPATH="$(pwd)"
PINPATH="$MULTIPIMPATH/pin"
BOOKSIMPATH="$MULTIPIMPATH/common/booksim2/src"
DRAMPOWERPATH="$MULTIPIMPATH/common/DRAMPower/src"
LIBCONFIGPATH="$MULTIPIMPATH/common/libconfig"
# XMLPARSERPATH="$MULTIPIMPATH/common/xmlparser"
NUMCPUS=$(grep -c ^processor /proc/cpuinfo)
NUMCPUS=2

export PINPATH
export LIBCONFIGPATH
export BOOKSIMPATH
export DRAMPOWERPATH
# export XMLPARSERPATH
export MULTIPIMVERSION

#--d, --o, --r, --p
if [ "$1" = "debug" ]
then
	BUILDTYPE=--d
elif [ "$1" = "opt" ]
then
	BUILDTYPE=--o
fi

if [ "$2" = "clean" ]
then
	rm -rf MULTIPIMPATH/build/$1
elif [ "$2" = "MultiPIM" ]
then
	echo "Compiling only MultiPIM ..."    
    scons -j$NUMCPUS $BUILDTYPE
else
	echo "Compiling all ..."
	cd $DRAMPOWERPATH/../
	echo "Compiling DRAMPower ..."
	make 
	cd -
	cd $BOOKSIMPATH
	echo "Compiling booksim ..."
	make
	cd -
	cd $LIBCONFIGPATH
	echo "Compiling libconfig ..."
	./configure --prefix=$LIBCONFIGPATH && make install
	cd -
	echo "Compiling MultiPIM ..."   
	scons -j$NUMCPUS $BUILDTYPE
fi
