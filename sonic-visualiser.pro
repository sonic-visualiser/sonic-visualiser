
TEMPLATE = subdirs

!win* {
    # We should build and run the tests on any platform,
    # but doing it automatically doesn't work so well from
    # within an IDE on Windows, so remove that from here
    SUBDIRS += svcore/base/test svcore/data/fileio/test svcore/data/model/test
}

SUBDIRS += \
	checker \
	sub_server \
        sub_convert \
	sub_sv

sub_server.file = server.pro
sub_convert.file = convert.pro
sub_sv.file = sv.pro
