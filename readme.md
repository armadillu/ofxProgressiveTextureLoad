ofxProgressiveTextureLoad
=====================================

OpenFrameworks add-on to load rgb textures (with mipmaps) without blocking the app, load them to CPU on a background thread, resize to fit power or two requirements for mipmaps, resize and prepare all mipmaps levels, and progressively upload that data to the GPU across several frames. 

Useful to load very large images... All this while keeping interactive frame rates.

You get notified when the texture is drawable (so that you can start drawing a low res version of it) and you will also get notified when its fully loaded.

also loads textures with no mipmaps.

See the example for more info.

[demo video](http://youtu.be/aQISt4ruskA) 

Introduction
------------
Describe what your addon is about

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
