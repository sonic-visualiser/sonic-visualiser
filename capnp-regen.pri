
capnpc.target = piper-vamp-cpp/vamp-capnp/piper.capnp.h
capnpc.depends = $$PWD/piper/capnp/piper.capnp

capnpc.commands = capnp compile --src-prefix=$$PWD/piper/capnp -oc++:$$PWD/piper-vamp-cpp/vamp-capnp $$capnpc.depends

macx* {
    exists(sv-dependency-builds) {
        capnpc.commands=$$PWD/sv-dependency-builds/osx/bin/capnp -I$$PWD/sv-dependency-builds/osx/include compile --src-prefix=$$PWD/piper/capnp -o$$PWD/sv-dependency-builds/osx/bin/capnpc-c++:$$PWD/piper-vamp-cpp/vamp-capnp $$capnpc.depends
    }
}

win32-g++ {
    capnpc.commands=$$PWD/sv-dependency-builds/win32-mingw/bin/capnp -I$$PWD/sv-dependency-builds/win32-mingw/include compile --src-prefix=$$PWD/piper/capnp -o$$PWD/sv-dependency-builds/win32-mingw/bin/capnpc-c++:$$PWD/piper-vamp-cpp/vamp-capnp $$capnpc.depends
}

win32-msvc* {
    # This config is actually for 64-bit Windows builds -- see
    # comments in noconfig.pri.

    # With MSVC2017 we have a problem that the header dependency is
    # written out with the relative path from the build dir to the
    # source dir (e.g. ..\sonic-visualiser\...) so if the header
    # target path doesn't match that, the build fails before
    # regenerating it. Not a problem with VC2015 for some reason.
    # I hope using the relative path as target should fix it without
    # breaking the VC2015 build.

    capnpc.target = ../$$basename(PWD)/piper-vamp-cpp/vamp-capnp/piper.capnp.h
    capnpc.commands=$$PWD/sv-dependency-builds/win64-msvc/bin/capnp -I$$PWD/sv-dependency-builds/win64-msvc/include compile --src-prefix=$$PWD/piper/capnp -o$$PWD/sv-dependency-builds/win64-msvc/bin/capnpc-c++:$$PWD/piper-vamp-cpp/vamp-capnp $$capnpc.depends
}

QMAKE_EXTRA_TARGETS += capnpc
PRE_TARGETDEPS += $$capnpc.target
QMAKE_CLEAN += $$capnpc.target

