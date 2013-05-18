#!/bin/sh

rm -rf binutils-2.23.2 gcc-4.7.3 newlib-2.0.0

tar jxf binutils-2.23.2.tar.bz2 || exit 1
tar jxf gcc-4.7.3.tar.bz2 || exit 1
tar zxf newlib-2.0.0.tar.gz || exit 1
