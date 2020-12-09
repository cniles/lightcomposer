# Lightcomposer

## Dependencies

The following libraries need to be installed

sudo apt-get install libfftw3-dev libavdevice-dev libavcodec-dev libavfilter-dev libavutil-dev libswscale-dev libswresaemple-dev libsdl2-dev libboost-dev

Note that currently there is an issue with ffmpeg 4.x.x where software
resampling causes a segfault.  You'll need to use ffmpeg 3.x.x; this may mean building from source and specifying LIBAV_ROOT_DIR when running cmake.

### Optional

To use graphical lights for, e.g., debugging in a Windowed environment:

sudo apt-get install libsdl2-gfx-dev

To control the Pi's GPIO, you'll need WiringPi2

sudo apt-get install wiringpi

### Building

From the repository root:

```
mkdir build
cd build
cmake ../
make
```

### User-specified libav

If libav is not located under include and lib directories:

```
cmake ../ -D LIBAV_ROOT_DIR=/path/to/libav/
```

Note that it was necessary to aggregate all the libraries under a
single lib directory for them to be found by the cmake module. E.g.,
from the libav root directory:

```
mkdir lib
find ./ -name '*.so*' -exec cp {} ./lib \;
```

If building from source, make sure you include run configure with
`--enable-shared` so shared objects are generated.