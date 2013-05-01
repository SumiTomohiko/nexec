
nexec
*****

.. contents:: Table of contents

Overview
========

nexec is system to use a remote machine as a local one over fsyscall_. You can
use CPU/memory/applications in the remote machine. The remote machine can also
read/write files in your machine.

.. _fsyscall: http://neko-daisuki.ddo.jp/~SumiTomohiko/fsyscall/index.html

nexec consists of two executables -- nexec and nexecd. nexec is the client
program, and nexecd is the server one.

Download
========

Tarballs (nexec-*x.y.z*.tar.xz) are available at `the author's repository
<http://neko-daisuki.ddo.jp/~SumiTomohiko/repos/index.html>`_.

How to install and prepare
==========================

Install fsyscall
----------------

nexec requires fsyscall in both of a local machine and a remote machine. Please
install fsyscall with following `the fsyscall documentation
<http://neko-daisuki.ddo.jp/~SumiTomohiko/fsyscall/index.html>`_. fmaster.ko and
fmhub must be installed into a server, fslave and fshub must be installed into
a client.

.. image:: install.png

Compile nexec
-------------

Compiling nexec needs `CMake <http://www.cmake.org>`_ 2.8.9. Instructions are::

    $ cmake . && make

You will get executables of nexec/nexec and nexecd/nexecd.

Load the kernel module of fsyscall
----------------------------------

Third step is loading the kernel module of fsyscall in the remoe machine. Please
execute the following command at the top directory of fsyscall::

    $ sudo kldload fmaster/fmaster.ko

Start nexecd
------------

Now is the time to start nexecd in the remote machine::

    $ nexecd

How to use
==========

Please give nexec with address of the remote machine and commands::

    $ nexec 192.168.42.26 /bin/echo foo bar baz quux

The above command executes /bin/echo in the remote machine of 192.168.42.26 with
passing four command line arguments of "foo", "bar", "baz" and "quux". You will
see the following stdout::

    foo bar baz quux

.. attention:: The current version supports only /bin/echo.

Anything else
=============

License
-------

nexec is under `the MIT license
<http://github.com/SumiTomohiko/nexec/blob/master/COPYING.rst#mit-license>`_.

GitHub repository
-----------------

GitHub repository of nexec is http://github.com/SumiTomohiko/nexec.

Author
------

The author of nexec is
`Tomohiko Sumi <http://neko-daisuki.ddo.jp/~SumiTomohiko/index.html>`_.

.. vim: tabstop=4 shiftwidth=4 expandtab softtabstop=4
