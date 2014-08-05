Verse Particles
===============

This project contains Verse client application used for testing [Verse protocol](https://verse.github.com). This
aplication could run in two modes. First mode (sender) is used for sending particle system to Verse server. The sender
should run on the same machine as Verse server. The second mode of Verse client (receiver) receives particle data
from Verse server over unreliable network. It uses verse layers for sharing particle position.

![Verse Particle Screenshot](/screenshots/verse-particles.png "Screenshot of Verse Particles")

Video with example could be found at YouTube:

 * http://www.youtube.com/watch?v=IJm-Inp9kTI

License
=======

The source code of Verse Particle is licensed under BSD license. For more details look at file LICENSE.

Compile
=======

It is possible to compile at Linux. Porting to other UNIX platform is probably possible.

Requirements
------------

 * GCC: http://gcc.gnu.org/ or Clang: http://clang.llvm.org/ 
 * CMake http://www.cmake.org/
 * Verse: https://verse.github.com
 * OpenGL: http://www.opengl.org/
 * GLUT: http://www.opengl.org/resources/libraries/glut/

Building
--------

To build verse_particle binary you have to type following commands:

    mkdir ./build
    cd ./build
    cmake ../
    make

There isn't any step for installing application to the system, because this application is only for testing purpose.

Usage
=====

To run verse_particle binary you have to do several steps. First of all, you have to have Verse server running.
Then you have to start one verse_particle in sender mode:

    ./bin/verse_particle -t sender host.with.verse.server.com ../particle_data/10

When sender is running, then you have to run receiver:

    ./bin/verse_particle -t receiver host.with.verse.server.com ../particle_data/10

You can also run sender and sender at virtualized server and receiver at host. Therse is script ./bin/tc_set.sh
that could be used for modification of links between virtualized machine and host and vica verse.

Contact
=======

 * E-mail: jiri.hnidek@tul.cz
 * Phone: +420 485 35 3695
 * Address: Studentska 2, 461 17, Liberec 1, Czech Republic
