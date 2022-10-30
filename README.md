## Dürer's Solid, rendered in SDL
This was intended as a way to play with SDL, WebAssembly and Emscripten.  
It's a simple toy SDL example.  

A more robust implementation would likely involve better memory management  
by way of a resource manager for managing SDL surfaces, textures, and similar
resources.

Check the Makefile for targets.  
While most of the SDL functionality works "out of the box" with Emscripten,  
certain things like the SDL event loop had to be modifed from a working C++
example to use Emscripten APIs to play nice with event-driven multitasking.  
See the call to `emscripten_set_main_loop_arg()` for an example.  

The html target requires running an HTTP server to host the files to manage
CORS headers and filesystem access.  
The web target should output an ES6 module, including a `initModule` function.  

