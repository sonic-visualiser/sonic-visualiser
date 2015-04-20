
AC_DEFUN([SV_MODULE_REQUIRED],
[
SV_MODULE_MODULE=$1
SV_MODULE_VERSION_TEST="$2"
SV_MODULE_HEADER=$3
SV_MODULE_LIB=$4
SV_MODULE_FUNC=$5
SV_MODULE_HAVE=HAVE_$(echo $1 | tr '[a-z]' '[A-Z]')
SV_MODULE_FAILED=1
if test -n "$$1_LIBS" ; then
   AC_MSG_NOTICE([User set ${SV_MODULE_MODULE}_LIBS explicitly, skipping test for $SV_MODULE_MODULE])
   CXXFLAGS="$CXXFLAGS $$1_CFLAGS"
   LIBS="$LIBS $$1_LIBS"
   SV_MODULE_FAILED=""
fi
if test -z "$SV_MODULE_VERSION_TEST" ; then
   SV_MODULE_VERSION_TEST=$SV_MODULE_MODULE
fi
if test -n "$SV_MODULE_FAILED" && test -n "$PKG_CONFIG"; then
   PKG_CHECK_MODULES($1,[$SV_MODULE_VERSION_TEST],[HAVES="$HAVES $SV_MODULE_HAVE";CXXFLAGS="$CXXFLAGS $$1_CFLAGS";LIBS="$LIBS $$1_LIBS";SV_MODULE_FAILED=""],[AC_MSG_NOTICE([Failed to find required module $SV_MODULE_MODULE using pkg-config, trying again by old-fashioned means])])
fi
if test -n "$SV_MODULE_FAILED"; then
   AC_CHECK_HEADER([$SV_MODULE_HEADER],[HAVES="$HAVES $SV_MODULE_HAVE"],[AC_MSG_ERROR([Failed to find header $SV_MODULE_HEADER for required module $SV_MODULE_MODULE])])
   if test -n "$SV_MODULE_LIB"; then
     AC_CHECK_LIB([$SV_MODULE_LIB],[$SV_MODULE_FUNC],[LIBS="$LIBS -l$SV_MODULE_LIB"],[AC_MSG_ERROR([Failed to find library $SV_MODULE_LIB for required module $SV_MODULE_MODULE])])
   fi
fi
])

AC_DEFUN([SV_MODULE_OPTIONAL],
[
SV_MODULE_MODULE=$1
SV_MODULE_VERSION_TEST="$2"
SV_MODULE_HEADER=$3
SV_MODULE_LIB=$4
SV_MODULE_FUNC=$5
SV_MODULE_HAVE=HAVE_$(echo $1 | tr '[a-z]' '[A-Z]')
SV_MODULE_FAILED=1
if test -n "$$1_LIBS" ; then
   AC_MSG_NOTICE([User set ${SV_MODULE_MODULE}_LIBS explicitly, skipping test for $SV_MODULE_MODULE])
   CXXFLAGS="$CXXFLAGS $$1_CFLAGS"
   LIBS="$LIBS $$1_LIBS"
   SV_MODULE_FAILED=""
fi
if test -z "$SV_MODULE_VERSION_TEST" ; then
   SV_MODULE_VERSION_TEST=$SV_MODULE_MODULE
fi
if test -n "$SV_MODULE_FAILED" && test -n "$PKG_CONFIG"; then
   PKG_CHECK_MODULES($1,[$SV_MODULE_VERSION_TEST],[HAVES="$HAVES $SV_MODULE_HAVE";CXXFLAGS="$CXXFLAGS $$1_CFLAGS";LIBS="$LIBS $$1_LIBS";SV_MODULE_FAILED=""],[AC_MSG_NOTICE([Failed to find optional module $SV_MODULE_MODULE using pkg-config, trying again by old-fashioned means])])
fi
if test -n "$SV_MODULE_FAILED"; then
   AC_CHECK_HEADER([$SV_MODULE_HEADER],[HAVES="$HAVES $SV_MODULE_HAVE";SV_MODULE_FAILED=""],[AC_MSG_NOTICE([Failed to find header $SV_MODULE_HEADER for optional module $SV_MODULE_MODULE])])
   if test -z "$SV_MODULE_FAILED"; then
      if test -n "$SV_MODULE_LIB"; then
           AC_CHECK_LIB([$SV_MODULE_LIB],[$SV_MODULE_FUNC],[LIBS="$LIBS -l$SV_MODULE_LIB"],[AC_MSG_NOTICE([Failed to find library $SV_MODULE_LIB for optional module $SV_MODULE_MODULE])])
      fi
   fi
fi
])

# Check for Qt.  The only part of Qt we use directly is qmake.

AC_DEFUN([SV_CHECK_QT],
[
AC_REQUIRE([AC_PROG_CXX])

if test x$QMAKE = x ; then
   	AC_CHECK_PROG(QMAKE, qmake-qt5, $QTDIR/bin/qmake-qt5,,$QTDIR/bin/)
fi
if test x$QMAKE = x ; then
   	AC_CHECK_PROG(QMAKE, qmake, $QTDIR/bin/qmake,,$QTDIR/bin/)
fi
if test x$QMAKE = x ; then
	AC_CHECK_PROG(QMAKE, qmake.exe, $QTDIR/bin/qmake.exe,,$QTDIR/bin/)
fi
if test x$QMAKE = x ; then
   	AC_CHECK_PROG(QMAKE, qmake-qt5, qmake-qt5,,$PATH)
fi
if test x$QMAKE = x ; then
   	AC_CHECK_PROG(QMAKE, qmake, qmake,,$PATH)
fi
if test x$QMAKE = x ; then
   	AC_MSG_ERROR([
Failed to find the required qmake-qt5 or qmake program.  Please
ensure you have the necessary Qt5 development files installed, and
if necessary set QTDIR to the location of your Qt5 installation.
])
fi

# Suitable versions of qmake should print out something like:
#
#   QMake version 2.01a
#   Using Qt version 4.6.3 in /usr/lib
#
# This may be translated, so we check only for the numbers (2.x and 4.x
# in that order).
#
QMAKE_VERSION_OUTPUT=`$QMAKE -v`
case "$QMAKE_VERSION_OUTPUT" in
     *5.*) ;;
     *) AC_MSG_WARN([
 *** The version of qmake found in "$QMAKE" looks like it might be
     from the wrong version of Qt (Qt5 is required).  Please check
     that this is the correct version of qmake for Qt5 builds.
])
esac

case "`uname`" in
     *Darwin*) QMAKE="$QMAKE -spec macx-g++";;
esac

])

# From autoconf archive:

# ============================================================================
#  http://www.gnu.org/software/autoconf-archive/ax_cxx_compile_stdcxx_11.html
# ============================================================================
#
# SYNOPSIS
#
#   AX_CXX_COMPILE_STDCXX_11([ext|noext],[mandatory|optional])
#
# DESCRIPTION
#
#   Check for baseline language coverage in the compiler for the C++11
#   standard; if necessary, add switches to CXXFLAGS to enable support.
#
#   The first argument, if specified, indicates whether you insist on an
#   extended mode (e.g. -std=gnu++11) or a strict conformance mode (e.g.
#   -std=c++11).  If neither is specified, you get whatever works, with
#   preference for an extended mode.
#
#   The second argument, if specified 'mandatory' or if left unspecified,
#   indicates that baseline C++11 support is required and that the macro
#   should error out if no mode with that support is found.  If specified
#   'optional', then configuration proceeds regardless, after defining
#   HAVE_CXX11 if and only if a supporting mode is found.
#
# LICENSE
#
#   Copyright (c) 2008 Benjamin Kosnik <bkoz@redhat.com>
#   Copyright (c) 2012 Zack Weinberg <zackw@panix.com>
#   Copyright (c) 2013 Roy Stogner <roystgnr@ices.utexas.edu>
#   Copyright (c) 2014 Alexey Sokolov <sokolov@google.com>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

m4_define([_AX_CXX_COMPILE_STDCXX_11_testbody], [[
  template <typename T>
    struct check
    {
      static_assert(sizeof(int) <= sizeof(T), "not big enough");
    };

    struct Base {
    virtual void f() {}
    };
    struct Child : public Base {
    virtual void f() override {}
    };

    typedef check<check<bool>> right_angle_brackets;

    int a;
    decltype(a) b;

    typedef check<int> check_type;
    check_type c;
    check_type&& cr = static_cast<check_type&&>(c);

    auto d = a;
    auto l = [](){};
]])

AC_DEFUN([AX_CXX_COMPILE_STDCXX_11], [dnl
  m4_if([$1], [], [],
        [$1], [ext], [],
        [$1], [noext], [],
        [m4_fatal([invalid argument `$1' to AX_CXX_COMPILE_STDCXX_11])])dnl
  m4_if([$2], [], [ax_cxx_compile_cxx11_required=true],
        [$2], [mandatory], [ax_cxx_compile_cxx11_required=true],
        [$2], [optional], [ax_cxx_compile_cxx11_required=false],
        [m4_fatal([invalid second argument `$2' to AX_CXX_COMPILE_STDCXX_11])])
  AC_LANG_PUSH([C++])dnl
  ac_success=no
  AC_CACHE_CHECK(whether $CXX supports C++11 features by default,
  ax_cv_cxx_compile_cxx11,
  [AC_COMPILE_IFELSE([AC_LANG_SOURCE([_AX_CXX_COMPILE_STDCXX_11_testbody])],
    [ax_cv_cxx_compile_cxx11=yes],
    [ax_cv_cxx_compile_cxx11=no])])
  if test x$ax_cv_cxx_compile_cxx11 = xyes; then
    ac_success=yes
  fi

  m4_if([$1], [noext], [], [dnl
  if test x$ac_success = xno; then
    for switch in -std=gnu++11 -std=gnu++0x; do
      cachevar=AS_TR_SH([ax_cv_cxx_compile_cxx11_$switch])
      AC_CACHE_CHECK(whether $CXX supports C++11 features with $switch,
                     $cachevar,
        [ac_save_CXXFLAGS="$CXXFLAGS"
         CXXFLAGS="$CXXFLAGS $switch"
         AC_COMPILE_IFELSE([AC_LANG_SOURCE([_AX_CXX_COMPILE_STDCXX_11_testbody])],
          [eval $cachevar=yes],
          [eval $cachevar=no])
         CXXFLAGS="$ac_save_CXXFLAGS"])
      if eval test x\$$cachevar = xyes; then
        CXXFLAGS="$CXXFLAGS $switch"
        ac_success=yes
        break
      fi
    done
  fi])

  m4_if([$1], [ext], [], [dnl
  if test x$ac_success = xno; then
    for switch in -std=c++11 -std=c++0x; do
      cachevar=AS_TR_SH([ax_cv_cxx_compile_cxx11_$switch])
      AC_CACHE_CHECK(whether $CXX supports C++11 features with $switch,
                     $cachevar,
        [ac_save_CXXFLAGS="$CXXFLAGS"
         CXXFLAGS="$CXXFLAGS $switch"
         AC_COMPILE_IFELSE([AC_LANG_SOURCE([_AX_CXX_COMPILE_STDCXX_11_testbody])],
          [eval $cachevar=yes],
          [eval $cachevar=no])
         CXXFLAGS="$ac_save_CXXFLAGS"])
      if eval test x\$$cachevar = xyes; then
        CXXFLAGS="$CXXFLAGS $switch"
        ac_success=yes
        break
      fi
    done
  fi])
  AC_LANG_POP([C++])
  if test x$ax_cxx_compile_cxx11_required = xtrue; then
    if test x$ac_success = xno; then
      AC_MSG_ERROR([*** A compiler with support for C++11 language features is required.])
    fi
  else
    if test x$ac_success = xno; then
      HAVE_CXX11=0
      AC_MSG_NOTICE([No compiler with C++11 support was found])
    else
      HAVE_CXX11=1
      AC_DEFINE(HAVE_CXX11,1,
                [define if the compiler supports basic C++11 syntax])
    fi

    AC_SUBST(HAVE_CXX11)
  fi
])

