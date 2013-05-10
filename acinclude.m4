
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

