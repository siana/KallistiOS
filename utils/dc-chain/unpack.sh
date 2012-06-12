#!/bin/sh

rm -rf binutils-2.22 gcc-4.7.0 newlib-1.20.0

tar jxf binutils-2.22.tar.bz2 || exit 1
tar jxf gcc-4.7.0.tar.bz2 || exit 1
tar zxf newlib-1.20.0.tar.gz || exit 1
