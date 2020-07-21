
TEMPLATE = subdirs

SUBDIRS += \
        sub_base

win32-msvc* {
    SUBDIRS += \
        sub_os_other \
        sub_os_win10
}

# We build the tests on every platform, though at the time of
# writing they are only automatically run on non-Windows platforms
# (because of the difficulty of getting them running nicely in the
# IDE without causing great confusion if a test fails).
SUBDIRS += \
        sub_test_svcore_base \
        sub_test_svcore_system \
        sub_test_svcore_data_fileio \
        sub_test_svcore_data_model

SUBDIRS += \
	checker \
	sub_server \
        sub_convert \
	sub_sv
        
sub_base.file = base.pro

sub_os_other.file = os-other.pro
sub_os_win10.file = os-win10.pro

sub_test_svcore_base.file = test-svcore-base.pro
sub_test_svcore_system.file = test-svcore-system.pro
sub_test_svcore_data_fileio.file = test-svcore-data-fileio.pro
sub_test_svcore_data_model.file = test-svcore-data-model.pro

sub_server.file = server.pro
sub_convert.file = convert.pro
sub_sv.file = sv.pro

CONFIG += ordered
