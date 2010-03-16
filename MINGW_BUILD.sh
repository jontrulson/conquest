# This is how I build it using a mingw cross compiler on my system.  YMMV.
set -x
./configure --prefix=/tmp --build=i686-linux --host=i686-pc-mingw32 --with-sdl-prefix=/home/jon/mingw32


