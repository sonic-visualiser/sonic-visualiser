
AC_INIT(SVcore, 1.8, cannam@all-day-breakfast.com)

AC_CONFIG_SRCDIR(base/base.pro)

# Autoconf will set CXXFLAGS; we don't usually want it to, because we
# either define our own flags (at least if GCC is in use) or else use
# the user's preferences.  We need to ensure CXXFLAGS is only set if
# the user has expressly set it.  So, save the user's (or empty)
# setting now and restore it after Autoconf has done its bit of
# piddling about.
USER_CXXFLAGS="$CXXFLAGS"

AC_LANG_CPLUSPLUS

AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_MKDIR_P

AC_HEADER_STDC

# These are the flags Autoconf guesses for us; we use them later if
# the user has set none and we are not using GCC (so lack our own
# preferred flags)
AUTOCONF_CXXFLAGS="$CXXFLAGS"

PKG_PROG_PKG_CONFIG

SV_CHECK_QT

SV_DEFINES_DEBUG="-DDEBUG -DBUILD_DEBUG -DWANT_TIMING"
SV_DEFINES_RELEASE="-DNDEBUG -DBUILD_RELEASE -DNO_TIMING"
SV_DEFINES_MINIMAL="$SV_DEFINES_RELEASE"

# Now we have: USER_CXXFLAGS contains any flags the user set
# explicitly; AUTOCONF_CXXFLAGS contains flags that Autoconf thought
# we should use.  If we have GCC, we override the latter but then
# allow ourselves to be overridden (later) by the former

CXXFLAGS_DEBUG="$AUTOCONF_CXXFLAGS"
CXXFLAGS_RELEASE="$AUTOCONF_CXXFLAGS"
CXXFLAGS_MINIMAL="$AUTOCONF_CXXFLAGS"

if test "x$GCC" = "xyes"; then
        CXXFLAGS_DEBUG="-Wall -Woverloaded-virtual -Wextra -Wformat-nonliteral -Wformat-security -Winit-self -Wswitch-enum -g -pipe"
   	CXXFLAGS_RELEASE="-g0 -O2 -Wall -pipe"
   	CXXFLAGS_MINIMAL="-g0 -O0"
fi

CXXFLAGS_BUILD="$CXXFLAGS_RELEASE"
SV_DEFINES_BUILD="$SV_DEFINES_RELEASE"

AC_ARG_ENABLE(debug, [AS_HELP_STRING([--enable-debug],[enable debug support [default=no]])],[AC_MSG_NOTICE([enabling debug build])
CXXFLAGS_BUILD="$CXXFLAGS_DEBUG"
SV_DEFINES_BUILD="$SV_DEFINES_DEBUG"])

if test x"$USER_CXXFLAGS" != x; then
   	AC_MSG_NOTICE([The CXXFLAGS environment variable is set to "$USER_CXXFLAGS".])
	AC_MSG_NOTICE(Overriding default compiler flags with the above user setting.)
	CXXFLAGS_BUILD="$USER_CXXFLAGS"
	CXXFLAGS_MINIMAL="$USER_CXXFLAGS"
fi

CXXFLAGS="$CXXFLAGS_BUILD $SV_DEFINES_BUILD"

SV_MODULE_REQUIRED([bz2],[],[bzlib.h],[bz2],[BZ2_bzReadOpen])
SV_MODULE_REQUIRED([fftw3],[fftw3 >= 3.0.0],[fftw3.h],[fftw3],[fftw_execute])
SV_MODULE_REQUIRED([fftw3f],[fftw3f >= 3.0.0],[fftw3.h],[fftw3f],[fftwf_execute])
SV_MODULE_REQUIRED([sndfile],[sndfile >= 1.0.16],[sndfile.h],[sndfile],[sf_open])
SV_MODULE_REQUIRED([samplerate],[samplerate >= 0.1.2],[samplerate.h],[samplerate],[src_new])
SV_MODULE_REQUIRED([vamp],[vamp >= 2.1],[vamp/vamp.h],[],[])
SV_MODULE_REQUIRED([vamphostsdk],[vamp-hostsdk >= 2.0],[vamp-hostsdk/PluginLoader.h],[],[])
SV_MODULE_REQUIRED([rubberband],[rubberband],[rubberband/RubberBandStretcher.h],[rubberband],[rubberband_new])
SV_MODULE_REQUIRED([raptor],[raptor >= 1.4.20],[raptor.h],[raptor],[raptor_start_parse])
SV_MODULE_REQUIRED([rasqal],[rasqal >= 0.9.19],[rasqal/rasqal.h],[rasqal],[rasqal_new_world])
SV_MODULE_REQUIRED([redland],[redland >= 1.0.10],[librdf.h],[rdf],[librdf_new_world])

SV_MODULE_OPTIONAL([liblo],[],[lo/lo.h],[lo],[lo_address_new])
SV_MODULE_OPTIONAL([JACK],[jack >= 0.100],[jack/jack.h],[jack],[jack_client_open])
SV_MODULE_OPTIONAL([libpulse],[libpulse >= 0.9],[pulse/pulseaudio.h],[pulse],[pa_stream_new])
SV_MODULE_OPTIONAL([lrdf],[lrdf >= 0.2],[lrdf.h],[lrdf],[lrdf_init])
SV_MODULE_OPTIONAL([oggz],[oggz >= 1.0.0],[oggz/oggz.h],[oggz],[oggz_run])
SV_MODULE_OPTIONAL([fishsound],[fishsound >= 1.0.0],[fishsound/fishsound.h],[fishsound],[fish_sound_new])
SV_MODULE_OPTIONAL([mad],[mad >= 0.15.0],[mad.h],[mad],[mad_decoder_init])
SV_MODULE_OPTIONAL([id3tag],[id3tag >= 0.15.0],[id3tag.h],[id3tag],[id3_tag_new])

AC_SUBST(PERL)
AC_SUBST(XARGS)
AC_SUBST(MAKEDEPEND)
AC_SUBST(SHA1SUM)
AC_SUBST(CUT)

AC_SUBST(CFLAGS)
AC_SUBST(CXXFLAGS)
AC_SUBST(CXXFLAGS_MINIMAL)
AC_SUBST(HAVES)
AC_SUBST(LIBS)

AC_SUBST(CC)
AC_SUBST(CXX)

AC_SUBST(CODENAME)

AC_OUTPUT([config.pri])

if ! $QMAKE -r; then
   AC_MSG_ERROR([qmake failed: Command was "$QMAKE -r"])
fi

# qmake builds our static libraries, which is fine -- what we'd really
# like to do afterwards is bundle them into a single .so with e.g.

# gcc -Wl,--whole-archive */*.a -Wl,--no-whole-archive -o libsvcore.so

# but how best to arrange this? if it's not possible, we'll presumably
# just have to use a single (q)make(project)file instead.

AC_MSG_NOTICE([

Configuration complete.
Please check the above messages for any warnings that you
might care about, and then run "make".

The file config.pri contains the configuration settings for
qmake.  If you want to adjust these by hand, edit config.pri
and run "qmake -r" again to regenerate the Makefile.
])
