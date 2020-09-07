# SimpleHydrology

## My input (Paweł)

I'd found weigert's [Simple Hydrology](https://github.com/weigert/SimpleHydrology) repo and I became very interested in how it works. Alas, original repo isn't made for running on Windows and I had no Linux box to run it on and no desire to configure one for that one thing, so I hacked around it until it compiled and run successfully on Windows using Visual Studio 2017. It is also compiled as a x64 binary, which I believe isn't the case for the original. I've also added rotating of the terrain via left and right arrow as the original relied on second wheel on a mouse (??) to do it, as well as I had to change some semantics around bool arrays for droplets, as the way they were originally coded didn't play well with MSVC. Same goes for including source files in TinyEngine.h - MSVC complained about it (as in - the separate source files that were included were all marked with errors and solution didn't compile), so I just actually included them in that header.

Below is copied README from original repo with some minor changes.

Project is named as TinyEngineWindows as it was initially to just made TinyEngine compile on Windows, but SimpleHydrology running on Windows was what I really wanted. Will probably change someday as I plan to tinker and hack this wonderful piece of software.

All the stuff apart from what I've described earlier - credits to weigert, I am too inept for this sort of thing. Hopefully I'll learn enough from it though!

Proof that I got it working:
![Proof](https://github.com/pawel-mazurkiewicz/SimpleHydrologyWindows/raw/master/simplehydrologyonwindows.png)

Hopefully it can be useful for someone like me.

## Summary

C++ implementation of a particle based procedural hydrology system for terrain generation. This extends simple particle based hydraulic erosion to capture streams and pools.

Rendered using weigert's homebrew [TinyEngine](https://github.com/weigert/TinyEngine).

[Link to a blog post about this](https://weigert.vsos.ethz.ch/2020/04/15/procedural-hydrology/)

![Banner Image](https://weigert.vsos.ethz.ch/wp-content/uploads/2020/04/Banner.png)

## Compiling

Use the makefile to compile the program.

Using "Build Solution" in Visual Studio 2017 or higher. Remember to update Project properties with your include and lib search directories.
    
### Dependencies

Erosion System:
- MSVC
- glm
- [LibNoise64](https://github.com/eldernos/LibNoise64) - you need to compile it yourself

Renderer (TinyEngine):
- SDL2 (Core, Image, TTF, Mixer)
- OpenGL3
- GLEW
- Boost
- ImGUI (included already as header files)

Full list of linked libraries:
- SDL2.lib
- SDL2_ttf.lib
- SDL2_mixer.lib
- SDL2_image.lib
- opengl32.lib
- glew32s.lib
- LibNoise64.lib

## Usage

    ./TinyEngineWindows.exe SEED

If no seed is specified, it will take a random one. Remember to have .dlls installed either in program's directory or Windows itself.

### Controls

    - Zoom Camera: Scroll
    - Rotate Camera: LEFT / RIGHT
    - Toggle Pause: P (WARNING: PAUSED BY DEFAULT!!)
    - Change Camera Vertical Angle: UP / DOWN
    - Toggle Hydrology Map View: ESC
    - Move the Camera Anchor: WASD / SPACE / C

### Screenshots
![Example Output](https://weigert.vsos.ethz.ch/wp-content/uploads/2020/04/hydrology.png)
Example output when simulating on semi-rugged terrain.

![Hydrology Map](https://weigert.vsos.ethz.ch/wp-content/uploads/2020/04/HydroMap-1.png)
Example of a generated hydrology map.

![Terrain Render](https://weigert.vsos.ethz.ch/wp-content/uploads/2020/04/HeightMap.png)
Corresponding rendering of the terrain.


## Reading
The main file is just to wrap the OpenGL code for drawing. At the very bottom, you can see the main game loop that calls the erosion and vegetation growth functions.

The part of the code described in the blog article is contained in the file `water.h`. Read this to find the implementation of the procedural hydrology.

The trees are implemented in `vegetation.h`.

All of the code is wrapped with the world class in `world.h`. The bottom of this file contains a bunch of stuff relevant for rendering, but not the erosion system.

The rest is shaders and rendering stuff.

## License
MIT License.

See Nicholas website for a [more detailed copyright notice](https://weigert.vsos.ethz.ch/copyright-notice/).
