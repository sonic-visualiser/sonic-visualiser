TEMPLATE = subdirs
SUBDIRS = dataquay_lib svcore svgui svapp sub_sv

!win* {
    # We should build and run the tests on any platform,
    # but doing it automatically doesn't work so well from
    # within an IDE on Windows, so remove that from here
    SUBDIRS += svcore/data/fileio/test
}

dataquay_lib.file = dataquay/lib.pro

sub_sv.file = sv.pro

svgui.depends = svcore
svapp.depends = svcore svgui
sub_sv.depends = svcore svgui svapp
