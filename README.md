mpvd
====

About mpvd
-------------

mpvd is a background service to play music with the `mpv(1)` media player.

mpvd is distributed under the terms of the GNU GPL License (version 3).


Compiling mpvd
--------------

mpvd depends on the following components:

 * libmpv from the mpv project

With GCC, this should then be enough to compile and install mpvd:

    $ make install

To install (or package) mpvd in a different location:

    $ make PREFIX="/another/prefix" install

mpvd also supports `DESTDIR`, to be installed in a staging directory; for
instance:

    $ make DESTDIR="/staging/directory" PREFIX="/another/prefix" install


Contributing to mpvd
--------------------

It is possible to re-generate the Makefiles for mpvd with `configure(1)`
from the DeforaOS configure project, found at
<https://www.defora.org/os/project/16/configure>. The procedure is then as
follows:

    $ configure
    $ make

Please refer to the documentation of DeforaOS configure for further
instructions.
