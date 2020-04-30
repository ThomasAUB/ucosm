# Unit Testing for uCoSM

## Dependencies

You need cppunit and scons in order to build and run the test.

On Debian-based machine, use apt-get to install the two packages:

- libcppunit-dev
- scons


## Building and running the test:

> scons test

Will build the test in *build/* and run it.

Status on all test is logged on the console.
It is possible to add other kind of report (xml, text file, ...), please check cppunit doc.

## Writing test

Check the sources in test/ folder.

and check cppunit doc