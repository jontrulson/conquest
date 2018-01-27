# This is how I build it using a mingw cross compiler on my system.  YMMV.
set -x

./configure --prefix=/tmp --disable-shared --build=i686-linux --host=i586-mingw32msvc --with-sdl-prefix=/home/jon/mingw32 CFLAGS="-I/home/jon/mingw32/include" LDFLAGS="-L/home/jon/mingw32/lib"
