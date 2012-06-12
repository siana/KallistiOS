#!/bin/sh
wget -c ftp://ftp.gnu.org/gnu/binutils/binutils-2.22.tar.bz2 || exit 1
wget -c ftp://ftp.gnu.org/gnu/gcc/gcc-4.7.0/gcc-4.7.0.tar.bz2 || exit 1
wget -c ftp://sources.redhat.com/pub/newlib/newlib-1.20.0.tar.gz || exit 1

