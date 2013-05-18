#!/bin/sh
wget -c ftp://ftp.gnu.org/gnu/binutils/binutils-2.23.2.tar.bz2 || exit 1
wget -c ftp://ftp.gnu.org/gnu/gcc/gcc-4.7.3/gcc-4.7.3.tar.bz2 || exit 1
wget -c ftp://sourceware.org/pub/newlib/newlib-2.0.0.tar.gz || exit 1

