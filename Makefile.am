ACLOCAL_AMFLAGS= -I m4
## This is the shared library to be built
lib_LTLIBRARIES = libmod_qr.la

## Define the source file for the module
libmod_qr_la_SOURCES = src/mod_qr.c src/qr_generator.c src/lookup3.c

## Define that an include directory is required.
INCLUDES = -I@apache_dir@/include 
CLFAGS = -fPIC
LIBS = -lpng
LIBS += -lqrencode

