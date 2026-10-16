sudo add-apt-repository -y universe
sudo apt update

sudo apt install -y build-essential autoconf automake libtool pkg-config perl cmake curl tar

# Navigate to the folder you want to compile the libraries:
cd /mnt/staff01/programming/ubuntu/ub24.04/ubuntu_static_libs


1. Set Environment Variables and Folders

export REPO_DIR="/mnt/staff01/programming/ubuntu/ub24.04/ubuntu_static_libs"
export WORK_DIR="$REPO_DIR/work"
export INSTALL_PREFIX="$REPO_DIR/dist"

mkdir -p "$WORK_DIR" "$INSTALL_PREFIX" "$REPO_DIR/lib" "$REPO_DIR/include"



2. Build Static zlib
=====================
cd "$WORK_DIR"
curl -sSL https://github.com/madler/zlib/releases/download/v1.3.1/zlib-1.3.1.tar.gz | tar -xz
cd zlib-1.3.1

CFLAGS="-fPIC -O3" ./configure --static --prefix="${INSTALL_PREFIX}"
make -j$(nproc) install

ll ../../dist/lib/


3. Build Static OpenSSL 3.0 LTS
===============================
cd "$WORK_DIR"
curl -sSL https://github.com/openssl/openssl/releases/download/openssl-3.0.15/openssl-3.0.15.tar.gz | tar -xz
cd openssl-3.0.15

./config no-shared no-tests -fPIC --libdir=lib --prefix="${INSTALL_PREFIX}" --openssldir="${INSTALL_PREFIX}/ssl"
make -j$(nproc)
make install_sw

ll ../../dist/lib/


4. Build Static nghttp2
=======================
cd "$WORK_DIR"
curl -sSL https://github.com/nghttp2/nghttp2/releases/download/v1.63.0/nghttp2-1.63.0.tar.gz | tar -xz
cd nghttp2-1.63.0

./configure \
  --prefix="${INSTALL_PREFIX}" \
  --enable-lib-only \
  --enable-static \
  --disable-shared \
  CFLAGS="-fPIC -O3"

make -j$(nproc) install

ll ../../dist/lib/



5. Build Static cURL
====================
cd "$WORK_DIR"
curl -sSL https://curl.se/download/curl-8.10.1.tar.gz | tar -xz
cd curl-8.10.1

./configure \
  --prefix="${INSTALL_PREFIX}" \
  --with-openssl="${INSTALL_PREFIX}" \
  --with-zlib="${INSTALL_PREFIX}" \
  --with-nghttp2="${INSTALL_PREFIX}" \
  --enable-static \
  --disable-shared \
  --disable-ftp \
  --disable-ldap \
  --disable-ldaps \
  --disable-rtsp \
  --disable-dict \
  --disable-telnet \
  --disable-tftp \
  --disable-pop3 \
  --disable-imap \
  --disable-smb \
  --disable-gopher \
  --disable-mqtt \
  --without-nghttp3 \
  --without-ngtcp2 \
  --without-libssh2 \
  --without-libpsl \
  --without-libidn2 \
  --without-brotli \
  --without-zstd \
  --disable-file \
  --disable-ipfs \
  --disable-smtp \
  --disable-websockets \
  CPPFLAGS="-DOPENSSL_NO_ENGINE -I${INSTALL_PREFIX}/include" \
  LDFLAGS="-L${INSTALL_PREFIX}/lib -L${INSTALL_PREFIX}/lib64" \
  CFLAGS="-fPIC -O2"

make -j$(nproc) install

ll ../../dist/lib/


6. Update the repository: Copy Files to ubuntu_static_libs repository
=====================================================================
# Copy library archives
cp -f "${INSTALL_PREFIX}"/lib*/libcurl.a    "${REPO_DIR}/lib/"
cp -f "${INSTALL_PREFIX}"/lib*/libz.a       "${REPO_DIR}/lib/"
cp -f "${INSTALL_PREFIX}"/lib*/libssl.a     "${REPO_DIR}/lib/"
cp -f "${INSTALL_PREFIX}"/lib*/libcrypto.a  "${REPO_DIR}/lib/"
cp -f "${INSTALL_PREFIX}"/lib*/libnghttp2.a  "${REPO_DIR}/lib/"

# If libnghttp2 was not found, execute:
NGHTTP2_FILE=$(find "${INSTALL_PREFIX}" -name "libnghttp2*.a" | head -n 1)
cp -f "${NGHTTP2_FILE}" "${REPO_DIR}/lib/libnghttp2.a"

# Copy headers
cp -rf "${INSTALL_PREFIX}/include/curl"    "${REPO_DIR}/include/"
cp -rf "${INSTALL_PREFIX}/include/openssl" "${REPO_DIR}/include/"
cp -rf "${INSTALL_PREFIX}/include/nghttp2" "${REPO_DIR}/include/"
cp -f  "${INSTALL_PREFIX}/include/zlib.h"  "${REPO_DIR}/include/"
cp -f  "${INSTALL_PREFIX}/include/zconf.h" "${REPO_DIR}/include/"


7. Inspect the Generated Archives
=================================
ls -lh "${REPO_DIR}/lib"
file "${REPO_DIR}/lib/"*.a


8. Test the binary that uses these libraries:
=============================================
# Verify no OpenSSL or cURL crypto symbols are leaked globally
nm -D /path/to/missionx.xpl | grep -E "EVP_CipherInit|aesni_|curl_easy_init"
















####### CachyOS
####### As of v26.09.3 we link against statically cURL library ######
Commands:

## Zlib
Source: https://www.zlib.net/

./configure --prefix="/mnt/staff01/programming/git/curl_openssl_static/zlib_libs" --static


## nghttp2 library for cURL
URL: https://github.com/nghttp2/nghttp2

cd /mnt/staff01/programming/git/curl_openssl_static

git clone https://github.com/nghttp2/nghttp2.git
cd nghttp2

git submodule update --init

autoreconf -i

./configure \
  --prefix="/mnt/staff01/programming/git/curl_openssl_static/nghttp2_libs" \
  --enable-lib-only \
  --disable-shared \
  --enable-static \
  CFLAGS="-fPIC -O2" \
  CXXFLAGS="-fPIC -O2"


make -j 4
make install


## OpenSSL - statically
./Configure  linux-x86_64   --prefix=/mnt/staff01/programming/git/curl_openssl_static/openssl_libs no-shared no-tests '-Wl,-rpath,$(LIBRPATH)'
make -j 8
make install_sw


## Curl Statically:
If we got the curl source from: "git" repository:
autoreconf -fi

> execute configure:
./configure \
--prefix="/mnt/staff01/programming/git/curl_openssl_static/curl_libs" \
--with-openssl="/mnt/staff01/programming/git/curl_openssl_static/openssl_libs" \
--with-zlib="/mnt/staff01/programming/git/curl_openssl_static/zlib_libs" \
--with-nghttp2="/mnt/staff01/programming/git/curl_openssl_static/curl_libs" \
--enable-static \
--disable-shared \
--disable-ftp \
--disable-ldap \
--disable-ldaps \
--disable-rtsp \
--disable-dict \
--disable-telnet \
--disable-tftp \
--disable-pop3 \
--disable-imap \
--disable-smb \
--disable-gopher \
--disable-mqtt \
--without-nghttp3 \
--without-ngtcp2 \
--without-libssh2 \
--without-libpsl \
--without-libidn2 \
--without-brotli \
--without-zstd \
--disable-file \
--disable-ipfs \
--disable-smtp \
--disable-websockets \
CPPFLAGS="-DOPENSSL_NO_ENGINE" \
  CFLAGS="-fPIC -O2"

## Test curl configuration
./curl-config --libs
./curl-config --features
./curl-config --static-libs
./curl-config --protocols

make -j 8
make install

We basically need all the static files "libcurl.a, libssl.a, libz.a and nghttp2.a" so the plugin linkage will work correctly.
The "libcurl.a" does not hold all the static libraries when we linked it. Only the plugin will combine all of them into the plugin binary file.








######## Ignore this is for shared libraries ##################
The Linux build should not use the OpenSSL libraries in this folder, instead use the OS libraries and headers.


## Linux Libraries to build Mission-X plugin based on CURL + OpenSSL:

* Install key applications/libraries for X-Plane development
> apt install plocate  (for quick search)
>
> apt install freeglut3-dev
>
> apt install g++
>
> apt install libopenal1
>

* Install cURL dev library

> apt install libssl-dev
>
> apt install libcurl4-openssl-dev
>


**How to determind the GILIBC usage on an application**

> objdump -T /path/to/your/library.so | grep GLIBC_
>

