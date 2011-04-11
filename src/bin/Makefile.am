AM_CPPFLAGS = -I$(srcdir)/../include
AM_LDFLAGS = -lfst -lxerces-c -ldl

bin_PROGRAMS = kyfd componentcompose beamtrim buildfstmodel

kyfd_SOURCES = kyfd.cc
kyfd_LDADD = ../lib/libkyfd.la

componentcompose_SOURCES = componentcompose.cc
componentcompose_LDADD = ../lib/libkyfd.la

buildfstmodel_SOURCES = buildfstmodel.cc
buildfstmodel_LDADD = ../lib/libkyfd.la

beamtrim_SOURCES = beamtrim.cc
beamtrim_LDADD = ../lib/libkyfd.la
