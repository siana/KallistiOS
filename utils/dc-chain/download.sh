#!/bin/sh
wget -c ftp://ftp.gnu.org/gnu/binutils/binutils-2.22.tar.bz2 || exit 1
wget -c ftp://ftp.gnu.org/gnu/gcc/gcc-4.5.2/gcc-4.5.2.tar.bz2 || exit 1
wget -c ftp://sources.redhat.com/pub/newlib/newlib-1.19.0.tar.gz || exit 1

