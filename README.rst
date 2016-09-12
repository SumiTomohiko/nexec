
nexec
*****

Prerequirements
===============

* FreeBSD 10.3/amd64

How to install
==============

Prerequirements
---------------

* GNU Make 4.2.1
* cmake 3.5.2

Building instructions
---------------------

::

  $ ./configure
  $ make
  $ sudo make install

Post-installation
-----------------

Add a user and a group for the nexecd::

  $ sudo pw groupadd nexecd
  $ sudo pw useradd nexecd -g nexecd -w no -s /usr/sbin/nologin -d /nonexsistent -c "nexec daemon"

Enable the nexecd service. Add this statement into ``/etc/rc.conf``::

  nexecd_enable="YES"

.. vim: tabstop=2 shiftwidth=2 expandtab softtabstop=2 filetype=rst
