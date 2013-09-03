rift_cuda_demos
===============

Various experiments with the Oculus Rift SDK leveraging CUDA for
	fancy rendering and such.

Credit for the working Rift GLSL shaders goes to Luke Groeninger:
	https://github.com/dghost/glslRiftDistort

To build: Modify the Makefile to point to the lib and include dirs of the 
various libraries at the top. Then just run Make in that directory and, after
a cascade of warnings (I'll try to deal with those eventually), binaries
should pop out in ./bin. Yay!

common/:
	Contains a bunch of general helpers for managing various
	parts of the system. Docs on those are under development... the
	most important one is the rift helper, which is a self-contained
	Rift initialization-and-management class that abstracts away most
	of the nastyness interfacing with the Rift SDK.

	One particular thing about how the Rift wrapper is set up: it actually
	is designed to set up (opengl-based) rendering for you, having a method 
	called "render" which takes an eye position and rotation (the position 
	the camera would have been at in a naive world without a rift / fancier 
	stereo), and a function pointer to a core rendering function that 
	contains all of the drawing things that need to happen during scene 
	rendering, AFTER modelview/perspective/view are set up, but BEFORE 
	screen flipping. (The idea is that the Rift helepr sets up the various 
	view and model and such matrices, as well as distortion shaders as such, 
	then calls the callback for both eyes setting viewport appropriately, 
	then finally flips screen.)

simple_particle_swirl:
	What it currently renders is a flat thin white ground (-100->100 in
	x and z, y=-0.1), and a bunch of swirling reddish particles overhead.
	The particles have positions updated via a CUDA kernel (so there are
	a lot of them... I think 1 or 2 million?) Development still underway,
	but it's a general test ground for Hydra, Rift, Ironman-esque HUD,
	etc etc.

	Press and hold c and click to simulate having a Rift if one isn't
	plugged in.

	w/a/s/d to walk around, mouse to look around, etc etc. More
	details here %TODO.

webcam_feedthrough:
	Demo demonstrating stereo camera feed through on rift.
	Controls:

        c to enable contour detection ({/} change threshold)
        t to enable thresholding ([/] change threshold)
        b to enable black/white 
        s to enable sobel
        i to toggle drawing main image over/under things
        h to toggle a HUD showing FPS
        f to toggle showing STAR features
        </> to switch camera shown in left eye, 
        ,/. to switch camera shown in right eye
        and press +/- to draw image closer or farther to get
            the rough projection size correct.

        And others are gradually being added. Some of those
        things can be rendered over others, by the way -- 
        favorites of mine are start and turn on sobel (s),
        to get sharper edges drawn over image. Doing just that,
        or turning off main image (hit i) looks very pretty.