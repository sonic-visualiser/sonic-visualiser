TEMPLATE = subdirs
SUBDIRS = sub_dataquay svcore svgui svapp sub_sv #svcore/data/fileio/test

sub_sv.file = sv.pro

sub_dataquay.file = dataquay/lib.pro

svgui.depends = svcore
svapp.depends = svcore svgui
sub_sv.depends = svcore svgui svapp
