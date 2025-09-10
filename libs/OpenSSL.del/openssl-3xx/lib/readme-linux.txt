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

