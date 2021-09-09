## Mirror JPEG web application

A project just to prove my abilities at writing modern C++ code.

### Testing
```
curl --data-binary @input.jpeg 127.0.0.1:17070 --output output.jpeg
```

### Requirements
- libjpeg
- Boost

I wanted to use only Boost, but, unfortunately, Boost GIL still [has problems with YCbCr colorspace.](https://www.boost.org/doc/libs/1_76_0/libs/gil/doc/html/io.html)

> Reading YCbCr or YCCK images is possible but might result in inaccuracies since both color spaces aren’t available yet for gil. For now these color space are read as rgb images.

CMakeLists contains versions that I used while developing this, but it should work with older versions of Boost as well.

**C++17** standard is used, since **C++20** features are still not fully supported by compilers.

### Notes
- Graceful shutdown, timeouts, logging and other features.
- Due to Boost problems with JPEG primary colorspace, libjpeg is used, so there is a bunch of super C code in [mirror_jpeg_handler.cpp](src/mirror_jpeg_handler.cpp), don't be embarassed.
- HTTP server uses actual request handlers through interface to simplify replacing handlers or testing server functionality.
- ¯\\\_(ツ)\_/¯
