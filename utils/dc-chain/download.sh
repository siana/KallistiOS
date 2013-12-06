#!/bin/sh

# These version numbers are all that should ever have to be changed.
export GCC_VER=4.7.3
export BINUTILS_VER=2.23.2
export NEWLIB_VER=2.0.0
export GMP_VER=5.1.3
export MPFR_VER=3.1.2
export MPC_VER=1.0.1

# Download everything.
if command -v wget >/dev/null 2>&1; then
    echo "Downloading binutils-$BINUTILS_VER..."
    wget -c ftp://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VER.tar.bz2 || exit 1
    echo "Downloading GCC $GCC_VER..."
    wget -c ftp://ftp.gnu.org/gnu/gcc/gcc-$GCC_VER/gcc-$GCC_VER.tar.bz2 || exit 1
    echo "Downloading Newlib $NEWLIB_VER..."
    wget -c ftp://sourceware.org/pub/newlib/newlib-$NEWLIB_VER.tar.gz || exit 1
    echo "Downloading GMP $GMP_VER..."
    wget -c ftp://ftp.gnu.org/gnu/gmp/gmp-$GMP_VER.tar.bz2 || exit 1
    echo "Downloading MPFR $MPFR_VER..."
    wget -c http://www.mpfr.org/mpfr-current/mpfr-$MPFR_VER.tar.bz2 || exit 1
    echo "Downloading MPC $MPC_VER..."
    wget -c http://www.multiprecision.org/mpc/download/mpc-$MPC_VER.tar.gz || exit 1
elif command -v curl >/dev/null 2>&1; then
    echo "Downloading Binutils $BINUTILS_VER..."
    curl -C - -O ftp://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VER.tar.bz2 || exit 1
    echo "Downloading GCC $GCC_VER..."
    curl -C - -O ftp://ftp.gnu.org/gnu/gcc/gcc-$GCC_VER/gcc-$GCC_VER.tar.bz2 || exit 1
    echo "Downloading Newlib $NEWLIB_VER..."
    curl -C - -O ftp://sourceware.org/pub/newlib/newlib-$NEWLIB_VER.tar.gz || exit 1
    echo "Downloading GMP $GMP_VER..."
    curl -C - -O ftp://ftp.gnu.org/gnu/gmp/gmp-$GMP_VER.tar.bz2 || exit 1
    echo "Downloading MPFR $MPFR_VER..."
    curl -O http://www.mpfr.org/mpfr-current/mpfr-$MPFR_VER.tar.bz2 || exit 1
    echo "Downloading MPC $MPC_VER..."
    curl -O http://www.multiprecision.org/mpc/download/mpc-$MPC_VER.tar.gz || exit 1
else
    echo >&2 "You must have either wget or cURL installed to use this script!"
    exit 1
fi
