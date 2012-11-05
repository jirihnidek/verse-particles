Verse Particles
===============

This is Verse client for testing verse protocol. This application contains one program (Verse client). This program
could run in two modes. First mode (sender) is used for sending particle system over un-reliable network to Verse
server. The second mode of Verse client (receiver) should run on same machine as Verse server and it tries to
subscribe and receive particle data from first Verse client over Verse server.

Video with example could be found at YouTube:

http://www.youtube.com/watch?v=IJm-Inp9kTI

Compile
-------

It is possible to compile at Linux. Porting to other UNIX platform is probably possible.

Requirements
============

 * GCC: http://gcc.gnu.org/ or Clang: http://clang.llvm.org/ 
 * CMake http://www.cmake.org/
 * Verse: https://github.com/jirihnidek/verse
 * OpenGL: http://www.opengl.org/
 * GLUT: http://www.opengl.org/resources/libraries/glut/

Building
========

To build verse_particle binary you have to type following commands:

    mkdir ./build
    cd ./build
    cmake ../
    make

There isn't any step for installing application to the system, because this application is only for testing purpose.

Using
-----

To run verse_particle binary you have to do several steps. First of all, you have to have Verse server running.
Then you have to start one verse_particle in sender mode:

    ./bin/verse_particle -t sender host.with.verse.server.com ../particle_data/10

When sender is running, then you have to run receiver:

    ./bin/verse_particle -t receiver localhost ../particle_data/10

You can also run sender and sender at virtualized server and receiver at host. Therse is script ./bin/tc_set.sh
that could be used for modification of links between virtualized machine and host and vica verse.

Contacts
--------

 * E-mail: jiri.hnidek@tul.cz
 * Phone: +420 485 35 3695
 * Address: Studentska 2, 461 17, Liberec 1, Czech Republic
