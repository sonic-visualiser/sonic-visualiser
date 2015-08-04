
AC_INIT([Sonic Visualiser], [], cannam@all-day-breakfast.com)

AC_CONFIG_SRCDIR(main/main.cpp)

# Autoconf will set CXXFLAGS; we don't usually want it to, because we
# either define our own flags (at least if GCC is in use) or else use
# the user's preferences.  We need to ensure CXXFLAGS is only set if
# the user has expressly set it.  So, save the user's (or empty)
# setting now and restore it after Autoconf has done its bit of
# piddling about.
USER_CXXFLAGS="$CXXFLAGS"

# If the user supplied CFLAGS but not CXXFLAGS, use CFLAGS instead
if test x"$USER_CXXFLAGS" = x; then
   if test x"$CFLAGS" != x; then
      USER_CXXFLAGS="$CFLAGS"
   fi
fi

AC_LANG_CPLUSPLUS

AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_MKDIR_P

# We are daringly making use of C++11 now
AX_CXX_COMPILE_STDCXX_11(noext)

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
   	CXXFLAGS_ANY="-Wall -Wextra -Werror -Woverloaded-virtual -Wformat-nonliteral -Wformat-security -Winit-self -Wswitch-enum -Wconversion -pipe"
        CXXFLAGS_DEBUG="$CXXFLAGS_ANY -g"
   	CXXFLAGS_RELEASE="$CXXFLAGS_ANY -g0 -O2"
   	CXXFLAGS_MINIMAL="$CXXFLAGS_ANY -g0 -O0"
fi

CXXFLAGS_BUILD="$CXXFLAGS_RELEASE"
SV_DEFINES_BUILD="$SV_DEFINES_RELEASE"

QMAKE_CONFIG="release"

AC_ARG_ENABLE(debug, [AS_HELP_STRING([--enable-debug],[enable debug support [default=no]])],[AC_MSG_NOTICE([enabling debug build])
QMAKE_CONFIG="debug"
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
SV_MODULE_REQUIRED([libsamplerate],[samplerate >= 0.1.2],[samplerate.h],[samplerate],[src_new])
SV_MODULE_REQUIRED([vamp],[vamp >= 2.1],[vamp/vamp.h],[],[])
SV_MODULE_REQUIRED([vamphostsdk],[vamp-hostsdk >= 2.5],[vamp-hostsdk/PluginLoader.h],[vamp-hostsdk],[libvamphostsdk_v_2_5_present])
SV_MODULE_REQUIRED([rubberband],[rubberband],[rubberband/RubberBandStretcher.h],[rubberband],[rubberband_new])
SV_MODULE_REQUIRED([sord],[sord-0 >= 0.5],[sord/sord.h],[sord-0],[sord_world_new])
SV_MODULE_REQUIRED([serd],[serd-0 >= 0.5],[serd/serd.h],[serd-0],[serd_reader_read_file])

SV_MODULE_OPTIONAL([liblo],[],[lo/lo.h],[lo],[lo_address_new])
SV_MODULE_OPTIONAL([portaudio],[portaudio-2.0 >= 19],[portaudio.h],[portaudio],[Pa_IsFormatSupported])
SV_MODULE_OPTIONAL([JACK],[jack >= 0.100],[jack/jack.h],[jack],[jack_client_open])
SV_MODULE_OPTIONAL([libpulse],[libpulse >= 0.9],[pulse/pulseaudio.h],[pulse],[pa_stream_new])
SV_MODULE_OPTIONAL([lrdf],[lrdf >= 0.2],[lrdf.h],[lrdf],[lrdf_init])
SV_MODULE_OPTIONAL([oggz],[oggz >= 1.0.0],[oggz/oggz.h],[oggz],[oggz_run])
SV_MODULE_OPTIONAL([fishsound],[fishsound >= 1.0.0],[fishsound/fishsound.h],[fishsound],[fish_sound_new])
SV_MODULE_OPTIONAL([mad],[mad >= 0.15.0],[mad.h],[mad],[mad_decoder_init])
SV_MODULE_OPTIONAL([id3tag],[id3tag >= 0.15.0],[id3tag.h],[id3tag],[id3_tag_new])

# Link in -lX11 if it exists -- this is for the X error handler
SV_MODULE_OPTIONAL([X11],[x11 >= 1.0.0],[X11/X.h],[x11],[XGetErrorText])

AC_SUBST(PERL)
AC_SUBST(XARGS)
AC_SUBST(MAKEDEPEND)
AC_SUBST(SHA1SUM)
AC_SUBST(CUT)

AC_SUBST(CFLAGS)
AC_SUBST(CXXFLAGS)
AC_SUBST(CXXFLAGS_MINIMAL)
AC_SUBST(LDFLAGS)
AC_SUBST(HAVES)
AC_SUBST(LIBS)

AC_SUBST(CC)
AC_SUBST(CXX)

AC_SUBST(QMAKE_CONFIG)

AC_CONFIG_SUBDIRS([svcore svgui svapp])
AC_CONFIG_FILES([config.pri])

AC_OUTPUT

if ! $QMAKE -r sonic-visualiser.pro; then
   AC_MSG_ERROR([qmake failed: Command was "$QMAKE -r"])
fi

AC_MSG_NOTICE([

Configuration complete.
Please check the above messages for any warnings that you
might care about, and then run "make".

The file config.pri contains the configuration settings for
qmake.  If you want to adjust these by hand, edit config.pri
and run "$QMAKE -r" again to regenerate the Makefile.
])
