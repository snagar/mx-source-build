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

