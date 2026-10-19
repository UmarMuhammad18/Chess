# Prefer the official NDK toolchain file:
#   $ANDROID_NDK/build/cmake/android.toolchain.cmake
#
# cmake -B build-android \
#   -DCHESS_ANDROID=ON \
#   -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
#   -DANDROID_ABI=arm64-v8a \
#   -DANDROID_PLATFORM=android-24
