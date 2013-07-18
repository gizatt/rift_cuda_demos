rift_cuda_demos
===============

Various experiments with the Oculus Rift SDK leveraging CUDA for fancy rendering and such.

simple_particle_swirl:
To build: Modify the Makefile to point to the lib and include dirs of a version of the
Rift SDK -- they'll be formatted something like what I've already got there. (Change
RIFTIDIR and RIFTLDIR, specifically.) Then just run Make in that directory and, after
a cascade of warnings (I'll try to deal with those eventually), a binary should pop
out in ./bin. Yay!