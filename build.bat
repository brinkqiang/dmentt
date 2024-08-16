
rmdir /S /Q build
mkdir build
pushd build

cmake -A x64 -DCMAKE_BUILD_TYPE=relwithdebinfo -DENTT_BUILD_TESTING=ON ..
cmake --build . --config relwithdebinfo -- /m:%NUMBER_OF_PROCESSORS%
popd

rem pause