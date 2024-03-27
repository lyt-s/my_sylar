#!bin/bash
#seript/install_thirdparty.sh

set -eux
# Important paths:
#current script location:
# SCRIPT_DIR=$(dirname -- "($readlink -f -- "$0")")
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
#project root directory:
PROJECT_ROOT_DIR=$(realpath "$SCRIPT_DIR/../")
# third_party directory:
THIRD_PARTY_DIR=$(realpath "$SCRIPT_DIR/../third_party/")
RET=0

if [ ! -d "$THIRD_PARTY_DIR" ]; then
    mkdir -p $THIRD_PARTY_DIR
fi

cd $THIRD_PARTY_DIR
#Install Boost
if [ ! -d "$THIRD_PARTY_DIR/boost" ]; then
    rm -rf boost-1.83.0.tar.gz boost-1.83.0
    wget http://github.com/boostorg/boost/releases/download/boost-1.83.0/boost-1.83.0.tar.gz
    tar -xf boost-1.83.0.tar.gz
    cd  ./boost-1.83.0
    ./bootstrap.sh --prefix=$THIRD_PARTY_DIR/boost || RET=$?
    ./b2 install || RET=$?
    cd ..
    rm -rf boost-1.83.0.tar.gz boost-1.83.0
    if [ $RET -ne 0 ]; then
        rm -rf boost
        echo "Intall Boost failed"
        exit 1
    fi
fi

# Install yaml-cpp
if [ ! -d "$THIRD_PARTY_DIR/yaml-cpp" ]; then
    rm -rf 0.8.0.tar.gz  yaml-cpp-0.8.0
    wget http://github.com/jbeder/yaml-cpp/archive/refs/tags/0.8.0.tar.gz
    tar -xf 0.8.0.tar.gz
    cd  ./yaml-cpp-0.8.0
    cmake -G Ninja -S . -B ./build -DCMAKE_INSTALL_PREFIX=$THIRD_PARTY_DIR/yaml-cpp || RET=$?
    cmake --build ./build || RET=$?
    cmake --build ./build --target install || RET=$?
    cd ..
    rm -rf  yaml-cpp-0.8.0 0.8.0.tar.gz
    if [ $RET -ne 0 ]; then
        rm -rf yaml-cpp
        echo "Intall yaml-cpp failed"
        exit 1
    fi
fi