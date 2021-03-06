# LavaVu #

A scientific visualisation tool with a python interface for fast and flexible visual analysis.

LavaVu development is supported by the [Monash Immersive Visualisation Plaform](http://monash.edu.au/mivp) and the Simulation, Analysis & Modelling component of the [NCRIS AuScope](http://www.auscope.org.au/ncris/) capability.

The acronym stands for: **L**ightweight, **A**utomatable  **V**isualisation & **A**nalysis **V**iewing **U**tility, but "lava" is also a reference to its primary application as a viewer for geophysical simulations. It wsa also chosen to be unique enough to find the repository with google.

The project sprang out of the gLucifer<sup>1</sup> framework for visualising geodynamics simulations when the OpenGL visualisation module was separated from the simulation and sampling libraries, but is now a more general purpose visualisation tool. gLucifer continues as a set of sampling tools for Underworld simulations as part of the [Underworld2](https://github.com/underworldcode/underworld2/) code. LavaVu provides the rendering library for creating 2d and 3d visualisations to view this sampled data, inline within interactive IPython notebooks and offline through saved visualisation databases and images/movies.

As a standalone tool it is a scriptable 3D visualisation tool capable of producing publication quality high res images and video output from time varying data sets along with HTML5 3D visualisations in WebGL.
Rendering features include correctly and efficiently rendering large numbers of opaque and transparent points and surfaces and volume rendering by GPU ray-marching. There are also features for drawing vector fields and tracers (streamlines).

Control is via python and a set of simple verbose scripting commands along with mouse/keyboard interaction.
GUI components can be generated for use from a web browser via the python "control" module and a built in web server.

A native data format called GLDB is used to store and visualisations in a compact single file, using SQLite for storage and fast loading. A small number of other data formats are supported for import (OBJ surfaces, TIFF stacks etc). 
Further data import formats are supported with python scripts, with the numpy interface allowing rapid loading and manipulation of data.

A CAVE2 virtual reality mode is provided by utilising Omegalib (http://github.com/uic-evl/omegalib) to allow use in Virtual Reality and Immersive Visualisation facilities, such as the CAVE2 at Monash, see (https://github.com/mivp/LavaVR).
Side-by-side and quad buffer stereoscopic 3D support is also provided for other 3D displays.

### This repository ###

This is the public source code repository for all development on the project.
Currently I am the sole developer but contributions are welcome.
Development happens in the "master" branch with stable releases tagged.

### How do I get set up? ###

The simplest way to get started on a Unix OS (Mac/Linux) is clone this repository and build from source:
(You will need to first install git)

```
  git clone https://github.com/OKaluza/LavaVu
  cd LavaVu
  make -j4
```

If all goes well the viewer will be built and ready to run in ./bin, try running with:
  bin/LavaVu

### Dependencies ###

To build the python interface requires swig (http://www.swig.org/)

For video output, requires: libavcodec, libavformat, libavutil, libswscale

TODO:

Releases: prebuilt versions for Windows, Mac, Linux

### Who do I talk to? ###

* Owen Kaluza (at) monash.edu

For further documentation / examples, see the Wiki
* https://github.com/OKaluza/LavaVu/wiki

TODO: 
* License, include/update documentation

---
<sup>1</sup> Stegman, D.R., Moresi, L., Turnbull, R., Giordani, J., Sunter, P., Lo, A. and S. Quenette, *gLucifer: Next Generation Visualization Framework for High performance computational geodynamics*, 2008, Visual Geosciences
