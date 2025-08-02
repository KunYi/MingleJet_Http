MingleJet Http Server
===

MingleJet is a simple HTTP server that utilizes libuv, llhttp, libxml2, nng, and utlist.

### BUILD
```
$ mkdir build
$ cd build
./build $ cmake ..
./build $ make
```

### Testing
```
./build $ cp -rf ../test/dist .
./build $ ./MingleJet
```
The expected output is as follows:
```
Launch MingleJet...

use the below third party components
  libuv:1.51.0 Release
  llhttp:9.3.0
  libxml2:2.14.5
  nng:1.11.0
  uthash:2.3.0 (for utlist/utarray)

Server listening on port 8080...
on_message_begin HTTP method: 0l
Headers complete
Request complete
Parse pass, type:1, method:1, url: /
check fs_stat failed
on_message_begin HTTP method: 1l
Headers complete
Request complete
Parse pass, type:1, method:1, url: /
try to find default file./dist/index.html
on_message_begin HTTP method: 1l
Headers complete
Request complete
Parse pass, type:1, method:1, url: /test.html
UV_EOF, close the connection
```

### Tips

Workaround for Valgrind Detection Issues

Apply the patch:
```
./patches/libuv/0001-patch-for-memcheck-of-valgrind.patch
```

For more detailed information, refer to [valgrind thinks statbuf is uninitialised during uv_fs_stat (false positive)](https://github.com/libuv/libuv/issues/4008)

### Third-Party Components
* [libuv](https://github.com/libuv/libuv)
* [llhttp](https://github.com/nodejs/llhttp)
* [libxml2](https://gitlab.gnome.org/GNOME/libxml2)
* [nng](https://github.com/nanomsg/nng)
* [uthash](https://github.com/troydhanson/uthash)


### License
MIT License, see [LICENSE](./LICENSE)
