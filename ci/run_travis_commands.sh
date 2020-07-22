#!/bin/bash

set -ev
cd build
cmake -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_CXX_FLAGS="$FLAGS" -DUSE_READLINE:BOOL=${USE_READLINE} -DCMAKE_INSTALL_PREFIX=${INSTALL} ..
make
ctest
make install
if [[ ${CMAKE_BUILD_TYPE} == Debug ]]; then
    cd ../regression && ./run-test-notiming.sh ../build/src/bin/opensmt;
    cd ../regression_itp && ./run-tests.sh ../build/src/bin/opensmt;
fi

cd ../examples && mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DOPENSMT_DIR=${INSTALL} ..
make