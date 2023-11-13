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
    wget https://github.com/boostorg/boost/releases/download/boost-1.83.0/boost-1.83.0.tar.gz
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
    wget https://github.com/jbeder/yaml-cpp/archive/refs/tags/0.8.0.tar.gz
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
# Install ZLIB
if [ ! -d "$THIRD_PARTY_DIR/zlib" ]; then
    rm -rf zlib-1.2.11.tar.gz  zlib-1.2.11
    wget http://www.zlib.net/fossils/zlib-1.2.11.tar.gz
    tar -xf zlib-1.2.11.tar.gz
    cd  ./zlib-1.2.11
    ./configure --prefix=$THIRD_PARTY_DIR/zlib || RET=$?
    make || RET=$?
    make install || RET=$?
    cd ..
    rm -rf  zlib-1.2.11.tar.gz  zlib-1.2.11
    if [ $RET -ne 0 ]; then
        rm -rf zlib
        echo "Intall zlib failed"
        exit 1
    fi
fi
# Install json-cpp
if [ ! -d "$THIRD_PARTY_DIR/json-cpp" ]; then
    rm -rf 00.11.0.tar.gz jsoncpp-00.11.0
    wget https://github.com/open-source-parsers/jsoncpp/archive/refs/tags/00.11.0.tar.gz
    tar -xf 00.11.0.tar.gz
    cd  ./jsoncpp-00.11.0
    cmake -G Ninja -S . -B ./build -DCMAKE_INSTALL_PREFIX=$THIRD_PARTY_DIR/json-cpp || RET=$?
    cmake --build ./build || RET=$?
    cmake --build ./build --target install || RET=$?
    cd ..
     rm -rf 00.11.0.tar.gz  jsoncpp-00.11.0
    if [ $RET -ne 0 ]; then
        rm -rf json-cpp
        echo "Intall yaml-cpp failed"
        exit 1
    fi
fi


# Install protobuf
if [ ! -d "$THIRD_PARTY_DIR/protobuf" ]; then
    rm -rf protobuf-all-21.1.tar.gz protobuf-3.21.1
    wget http://github.com/protocolbuffers/protobuf/releases/download/v21.1/protobuf-all-21.1.tar.gz
    tar -zxvf protobuf-all-21.1.tar.gz
    cd protobuf-3.21.1
    cmake -G Ninja -S . -B ./build -DCMAKE_INSTALL_PREFIX=$THIRD_PARTY_DIR/protobuf || RET=$?
    cmake --build ./build || RET=$?
    cmake --build ./build --target install || RET=$?
    cd ..
    rm -rf protobuf-all-21.1.tar.gz  protobuf-3.21.1
    if [ $RET -ne 0 ]; then
        rm -rf protobuf
        echo "Intall protobuf failed"
        exit 1
    fi
fi