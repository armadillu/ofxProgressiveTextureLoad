# ofxProgressiveTextureLoad

[![Build Status](https://travis-ci.org/armadillu/ofxProgressiveTextureLoad.svg?branch=master)](https://travis-ci.org/armadillu/ofxProgressiveTextureLoad)
[![Build status](https://ci.appveyor.com/api/projects/status/2kx6dm9iykkmr7m2/branch/master?svg=true)](https://ci.appveyor.com/project/armadillu/ofxprogressivetextureload/branch/master)

OpenFrameworks add-on to load grayscale, RGB and RGBA textures (with mipmaps) without blocking the app, load them to CPU on a background thread, resize to fit power or two requirements for mipmaps, resize and prepare all mipmaps levels, and progressively upload that data to the GPU across several frames. 

Useful to load very large images... All this while keeping interactive frame rates.

You get notified when the texture is drawable (so that you can start drawing a low res version of it) and you will also get notified when its fully loaded.

also loads textures with no mipmaps.

There's a helper class to help handle a scenario where several simultaneous requests might happen, allowing only N of them to be loaded at the same time.

See the examples for more info.

[demo video](http://youtu.be/aQISt4ruskA) 

Requires ofxOpenCV for image resizing, example uses ofxHistoryPlot and ofxTimeMeasurements to track performance, but they are not necessary.


License
-------
MIT

Compatibility
------------
tested on 0F >=0.8.3

Known issues
------------

Version history
------------
