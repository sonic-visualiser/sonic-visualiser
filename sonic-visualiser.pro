TEMPLATE = subdirs
SUBDIRS = dataquay_lib svcore svgui svapp sub_sv svcore/data/fileio/test

dataquay_lib.file = dataquay/lib.pro

sub_sv.file = sv.pro

svgui.depends = svcore
svapp.depends = svcore svgui
sub_sv.depends = svcore svgui svapp
