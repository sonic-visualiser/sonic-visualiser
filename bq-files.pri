
BQ_HEADERS += \
	bqvec/bqvec/Allocators.h \
	bqvec/bqvec/Barrier.h \
	bqvec/bqvec/ComplexTypes.h \
	bqvec/bqvec/Restrict.h \
	bqvec/bqvec/RingBuffer.h \
	bqvec/bqvec/VectorOpsComplex.h \
	bqvec/bqvec/VectorOps.h \
	bqvec/pommier/neon_mathfun.h \
	bqvec/pommier/sse_mathfun.h \
	bqresample/bqresample/Resampler.h \
	bqresample/speex/speex_resampler.h \
	bqaudioio/bqaudioio/ApplicationPlaybackSource.h \
	bqaudioio/bqaudioio/ApplicationRecordTarget.h \
	bqaudioio/bqaudioio/AudioFactory.h \
	bqaudioio/bqaudioio/SystemAudioIO.h \
	bqaudioio/bqaudioio/SystemPlaybackTarget.h \
	bqaudioio/bqaudioio/SystemRecordSource.h \
	bqaudioio/src/DynamicJACK.h \
	bqaudioio/src/JACKAudioIO.h \
	bqaudioio/src/JACKPlaybackTarget.h \
	bqaudioio/src/JACKRecordSource.h \
	bqaudioio/src/PortAudioIO.h \
	bqaudioio/src/PortAudioPlaybackTarget.h \
	bqaudioio/src/PortAudioRecordSource.h \
	bqaudioio/src/PulseAudioIO.h \
	bqaudioio/src/PulseAudioPlaybackTarget.h

BQ_SOURCES += \
	bqvec/src/Allocators.cpp \
	bqvec/src/Barrier.cpp \
	bqvec/src/VectorOpsComplex.cpp \
	bqresample/src/Resampler.cpp \
	bqaudioio/src/AudioFactory.cpp \
	bqaudioio/src/JACKAudioIO.cpp \
	bqaudioio/src/JACKPlaybackTarget.cpp \
	bqaudioio/src/JACKRecordSource.cpp \
	bqaudioio/src/PortAudioIO.cpp \
	bqaudioio/src/PortAudioPlaybackTarget.cpp \
	bqaudioio/src/PortAudioRecordSource.cpp \
	bqaudioio/src/PulseAudioIO.cpp \
	bqaudioio/src/PulseAudioPlaybackTarget.cpp \
	bqaudioio/src/SystemPlaybackTarget.cpp \
	bqaudioio/src/SystemRecordSource.cpp

