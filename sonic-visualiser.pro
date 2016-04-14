TEMPLATE = subdirs
SUBDIRS = sub_bq sub_dataquay svcore svgui svapp vamp-plugin-load-checker sub_sv 

!win* {
    # We should build and run the tests on any platform,
    # but doing it automatically doesn't work so well from
    # within an IDE on Windows, so remove that from here
    SUBDIRS += svcore/base/test svcore/data/fileio/test svcore/data/model/test
}

sub_bq.file = bq.pro
sub_sv.file = sv.pro

sub_dataquay.file = dataquay/lib.pro

svgui.depends = svcore
svapp.depends = svcore svgui
sub_sv.depends = svcore svgui svapp
