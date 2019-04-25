
macx* {
      QMAKE_LFLAGS += -sectcreate __TEXT __info_plist $$shell_quote($$PWD/deploy/osx/Info-helpers.plist)
}
