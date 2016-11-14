
TEMPLATE = subdirs

!win* {
    # We should build and run the tests on any platform,
    # but doing it automatically doesn't work so well from
    # within an IDE on Windows, so remove that from here
    SUBDIRS += \
	sub_test_svcore_base \
        sub_test_svcore_data_fileio \
        sub_test_svcore_data_model
}

SUBDIRS += \
	checker \
	sub_server \
        sub_convert \
	sub_sv

sub_test_svcore_base.file = test-svcore-base.pro
sub_test_svcore_data_fileio.file = test-svcore-data-fileio.pro
sub_test_svcore_data_model.file = test-svcore-data-model.pro

sub_server.file = server.pro
sub_convert.file = convert.pro
sub_sv.file = sv.pro
