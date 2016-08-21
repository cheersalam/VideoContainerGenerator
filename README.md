# VideoStreamer

A library for creating video containers and video rendering. It uses ffmpeg for video containers and decoding and SDL for rendering. Latest ffmpeg code is pulled and compiled when this library is build. It has some dependencies which needs to be resolved first before compiling it first.

It accepts H264 video frames (no audio at the moment) and can decode it for rendering or can create mp4 or mpegts containers. User can also pass duration for containers when library is initialized programatically.

it also works on Raspberry Pi3 running raspbian jessie

# Dependencies
 * `cmake`: for compiling project
 * `yasm` needed by ffmpeg
 * `libsdl-dev` for rendering  

  `sudo apt-get install cmake yasm libsdl-dev`

# Compiling project
 Go to root of the project

 * create debug directory
 * Enter debug directory
 * generate makefiles using cmake
 * Compile whole project
 
```Linux
$ cd VideoContainerGenerator
$ mkdir debug
$ cd debug
$ cmake ..
$ make
```
> Note: If compiling on pi3, it takes around 1 hour to compile whole project. Once ffmpeg is compiled it takes less than a minute to compile whole project again.
