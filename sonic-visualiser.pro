TEMPLATE = subdirs
SUBDIRS = svcore svgui svapp sub_sv svcore/data/fileio/test

sub_sv.file = sv.pro

svgui.depends = svcore
svapp.depends = svcore svgui
sub_sv.depends = svcore svgui svapp
