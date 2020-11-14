#!/bin/bash
#
# Build script for Android
#
# $NDK must be set to NDK home directory.
# For example NDK=/home/daniel/android/android-ndk-r9d
#

export PATH=$(pwd)/ndk-toolchain/bin:$PATH

# armeabi

export CXX=arm-linux-androideabi-g++
export target="armeabi"

rm -rf ndk-toolchain 2>/dev/null
$NDK/build/tools/make-standalone-toolchain.sh --arch=armeabi --platform=android-9 --install-dir=./ndk-toolchain

make -f Makefile.android
make -f Makefile.android clean

# armv7-a

export CXX=arm-linux-androideabi-g++
export target="armeabi-v7a"

rm -rf ndk-toolchain 2>/dev/null
$NDK/build/tools/make-standalone-toolchain.sh --arch=armeabi-v7a --platform=android-9 --install-dir=./ndk-toolchain

make -f Makefile.android
make -f Makefile.android clean

# x86

export CXX=i686-linux-android-g++
export target="x86"

rm -rf ndk-toolchain 2>/dev/null
$NDK/build/tools/make-standalone-toolchain.sh --arch=x86 --platform=android-9 --install-dir=./ndk-toolchain

make -f Makefile.android
make -f Makefile.android clean

# mips

export CXX=mipsel-linux-android-g++
export target="mips"

rm -rf ndk-toolchain 2>/dev/null
$NDK/build/tools/make-standalone-toolchain.sh --arch=mips --platform=android-9 --install-dir=./ndk-toolchain

make -f Makefile.android
make -f Makefile.android clean
