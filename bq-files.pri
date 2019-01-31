
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
        bqfft/bqfft/FFT.h \
	bqresample/bqresample/Resampler.h \
	bqresample/speex/speex_resampler.h \
	bqaudioio/bqaudioio/ApplicationPlaybackSource.h \
	bqaudioio/bqaudioio/ApplicationRecordTarget.h \
	bqaudioio/bqaudioio/AudioFactory.h \
	bqaudioio/bqaudioio/ResamplerWrapper.h \
	bqaudioio/bqaudioio/SystemAudioIO.h \
	bqaudioio/bqaudioio/SystemPlaybackTarget.h \
	bqaudioio/bqaudioio/SystemRecordSource.h \
	bqaudioio/src/DynamicJACK.h \
	bqaudioio/src/JACKAudioIO.h \
	bqaudioio/src/Log.h \
	bqaudioio/src/PortAudioIO.h \
	bqaudioio/src/PulseAudioIO.h \
        bqaudiostream/bqaudiostream/AudioReadStream.h \
        bqaudiostream/bqaudiostream/AudioReadStreamFactory.h \
        bqaudiostream/bqaudiostream/Exceptions.h \
        bqthingfactory/bqthingfactory/ThingFactory.h

BQ_SOURCES += \
	bqvec/src/Allocators.cpp \
	bqvec/src/Barrier.cpp \
	bqvec/src/VectorOpsComplex.cpp \
        bqfft/src/FFT.cpp \
	bqresample/src/Resampler.cpp \
	bqaudioio/src/AudioFactory.cpp \
	bqaudioio/src/JACKAudioIO.cpp \
	bqaudioio/src/Log.cpp \
	bqaudioio/src/PortAudioIO.cpp \
	bqaudioio/src/PulseAudioIO.cpp \
	bqaudioio/src/ResamplerWrapper.cpp \
	bqaudioio/src/SystemPlaybackTarget.cpp \
	bqaudioio/src/SystemRecordSource.cpp \
        bqaudiostream/src/AudioReadStream.cpp \
        bqaudiostream/src/AudioReadStreamFactory.cpp \
        bqaudiostream/src/AudioStreamExceptions.cpp
        
