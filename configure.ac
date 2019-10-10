
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
SV_DEFINES_RELEASE="-DNDEBUG -DBUILD_RELEASE -DNO_TIMING -DNO_HIT_COUNTS"

# Now we have: USER_CXXFLAGS contains any flags the user set
# explicitly; AUTOCONF_CXXFLAGS contains flags that Autoconf thought
# we should use.  If we have GCC, we override the latter but then
# allow ourselves to be overridden (later) by the former

CXXFLAGS_DEBUG="$AUTOCONF_CXXFLAGS"
OPTFLAGS_DEBUG="-O2"
LDFLAGS_DEBUG=""
CXXFLAGS_RELEASE="$AUTOCONF_CXXFLAGS"
OPTFLAGS_RELEASE="-O2"
LDFLAGS_RELEASE=""
CXXSTANDARD=c++14

if test "x$GCC" = "xyes"; then
   	CXXFLAGS_ANY="-fpic -Wall -Wextra -Woverloaded-virtual -Wformat-nonliteral -Wformat-security -Winit-self -Wswitch-enum -Wconversion -pipe"
        CXXFLAGS_DEBUG="$CXXFLAGS_ANY -g -Werror"
        OPTFLAGS_DEBUG="-O2"
   	CXXFLAGS_RELEASE="$CXXFLAGS_ANY -g0"
        OPTFLAGS_RELEASE="-O3 -ffast-math"
        LIBS_DEBUG="$LIBS"
        AX_CHECK_COMPILE_FLAG(--std=$CXXSTANDARD, [], [AC_MSG_NOTICE([Compiler does not appear to support $CXXSTANDARD, falling back to c++11])
        CXXSTANDARD=c++11])
fi

CXXFLAGS_BUILD="$CXXFLAGS_RELEASE $OPTFLAGS_RELEASE"
LDFLAGS_BUILD="$LDFLAGS_RELEASE $OPTFLAGS_RELEASE"
SV_DEFINES_BUILD="$SV_DEFINES_RELEASE"

QMAKE_CONFIG="release"

AC_ARG_ENABLE(debug, [AS_HELP_STRING([--enable-debug],[enable debug support [default=no]])],[AC_MSG_NOTICE([enabling debug build])
QMAKE_CONFIG="debug"
CXXFLAGS_BUILD="$CXXFLAGS_DEBUG $OPTFLAGS_DEBUG"
LDFLAGS_BUILD="$LDFLAGS_DEBUG"
SV_DEFINES_BUILD="$SV_DEFINES_DEBUG"
LIBS="$LIBS_DEBUG"])

if test x"$USER_CXXFLAGS" != x; then
   	AC_MSG_NOTICE([The CXXFLAGS environment variable is set to "$USER_CXXFLAGS".])
	AC_MSG_NOTICE(Overriding default compiler flags with the above user setting.)
	CXXFLAGS_BUILD="$USER_CXXFLAGS"
        LDFLAGS_BUILD=""
fi

CXXFLAGS="$CXXFLAGS_BUILD $SV_DEFINES_BUILD"
LDFLAGS="$LDFLAGS_BUILD"

SV_MODULE_REQUIRED([bz2],[],[bzlib.h],[bz2],[BZ2_bzReadOpen])
SV_MODULE_REQUIRED([fftw3],[fftw3 >= 3.0.0],[fftw3.h],[fftw3],[fftw_execute])
SV_MODULE_REQUIRED([fftw3f],[fftw3f >= 3.0.0],[fftw3.h],[fftw3f],[fftwf_execute])
SV_MODULE_REQUIRED([sndfile],[sndfile >= 1.0.16],[sndfile.h],[sndfile],[sf_open])
SV_MODULE_REQUIRED([libsamplerate],[samplerate >= 0.1.2],[samplerate.h],[samplerate],[src_new])
SV_MODULE_REQUIRED([rubberband],[rubberband],[rubberband/RubberBandStretcher.h],[rubberband],[rubberband_new])
SV_MODULE_REQUIRED([sord],[sord-0 >= 0.5],[sord/sord.h],[sord-0],[sord_world_new])
SV_MODULE_REQUIRED([serd],[serd-0 >= 0.5],[serd/serd.h],[serd-0],[serd_reader_read_file])

# We need a Cap'n Proto version with the expectedSizeInWordsFromPrefix
# function, which appeared in v0.6. If we can't find at least that
# version with pkg-config, we check for the symbol with the compiler.
PKG_CHECK_MODULES([capnp],[capnp >= 0.6],[HAVES="$HAVES HAVE_CAPNP";CXXFLAGS="$CXXFLAGS $capnp_CFLAGS";LIBS="$LIBS $capnp_LIBS"],[AC_MSG_NOTICE([Failed to find required module capnp using pkg-config, trying again by old-fashioned means])])
if test -z "$capnp_LIBS" ; then
    AC_COMPILE_IFELSE([
        AC_LANG_SOURCE([[
        #include <capnp/serialize.h>
        int main() {
            kj::Array<capnp::word> prefix;
            return capnp::expectedSizeInWordsFromPrefix(prefix.asPtr());
        }]])],
        [HAVES="$HAVES HAVE_CAPNP";LIBS="$LIBS -lcapnp -lkj"],
        [AC_MSG_ERROR([Failed to find appropriate capnp library])]
    )
fi

SV_MODULE_OPTIONAL([liblo],[],[lo/lo.h],[lo],[lo_address_new])
SV_MODULE_OPTIONAL([portaudio],[portaudio-2.0 >= 19],[portaudio.h],[portaudio],[Pa_IsFormatSupported])
SV_MODULE_OPTIONAL([JACK],[jack >= 0.100],[jack/jack.h],[jack],[jack_client_open])
SV_MODULE_OPTIONAL([libpulse],[libpulse >= 0.9],[pulse/pulseaudio.h],[pulse],[pa_stream_new])

SV_MODULE_REQUIRED([lrdf],[lrdf >= 0.2],[lrdf.h],[lrdf],[lrdf_init])
SV_MODULE_REQUIRED([oggz],[oggz >= 1.0.0],[oggz/oggz.h],[oggz],[oggz_run])
SV_MODULE_REQUIRED([fishsound],[fishsound >= 1.0.0],[fishsound/fishsound.h],[fishsound],[fish_sound_new])
SV_MODULE_REQUIRED([mad],[mad >= 0.15.0],[mad.h],[mad],[mad_decoder_init])
SV_MODULE_REQUIRED([id3tag],[id3tag >= 0.15.0],[id3tag.h],[id3tag],[id3_tag_new])
SV_MODULE_REQUIRED([opus],[opusfile],[opus/opusfile.h],[opusfile],[op_read_float])

# Link in -lX11 if it exists -- this is for the X error handler
SV_MODULE_OPTIONAL([X11],[x11 >= 1.0.0],[X11/X.h],[x11],[XGetErrorText])

AC_SUBST(PERL)
AC_SUBST(XARGS)
AC_SUBST(MAKEDEPEND)
AC_SUBST(SHA1SUM)
AC_SUBST(CUT)

AC_SUBST(CFLAGS)
AC_SUBST(CXXFLAGS)
AC_SUBST(CXXSTANDARD)
AC_SUBST(LDFLAGS)
AC_SUBST(HAVES)
AC_SUBST(LIBS)

AC_SUBST(CC)
AC_SUBST(CXX)

AC_SUBST(prefix)

AC_SUBST(QMAKE_CONFIG)

AC_CONFIG_FILES([config.pri])

AC_OUTPUT

if test -x repoint ; then
   if test -d .hg -o -d .git ; then 
      if ! ./repoint install; then
         AC_MSG_ERROR([Repoint failed; please fix any reported errors and try again])
      fi
   else
      AC_MSG_NOTICE([Repoint executable found but not in an Hg or Git working-copy: not running it])
      if ! test -d vamp-plugin-sdk ; then
          AC_MSG_WARN([No vamp-plugin-sdk directory present, so external libraries might not have been updated])
      fi
   fi
else
   AC_MSG_NOTICE([No Repoint executable found: assuming external components are already here])
   if ! test -d vamp-plugin-sdk ; then
      AC_MSG_WARN([No vamp-plugin-sdk directory present, so external components might not have been updated])
   fi
fi

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
