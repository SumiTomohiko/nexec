
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

Moreover, `Android client`_ is ready. You can use applications on a FreeBSD
machine from your Android tablet.

.. _Android client:
    http://neko-daisuki.ddo.jp/~SumiTomohiko/android-nexec-client/index.html

What are the rolls of nexec?
============================

The core software to transfer system call requests is fsyscall. But fsyscall
does not have any functions to make connections. And, the function to tell a
command to execute in a master machine is not included in fsyscall.

The roles of nexec are these lost functions -- connecting with a server, and
sending a command to a master machine.

Requirements
============

nexec (and fsyscall) works on FreeBSD/amd64.

Download
========

Tarballs (nexec-*x.y.z*.tar.xz) are available at `the author's repository`_.

.. _the author's repository:
    http://neko-daisuki.ddo.jp/~SumiTomohiko/repos/index.html

How to compile/install
======================

Requirements
------------

Compiling nexec needs

* `CMake <http://www.cmake.org>`_ 2.8.9

Install fsyscall
----------------

nexec requires fsyscall in both of a local machine and a remote machine. Please
install fsyscall with following `the fsyscall documentation`_. fmaster.ko and
fmhub must be installed into a server, fslave and fshub must be installed into a
client.

.. image:: install.png

.. _the fsyscall documentation:
    http://neko-daisuki.ddo.jp/~SumiTomohiko/fsyscall/index.html

Edit configure.conf
-------------------

The Second step of installing is editing configure.conf. You can give the
building/compiling options by this file. There is a sample file of this file
named configure.conf.sample at the top directory of the source tree. Copying
this sample is the most easy beginning::

    $ cp configure.conf.sample configure.conf
    $ your_favorite_editor configure.conf

You must give two settings, prefix and fsyscall_dir in the following form::

    prefix: ~/.local
    fsyscall_dir: ~/projects/fsyscall2

prefix is the top directory to install. The executables and the configuretaion
file will be installed into prefix/bin or prefix/etc. fsyscall_dir is the
directory which contains the source tree of fsyscall.

Compile nexec
-------------

The command to compile and install is::

    $ ./configure && make && make install

How to compile the client of Java version
=========================================

Requirements
------------

To compile Java version, you must use

* `Open JDK`_ 1.6.0
* `Apache Ant`_ 1.8.4

.. _Open JDK: http://openjdk.java.net/
.. _Apache Ant: http://ant.apache.org/

Compile
-------

You can compile Java version with the command::

    $ make java

Then, you will have java/bin/nexec-client.jar.

Start nexecd
============

Edit /etc/nexecd.conf
---------------------

/etc/nexecd.conf is the file to define behavior of nexecd. The contents of this
file is like::

    mapping
        "echo": "/bin/echo"
        "ffmpeg": "/usr/local/bin/ffmpeg"
    end

The mapping section defines commands. The left side of a colon (":") is a
command name, and the right side is a path to an executable. nexec client must
specify one command in the mapping section, and nexecd DOES NOT EXECUTE ANY
COMMANDS WHICH DO NOT APPEAR IN THIS SECTION.

Load the kernel module of fsyscall
----------------------------------

The second step is loading the kernel module of fsyscall in the remote machine.
Please execute the following command at the top directory of fsyscall::

    $ sudo kldload fmaster/fmaster.ko

Start nexecd
------------

Now is the time to start nexecd in the remote machine::

    $ nexecd

Stop nexecd
-----------

SIGTERM stops nexecd::

    $ killall nexecd    # SIGTERM is the default signal of killall.

How to use
==========

Please give nexec with address of the remote machine and commands::

    $ nexec 192.168.42.26 echo foo bar baz quux

The above command executes echo in the remote machine of 192.168.42.26 with
passing four command line arguments of "foo", "bar", "baz" and "quux". You will
see the following stdout::

    foo bar baz quux

You can see the supported applications in `the fsyscall page`_.

.. _the fsyscall page:
    http://neko-daisuki.ddo.jp/~SumiTomohiko/fsyscall/index.html#supported-applications

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
