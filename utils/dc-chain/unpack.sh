#!/bin/sh

# These version numbers are all that should ever have to be changed.
export GCC_VER=4.7.3
export BINUTILS_VER=2.23.2
export NEWLIB_VER=2.0.0
export GMP_VER=5.1.3
export MPFR_VER=3.1.2
export MPC_VER=1.0.1

# Clean up from any old builds.
rm -rf binutils-$BINUTILS_VER gcc-$GCC_VER newlib-$NEWLIB_VER
rm -rf gmp-$GMP_VER mpfr-$MPFR_VER mpc-$MPC_VER

# Unpack everything.
tar jxf binutils-$BINUTILS_VER.tar.bz2 || exit 1
tar jxf gcc-$GCC_VER.tar.bz2 || exit 1
tar zxf newlib-$NEWLIB_VER.tar.gz || exit 1
tar jxf gmp-$GMP_VER.tar.bz2 || exit 1
tar jxf mpfr-$MPFR_VER.tar.bz2 || exit 1
tar zxf mpc-$MPC_VER.tar.gz || exit 1

# Move the GCC dependencies into their required locations.
mv gmp-$GMP_VER gcc-$GCC_VER/gmp
mv mpfr-$MPFR_VER gcc-$GCC_VER/mpfr
mv mpc-$MPC_VER gcc-$GCC_VER/mpc
