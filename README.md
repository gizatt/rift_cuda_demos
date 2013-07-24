rift_cuda_demos
===============

Various experiments with the Oculus Rift SDK leveraging CUDA for fancy rendering and such.

simple_particle_swirl:
To build: Modify the Makefile to point to the lib and include dirs of a version of the
Rift SDK -- they'll be formatted something like what I've already got there. (Change
RIFTIDIR and RIFTLDIR, specifically.) Then just run Make in that directory and, after
a cascade of warnings (I'll try to deal with those eventually), a binary should pop
out in ./bin. Yay!

What it SHOULD render (it doesn't currently) is a flat thin white ground (-10->10 in
x and z, y=0), and a bunch of swirling reddish particles overhead.

simple_particle_swirl.cu runs the made code, and has some brief outline in its header,
as well as a lot of prototypes up near the top. It is based on glut's callback structure,
and isn't very fancy, but it does initialize and rely on two helpers, defined in common/:
a Rift helper, which wraps the Rift SDK, and a Player helper, which can be fed arguments
fed to glut callbacks to manage the player's movement with rules encoded in that class.
The purpose of these is mainly to get repeated / common code out of the specific demos
into more useful general classes.

One particular thing about how the Rift wrapper is set up: it actually is designed to
set up (opengl-based) rendering for you, having a method called "render" which takes
an eye position and rotation (the position the camera would have been at in a naive 
world without a rift / fancier stereo), and a function pointer to a core rendering
function that contains all of the drawing things that need to happen during scene
rendering, AFTER modelview/perspective/view are set up, but BEFORE screen flipping.
(The idea is that the Rift helepr sets up the various view and model and such matrices,
as well as distortion shaders as such, then calls the callback for both eyes setting
viewport appropriately, then finally flips screen.)

It doesn't work currently, because I such at opengl. aaah.