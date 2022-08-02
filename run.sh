#!/bin/sh
apt-get update -y
DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends tzdata
apt-get install -qy wget gcc g++ make autoconf
apt-get install -qy libthai-dev
apt-get install -qy libcairo-dev
apt-get install -qy libfontconfig-dev
apt-get install -qy libfribidi-dev
apt-get install -qy glib-2.0-dev
apt-get install -qy libxft-dev
apt-get install -qy libxrender-dev
apt-get install -qy git
apt-get install -qy libharfbuzz-dev
apt-get install -qy cmake
apt-get install -qy python3 python3-pip python3-setuptools  python3-wheel ninja-build

pip3 install meson
export PATH=$PATH:/usr/local/bin/

cd /tmp
wget https://github.com/silnrsi/graphite/releases/download/1.3.14/graphite2-1.3.14.tgz
tar xf graphite2-1.3.14.tgz
cd graphite2-1.3.14
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF .
cmake --build .
cmake --install .

cd /tmp
git clone https://github.com/bhoehl/pango
cd /tmp/pango
git checkout pango_static
mkdir build
cd /tmp/pango/build
meson -Ddefault_library=static --prefix=/usr ..
ninja
strip utils/pango-view
cp utils/pango-view /work
