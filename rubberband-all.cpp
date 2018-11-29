
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-value"
#pragma GCC diagnostic ignored "-Wsign-compare"

#define USE_KISSFFT 1
#define USE_SPEEX 1

#ifdef _MSC_VER
#define __MSVC__
#endif
 
#include "rubberband/src/rubberband-c.cpp"
#include "rubberband/src/RubberBandStretcher.cpp"
#include "rubberband/src/StretcherProcess.cpp"
#include "rubberband/src/StretchCalculator.cpp"
#include "rubberband/src/dsp/AudioCurveCalculator.cpp"
#include "rubberband/src/base/Profiler.cpp"
#include "rubberband/src/audiocurves/CompoundAudioCurve.cpp"
#include "rubberband/src/audiocurves/SpectralDifferenceAudioCurve.cpp"
#include "rubberband/src/audiocurves/HighFrequencyAudioCurve.cpp"
#include "rubberband/src/audiocurves/SilentAudioCurve.cpp"
#include "rubberband/src/audiocurves/ConstantAudioCurve.cpp"
#include "rubberband/src/audiocurves/PercussiveAudioCurve.cpp"
#include "rubberband/src/dsp/Resampler.cpp"
#include "rubberband/src/dsp/FFT.cpp"
#include "rubberband/src/system/Allocators.cpp"
#include "rubberband/src/system/sysutils.cpp"
#include "rubberband/src/system/Thread.cpp"
#include "rubberband/src/StretcherChannelData.cpp"
#include "rubberband/src/StretcherImpl.cpp"
#include "rubberband/src/kissfft/kiss_fft.c"
#include "rubberband/src/kissfft/kiss_fftr.c"
#include "rubberband/src/speex/resample.c"

        
