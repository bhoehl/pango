#!/bin/sh
apt-get update -y
DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends tzdata
apt-get install -qy wget gcc g++ make autoconf git cmake python3 python3-pip python3-setuptools  python3-wheel ninja-build pkg-config xsltproc libtool gettext autopoint
apt-get install -qy libexpat1-dev libffi-dev libfribidi-dev libicu-dev libpcre2-dev libpixman-1-dev libpng-dev libxml2-dev libxslt1-dev libbrotli-dev libbz2-dev
apt-get install -qy libthai-dev libxft-dev  libxext-dev librsvg2-dev libspectre-dev libpoppler-dev libbfd-dev
apt-get install -qy gperf vim
pip3 install meson

export PATH=$PATH:/usr/local/bin/

# first build freetype without harfbuzz dependency
cd /tmp
wget https://downloads.sourceforge.net/freetype/freetype-2.12.1.tar.gz
tar xf freetype-2.12.1.tar.gz
cd freetype-2.12.1
sed -ri "s:.*(AUX_MODULES.*valid):\1:" modules.cfg
sed -r "s:.*(#.*SUBPIXEL_RENDERING) .*:\1:" -i include/freetype/config/ftoption.h
./configure --prefix=/usr --enable-freetype-config --enable-static --disable-shared --with-harfbuzz=no
make -j 8 && make install

# build fontconfig
cd /tmp
wget https://www.freedesktop.org/software/fontconfig/release/fontconfig-2.14.1.tar.xz
tar xf fontconfig-2.14.1.tar.xz
cd fontconfig-2.14.1
./configure --prefix=/usr --sysconfdir=/etc --localstatedir=/var --disable-docs --docdir=/usr/share/doc/fontconfig-2.14.1 --enable-static --disable-shared
make -j 8 && make install

# build cairo
cd /tmp
wget https://download.gnome.org/sources/cairo/1.17/cairo-1.17.6.tar.xz
tar xf cairo-1.17.6.tar.xz
cd cairo-1.17.6
./configure --prefix=/usr --enable-static --enable-tee --disable-shared
make -j 8 && make install

# build graphite
cd /tmp
wget https://github.com/silnrsi/graphite/releases/download/1.3.14/graphite2-1.3.14.tgz
tar xf graphite2-1.3.14.tgz
cd graphite2-1.3.14
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF .
cmake --build .
cmake --install .

# build  harfbuzz
wget https://github.com/harfbuzz/harfbuzz/releases/download/6.0.0/harfbuzz-6.0.0.tar.xz
tar xf harfbuzz-6.0.0.tar.xz
cd harfbuzz-6.0.0
mkdir build && cd build
meson --prefix=/usr --buildtype=release -Dgraphite2=enabled --default-library=static ..
ninja && ninja install

# build glib
cd /tmp
wget https://download.gnome.org/sources/glib/2.74/glib-2.74.4.tar.xz
tar xf glib-2.74.4.tar.xz
cd glib-2.74.4
mkdir build && cd build
meson --prefix=/usr --buildtype=release -Dman=false -Dgtk_doc=false --default-library=static  ..
ninja && ninja install

# build freetype with harfbuzz dependency NOW
cd /tmp/freetype-2.12.1
./configure --prefix=/usr --enable-freetype-config --enable-static --disable-shared
make -j 8 && make install

# build pango
cd /tmp
git clone --depth 1 --branch pango_static https://github.com/bhoehl/pango
cd pango
mkdir build && cd build
meson -Ddefault_library=static --prefix=/usr --buildtype=release --wrap-mode=nofallback ..
ninja
strip utils/pango-view
cp utils/pango-view /work
