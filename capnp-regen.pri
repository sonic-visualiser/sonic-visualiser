
capnpc.target = piper-cpp/vamp-capnp/piper-capnp.h
capnpc.depends = $$PWD/piper/capnp/piper.capnp

capnpc.commands = capnp compile --src-prefix=$$PWD/piper/capnp -oc++:$$PWD/piper-cpp/vamp-capnp $$capnpc.depends

macx* {
    capnpc.commands=$$PWD/sv-dependency-builds/osx/bin/capnp -I$$PWD/sv-dependency-builds/osx/include compile --src-prefix=$$PWD/piper/capnp -o$$PWD/sv-dependency-builds/osx/bin/capnpc-c++:$$PWD/piper-cpp/vamp-capnp $$capnpc.depends
}

win32-msvc* {
    # This config is actually for 64-bit Windows builds -- see
    # comments in noconfig.pri
    capnpc.commands=$$PWD/sv-dependency-builds/win64-msvc/bin/capnp -I$$PWD/sv-dependency-builds/win64-msvc/include compile --src-prefix=$$PWD/piper/capnp -o$$PWD/sv-dependency-builds/win64-msvc/bin/capnpc-c++:$$PWD/piper-cpp/vamp-capnp $$capnpc.depends
}

QMAKE_EXTRA_TARGETS += capnpc
PRE_TARGETDEPS += $$capnpc.target

