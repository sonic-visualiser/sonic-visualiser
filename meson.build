
project(
  'Sonic Visualiser',
  'c', 'cpp',
  version: '5.0',
  license: 'GPL-2.0-or-later',
  default_options: [
    'cpp_std=c++17',
    'warning_level=2',
    'buildtype=release',
    'b_ndebug=if-release',
  ],
  meson_version: '>= 0.57.0'
)

conf = configuration_data()
conf.set('version', meson.project_version())
configure_file(input: 'version.h.in', output: 'version.h', configuration: conf)

qt = import('qt6')

qt_dep = dependency(
  'qt6',
  modules: [
    'Core', 'Gui', 'Widgets', 'Xml', 'Network', 'Svg', 'Test',
  ],
  main: true
)

system = host_machine.system()
architecture = host_machine.cpu_family()
compiler = meson.get_compiler('cpp').get_id()
buildtype = get_option('buildtype')

general_defines = [
  '-DHAVE_PIPER',
  '-DHAVE_PLUGIN_CHECKER_HELPER',
]

general_link_args = []

config_summary = {
  'Compiler': compiler,
  'Architecture': architecture,
  'System': system,
  'Build type': buildtype
}

if buildtype.startswith('release')
  general_defines += [
    '-DNO_TIMING',
    '-DNO_HIT_COUNTS',
  ]
  if compiler == 'clang'
    general_defines += [
      '-Wno-deprecated-copy', # Qt currently hits lots of these internally
    ]
  elif compiler == 'gcc'
    general_defines += [
    ]
  elif compiler == 'msvc'
    general_defines += [
      '-wd4068', # unknown pragma (we have lots of pragma 'GCC')
      '-wd4267', # conversion from size_t to smaller integral type
      '-wd4996', # deprecated functions, including annoying warnings about POSIX
      '-Zc:twoPhase-', # work around incompatibility with some SDKs (CI only)
    ]
  endif # compiler
elif buildtype.startswith('debug')
  general_defines += [
    '-DWANT_TIMING',
    '-DWANT_HIT_COUNTS',
  ]
endif # get_option('buildtype')

svcore_moc_args = []
feature_additional_libs = []
feature_include_dirs = []
server_dependencies = []

dl_dep = meson.get_compiler('c').find_library('dl', required: false)
vamphostsdk_dep = dependency('vamp-hostsdk', version: '>= 2.10', required: false)

if system == 'linux'

  capnp = find_program('capnp')
  capnp_additional_args = []
  capnp_cpp_plugin = 'c++'

  rc = []
  
  bzip2_dep = dependency('bzip2', required: false)

  if not bzip2_dep.found()
    message('Failed to find bzip2 dependency. On many systems this exists but is not discoverable through pkg-config; proceeding on this assumption')
    feature_additional_libs = [ '-lbz2' ]
  endif # bzip2_dep

  fftw3_dep = dependency('fftw3', version: '>= 3.0.0')
  sndfile_dep = dependency('sndfile', version: '>= 1.0.16')
  samplerate_dep = dependency('samplerate', version: '>= 0.1.2')
  rubberband_dep = dependency('rubberband', version: '>= 3.0.0')
  sord_dep = dependency('sord-0', version: '>= 0.5')
  serd_dep = dependency('serd-0', version: '>= 0.5')
  capnp_dep = dependency('capnp', version: '>= 0.6')
  lrdf_dep = dependency('lrdf', version: '>= 0.2')
  oggz_dep = dependency('oggz', version: '>= 1.0.0')
  fishsound_dep = dependency('fishsound', version: '>= 1.0.0')
  mad_dep = dependency('mad', version: '>= 0.15.0')
  id3tag_dep = dependency('id3tag', version: '>= 0.15.0')
  opus_dep = dependency('opusfile')
  opusenc_dep = dependency('opusenc', required: false)
  if not opusenc_dep.found()
    opusenc_dep = dependency('libopusenc', required: false)
  endif
  lo_dep = dependency('liblo')
  jack_dep = dependency('jack', version: '>= 0.100')
  libpulse_dep = dependency('libpulse', version: '>= 0.9')
  alsa_dep = dependency('alsa')
  
  portaudio_dep = dependency('portaudio-2.0', version: '>= 19', required: false)

  feature_dependencies = [
    vamphostsdk_dep,
    bzip2_dep,
    fftw3_dep,
    sndfile_dep,
    samplerate_dep,
    rubberband_dep,
    sord_dep,
    serd_dep,
    capnp_dep,
    lrdf_dep,
    oggz_dep,
    fishsound_dep,
    mad_dep,
    id3tag_dep,
    opus_dep,
    opusenc_dep,
    lo_dep,
    portaudio_dep,
    jack_dep,
    libpulse_dep,
    alsa_dep,
  ]

  server_dependencies = [
    vamphostsdk_dep,
    capnp_dep,
    sord_dep,
    serd_dep,
    dl_dep,
  ]
  
  feature_defines = [
    '-DHAVE_BZ2',
    '-DHAVE_FFTW3',
    '-DFFTW_DOUBLE_ONLY',
    '-DHAVE_SNDFILE',
    '-DHAVE_LIBSAMPLERATE',
    '-DHAVE_RUBBERBAND',
    '-DHAVE_SORD', '-DUSE_SORD',
    '-DHAVE_SERD',
    '-DHAVE_CAPNP',
    '-DHAVE_LRDF',
    '-DHAVE_OGGZ',
    '-DHAVE_FISHSOUND',
    '-DHAVE_MAD',
    '-DHAVE_ID3TAG',
    '-DHAVE_OPUS',
    '-DHAVE_LIBLO',
    '-DHAVE_JACK', '-DDYNAMIC_JACK',
    '-DHAVE_LIBPULSE',
    '-D__LINUX_ALSASEQ__',
    '-D__LINUX_ALSA__' # for RtMidi
  ]

  if portaudio_dep.found()
    feature_defines += [
      '-DHAVE_PORTAUDIO',
    ]
  endif

  if not opusenc_dep.found()
    feature_defines += [
      '-DHAVE_OPUS_READ_ONLY'
    ]
  endif

  svcore_moc_args = [
    '-DHAVE_MAD'
  ]
  
elif system == 'darwin'

  svdeps_dir = meson.current_source_dir() / 'sv-dependency-builds/osx'

  if architecture == 'aarch64'
    svdeps_libdir = svdeps_dir / 'lib-arm64'
  else
    svdeps_libdir = svdeps_dir / 'lib'
  endif
    
  if architecture == 'aarch64'
    capnp = find_program(
      'capnp',
      dirs: [
	svdeps_dir / 'bin-arm64'
      ])
    capnp_additional_args = [
      '-I' + svdeps_dir / 'include-arm64',
      '-I' + svdeps_dir / 'include'
    ]
    capnp_cpp_plugin = svdeps_dir / 'bin-arm64/capnpc-c++'
  else
    capnp = find_program(
      'capnp',
      dirs: [
	svdeps_dir / 'bin'
      ])
    capnp_additional_args = [
      '-I' + svdeps_dir / 'include'
    ]
    capnp_cpp_plugin = svdeps_dir / 'bin/capnpc-c++'
  endif
  
  rc = []

  general_defines += [
    '-mmacosx-version-min=10.15'
  ]
  general_link_args += [
    '-mmacosx-version-min=10.15'
  ]
  
  feature_defines = [
    '-DHAVE_BZ2',
    '-DHAVE_SNDFILE',
    '-DHAVE_LIBSAMPLERATE',
    '-DHAVE_RUBBERBAND',
    '-DHAVE_SORD', '-DUSE_SORD',
    '-DHAVE_SERD',
    '-DHAVE_CAPNP',
    '-DHAVE_OPUS',
    '-DHAVE_OPUS_READ_ONLY',
    '-DHAVE_LIBLO',
    '-DHAVE_PORTAUDIO',
    '-DHAVE_COREAUDIO',
    '-DHAVE_VDSP',
    '-D__MACOSX_CORE__', # for RtMidi
  ]
    
  # vDSP will be used in preference to FFTW most of the time, but
  # FFTW supports more fft lengths - without it we can end up with a
  # very slow implementation for unusual lengths.
  feature_defines += [
    '-DHAVE_FFTW3',
    '-DFFTW_DOUBLE_ONLY',
  ]

  if architecture != 'aarch64'

    # We don't actually need MAD on this platform at all - CoreAudio
    # can read mp3 files no problem - but we have been using it up to
    # now, so for consistency let's stick with it. No need to bring it
    # along to aarch64 with us though.
    feature_defines += [
      '-DHAVE_MAD',
      '-DHAVE_ID3TAG',
    ]
    svcore_moc_args += [
      '-DHAVE_MAD',
    ]
    
  endif
    
  feature_dependencies = [
    vamphostsdk_dep,
  ]

  server_dependencies = [
    vamphostsdk_dep,
    dl_dep,
  ]
  
  feature_include_dirs = [
    # These must be relative, so not using svdeps_dir
    'sv-dependency-builds/osx/include-arm64',
    'sv-dependency-builds/osx/include',
    'sv-dependency-builds/osx/include/opus',
  ]

  feature_additional_libs = [
    '-L' + svdeps_libdir,
    '-lbz2',
    '-lrubberband',
    '-lsndfile',
    '-lFLAC',
    '-logg',
    '-lvorbis',
    '-lvorbisenc',
    '-lvorbisfile',
    '-lopusfile',
    '-lopus',
    '-lportaudio',
    '-lsamplerate',
    '-lfftw3',
    '-lz',
    '-lsord-0',
    '-lserd-0',
    '-llo',
    '-lcapnp',
    '-lkj',
    '-framework', 'CoreAudio',
    '-framework', 'CoreMidi',
    '-framework', 'AudioUnit',
    '-framework', 'AudioToolbox',
    '-framework', 'CoreFoundation',
    '-framework', 'CoreServices',
    '-framework', 'Accelerate',
  ]

  if architecture != 'aarch64'
    # See comments when setting up feature_defines above
    feature_additional_libs += [
      '-lmad',
      '-lid3tag',
    ]
  endif
  
elif system == 'windows'

  if architecture == 'x86'
    svdeps_dir = meson.current_source_dir() / 'sv-dependency-builds/win32-mingw'
  else 
    svdeps_dir = meson.current_source_dir() / 'sv-dependency-builds/win64-msvc'
  endif # architecture
  
  capnp = find_program(
    'capnp',
    dirs: [
      svdeps_dir / 'bin'
    ])
  capnp_additional_args = [
    '-I' + svdeps_dir / 'include'
  ]
  capnp_cpp_plugin = svdeps_dir / 'bin/capnpc-c++'

  windows = import('windows')
  rc = windows.compile_resources('icons/sv.rc')

  general_defines += [
    '-DWIN32',
    '-DNOMINMAX',
    '-DUNICODE',
    '-D_USE_MATH_DEFINES',
    '-DCAPNP_LITE',
    '-D_HAS_STD_BYTE=1',
  ]
  
  feature_defines = [
    '-DHAVE_BZ2',
    '-DHAVE_FFTW3',
    '-DFFTW_DOUBLE_ONLY',
    '-DHAVE_SNDFILE',
    '-DHAVE_LIBSAMPLERATE',
    '-DHAVE_RUBBERBAND',
    '-DHAVE_SORD', '-DUSE_SORD',
    '-DHAVE_SERD',
    '-DHAVE_CAPNP',
    '-DHAVE_MAD',
    '-DHAVE_ID3TAG',
    '-DHAVE_OPUS',
    '-DHAVE_OPUS_READ_ONLY',
    '-DHAVE_PORTAUDIO',
    '-D__WINDOWS_MM__', # for RtMidi
  ]

  feature_dependencies = [
    vamphostsdk_dep,
  ]

  server_dependencies = [
    vamphostsdk_dep,
    dl_dep,
  ]
  
  if architecture == 'x86'
    feature_include_dirs = [
      # These must be relative, so not using svdeps_dir
      'sv-dependency-builds/win32-mingw/include',
      'sv-dependency-builds/win32-mingw/include/opus',
    ]
  else
    feature_defines += [
      '-DHAVE_MEDIAFOUNDATION',
    ]
    feature_include_dirs = [
      # These must be relative, so not using svdeps_dir
      'sv-dependency-builds/win64-msvc/include',
      'sv-dependency-builds/win64-msvc/include/opus',
    ]
  endif # architecture

  if buildtype.startswith('release')
    feature_additional_libpaths =  [
      '-L' + svdeps_dir / 'lib',
    ]
  else
    feature_additional_libpaths =  [
      '-L' + svdeps_dir / 'lib/debug',
      '-L' + svdeps_dir / 'lib',
    ]
  endif
  
  feature_additional_libs = [
    feature_additional_libpaths,
    '-lbz2',
    '-lrubberband',
    '-lfftw3',
    '-lsndfile',
    '-logg',
    '-lopusfile',
    '-lopus',
    '-lmad',
    '-lid3tag',
    '-lportaudio',
    '-lsamplerate',
    '-lz',
    '-lcapnp',
    '-lkj',
  ]

  if architecture == 'x86'
    feature_additional_libs += [
      '-lFLAC',
      '-lvorbis',
      '-lvorbisenc',
      '-lvorbisfile',
      '-lsord-0',
      '-lserd-0',
      # (We don't have MediaFoundation support with this build sadly)
      '-lwinmm',
      '-lws2_32',
    ]
  elif architecture == 'x86_64'
    feature_additional_libs += [
#      '-NODEFAULTLIB:LIBCMT',
      '-lsord',
      '-lserd',
      '-lmfplat',
      '-lmfreadwrite',
      '-lmfuuid',
      '-lpropsys',
      '-ladvapi32',
      '-lwinmm',
      '-lws2_32',
    ]
  else 
    error('Build for architecture ' + architecture + ' is not supported on this platform')
  endif # architecture
  
  svcore_moc_args = [
    '-DHAVE_MAD',
    '-DQ_OS_WIN',
  ]

else
    error('This operating system ("' + system + '") is not supported by this build file')
endif # system
    
all_include_dirs = include_directories(
  [
    'bqvec',
    'bqvec/bqvec',
    'bqfft',
    'bqresample',
    'bqaudioio',
    'bqaudioio/bqaudioio',
    'bqaudiostream',
    'bqaudiostream/bqaudiostream',
    'bqthingfactory',
    'piper-vamp-cpp',
    'checker',
    'checker/checker',
    'dataquay',
    'dataquay/dataquay',
    'svcore',
    'svcore/data',
    'svcore/plugin/api/alsa',
    'svcore',
    'svgui',
    'svapp',
    'vamp-plugin-sdk',
  ]
)

capnp_gen_target = custom_target(
  'capnp_gen',
  # The input must be absolute, or (the version of capnp I have here)
  # writes output into the wrong directory!
  input: meson.current_source_dir() / 'piper/capnp/piper.capnp',
  output: ['piper.capnp.h', 'piper.capnp.c++'],
  command: [
    capnp,
    capnp_additional_args,
    'compile',
    '--src-prefix=' + meson.current_source_dir() / 'piper/capnp',
    '-o' + capnp_cpp_plugin + ':@OUTDIR@',
    '@INPUT@'
  ])

capnp_gen_header_dep = declare_dependency(
  sources: capnp_gen_target[0]
)

if system == 'windows'

  # This nonsense is just because MSVC CL won't accept files with a
  # .c++ extension (unless you can somehow finagle the /tp option into
  # the right place in the args list). We copy the .c++ file generated
  # by capnpc-c++ to a .cpp file and use that instead
  
  capnp_gen_cpp_target = custom_target(
    'capnp_gen_cpp',
    input: capnp_gen_target[1],
    output: 'piper.capnp.cpp',
    command: [
      'xcopy',
      '@INPUT@',
      '@OUTPUT@' + '*',
    ])

else

  # Unnecessary on non-Windows platforms, but for consistency...
  
  capnp_gen_cpp_target = custom_target(
    'capnp_gen_cpp',
    input: capnp_gen_target[1],
    output: 'piper.capnp.cpp',
    command: [
      'cp',
      '@INPUT@',
      '@OUTPUT@',
    ])

endif  

bq_files = [
  'bqvec/src/Allocators.cpp',
  'bqvec/src/Barrier.cpp',
  'bqvec/src/VectorOpsComplex.cpp',
  'bqfft/src/FFT.cpp',
  'bqresample/src/Resampler.cpp',
  'bqaudioio/src/AudioFactory.cpp',
  'bqaudioio/src/JACKAudioIO.cpp',
  'bqaudioio/src/Log.cpp',
  'bqaudioio/src/PortAudioIO.cpp',
  'bqaudioio/src/PulseAudioIO.cpp',
  'bqaudioio/src/ResamplerWrapper.cpp',
  'bqaudioio/src/SystemPlaybackTarget.cpp',
  'bqaudioio/src/SystemRecordSource.cpp',
  'bqaudiostream/src/AudioReadStream.cpp',
  'bqaudiostream/src/AudioReadStreamFactory.cpp',
  'bqaudiostream/src/AudioWriteStreamFactory.cpp',
  'bqaudiostream/src/AudioStreamExceptions.cpp',
]

if vamphostsdk_dep.found()
  config_summary += { 'Vamp SDK': 'system' }
  vamphostsdk_files = []
else
  config_summary += { 'Vamp SDK': 'bundled' }
  vamphostsdk_files = [
    'vamp-plugin-sdk/src/vamp-hostsdk/PluginBufferingAdapter.cpp',
    'vamp-plugin-sdk/src/vamp-hostsdk/PluginChannelAdapter.cpp',
    'vamp-plugin-sdk/src/vamp-hostsdk/PluginHostAdapter.cpp',
    'vamp-plugin-sdk/src/vamp-hostsdk/PluginInputDomainAdapter.cpp',
    'vamp-plugin-sdk/src/vamp-hostsdk/PluginLoader.cpp',
    'vamp-plugin-sdk/src/vamp-hostsdk/PluginSummarisingAdapter.cpp',
    'vamp-plugin-sdk/src/vamp-hostsdk/PluginWrapper.cpp',
    'vamp-plugin-sdk/src/vamp-hostsdk/RealTime.cpp',
    'vamp-plugin-sdk/src/vamp-hostsdk/Files.cpp',
  ]
endif

dataquay_files = [
  'dataquay/src/Connection.cpp',
  'dataquay/src/Node.cpp',
  'dataquay/src/PropertyObject.cpp',
  'dataquay/src/RDFException.cpp',
  'dataquay/src/Store.cpp',
  'dataquay/src/Transaction.cpp',
  'dataquay/src/TransactionalStore.cpp',
  'dataquay/src/Triple.cpp',
  'dataquay/src/Uri.cpp',
  'dataquay/src/backend/BasicStoreRedland.cpp',
  'dataquay/src/backend/BasicStoreSord.cpp',
  'dataquay/src/backend/define-check.cpp',
  'dataquay/src/objectmapper/ContainerBuilder.cpp',
  'dataquay/src/objectmapper/ObjectBuilder.cpp',
  'dataquay/src/objectmapper/ObjectLoader.cpp',
  'dataquay/src/objectmapper/ObjectMapper.cpp',
  'dataquay/src/objectmapper/ObjectMapperForwarder.cpp',
  'dataquay/src/objectmapper/ObjectStorer.cpp',
  'dataquay/src/objectmapper/TypeMapping.cpp',
]

dataquay_moc_files = qt.preprocess(
  moc_headers: [
  'dataquay/dataquay/Connection.h',
  'dataquay/dataquay/TransactionalStore.h',
  'dataquay/dataquay/objectmapper/ObjectMapper.h',
  'dataquay/dataquay/objectmapper/ObjectMapperForwarder.h',
])

svcore_files = [
  'svcore/base/AudioLevel.cpp',
  'svcore/base/ById.cpp',
  'svcore/base/Clipboard.cpp',
  'svcore/base/ColumnOp.cpp',
  'svcore/base/Command.cpp',
  'svcore/base/Debug.cpp',
  'svcore/base/EventSeries.cpp',
  'svcore/base/Exceptions.cpp',
  'svcore/base/HelperExecPath.cpp',
  'svcore/base/HitCount.cpp',
  'svcore/base/LogRange.cpp',
  'svcore/base/Pitch.cpp',
  'svcore/base/PlayParameterRepository.cpp',
  'svcore/base/PlayParameters.cpp',
  'svcore/base/Preferences.cpp',
  'svcore/base/Profiler.cpp',
  'svcore/base/ProgressPrinter.cpp',
  'svcore/base/ProgressReporter.cpp',
  'svcore/base/PropertyContainer.cpp',
  'svcore/base/RangeMapper.cpp',
  'svcore/base/RealTimeSV.cpp',
  'svcore/base/RecentFiles.cpp',
  'svcore/base/RecordDirectory.cpp',
  'svcore/base/ResourceFinder.cpp',
  'svcore/base/Selection.cpp',
  'svcore/base/Serialiser.cpp',
  'svcore/base/StorageAdviser.cpp',
  'svcore/base/StringBits.cpp',
  'svcore/base/Strings.cpp',
  'svcore/base/TempDirectory.cpp',
  'svcore/base/TempWriteFile.cpp',
  'svcore/base/TextMatcher.cpp',
  'svcore/base/Thread.cpp',
  'svcore/base/UnitDatabase.cpp',
  'svcore/base/ViewManagerBase.cpp',
  'svcore/base/XmlExportable.cpp',
  'svcore/base/ZoomLevel.cpp',
  'svcore/data/fileio/AudioFileReader.cpp',
  'svcore/data/fileio/AudioFileReaderFactory.cpp',
  'svcore/data/fileio/AudioFileSizeEstimator.cpp',
  'svcore/data/fileio/BQAFileReader.cpp',
  'svcore/data/fileio/BZipFileDevice.cpp',
  'svcore/data/fileio/CachedFile.cpp',
  'svcore/data/fileio/CodedAudioFileReader.cpp',
  'svcore/data/fileio/CSVFileReader.cpp',
  'svcore/data/fileio/CSVFileWriter.cpp',
  'svcore/data/fileio/CSVFormat.cpp',
  'svcore/data/fileio/DataFileReaderFactory.cpp',
  'svcore/data/fileio/DecodingWavFileReader.cpp',
  'svcore/data/fileio/FileReadThread.cpp',
  'svcore/data/fileio/FileSource.cpp',
  'svcore/data/fileio/MIDIFileReader.cpp',
  'svcore/data/fileio/MIDIFileWriter.cpp',
  'svcore/data/fileio/MP3FileReader.cpp',
  'svcore/data/fileio/PlaylistFileReader.cpp',
  'svcore/data/fileio/TextTest.cpp',
  'svcore/data/fileio/WavFileReader.cpp',
  'svcore/data/fileio/WavFileWriter.cpp',
  'svcore/data/midi/MIDIInput.cpp',
  'svcore/data/midi/rtmidi/RtMidi.cpp',
  'svcore/data/model/AggregateWaveModel.cpp',
  'svcore/data/model/AlignmentModel.cpp',
  'svcore/data/model/BasicCompressedDenseThreeDimensionalModel.cpp',
  'svcore/data/model/Dense3DModelPeakCache.cpp',
  'svcore/data/model/DenseTimeValueModel.cpp',
  'svcore/data/model/EditableDenseThreeDimensionalModel.cpp',
  'svcore/data/model/FFTModel.cpp',
  'svcore/data/model/Model.cpp',
  'svcore/data/model/ModelDataTableModel.cpp',
  'svcore/data/model/PowerOfSqrtTwoZoomConstraint.cpp',
  'svcore/data/model/PowerOfTwoZoomConstraint.cpp',
  'svcore/data/model/RangeSummarisableTimeValueModel.cpp',
  'svcore/data/model/RelativelyFineZoomConstraint.cpp',
  'svcore/data/model/WaveformOversampler.cpp',
  'svcore/data/model/WaveFileModel.cpp',
  'svcore/data/model/ReadOnlyWaveFileModel.cpp',
  'svcore/data/model/WritableWaveFileModel.cpp',
  'svcore/data/osc/OSCMessage.cpp',
  'svcore/data/osc/OSCQueue.cpp',
  'svcore/plugin/PluginScan.cpp',
  'svcore/plugin/DSSIPluginFactory.cpp',
  'svcore/plugin/DSSIPluginInstance.cpp',
  'svcore/plugin/FeatureExtractionPluginFactory.cpp',
  'svcore/plugin/LADSPAPluginFactory.cpp',
  'svcore/plugin/LADSPAPluginInstance.cpp',
  'svcore/plugin/NativeVampPluginFactory.cpp',
  'svcore/plugin/PiperVampPluginFactory.cpp',
  'svcore/plugin/PluginIdentifier.cpp',
  'svcore/plugin/PluginPathSetter.cpp',
  'svcore/plugin/PluginXml.cpp',
  'svcore/plugin/RealTimePluginFactory.cpp',
  'svcore/plugin/RealTimePluginInstance.cpp',
  'svcore/plugin/plugins/SamplePlayer.cpp',
  'svcore/rdf/PluginRDFIndexer.cpp',
  'svcore/rdf/PluginRDFDescription.cpp',
  'svcore/rdf/RDFExporter.cpp',
  'svcore/rdf/RDFFeatureWriter.cpp',
  'svcore/rdf/RDFImporter.cpp',
  'svcore/rdf/RDFTransformFactory.cpp',
  'svcore/system/Init.cpp',
  'svcore/system/System.cpp',
  'svcore/transform/CSVFeatureWriter.cpp',
  'svcore/transform/FeatureExtractionModelTransformer.cpp',
  'svcore/transform/FileFeatureWriter.cpp',
  'svcore/transform/RealTimeEffectModelTransformer.cpp',
  'svcore/transform/Transform.cpp',
  'svcore/transform/TransformFactory.cpp',
  'svcore/transform/ModelTransformer.cpp',
  'svcore/transform/ModelTransformerFactory.cpp',
]

if compiler != 'msvc' # i.e. build is not the 64-bit Windows one
  svcore_files += [
    'svcore/system/os-other.cpp',
  ]
endif

svcore_moc_files = qt.preprocess(
  moc_headers: [
  'svcore/base/Command.h',
  'svcore/base/PlayParameterRepository.h',
  'svcore/base/PlayParameters.h',
  'svcore/base/Preferences.h',
  'svcore/base/ProgressPrinter.h',
  'svcore/base/ProgressReporter.h',
  'svcore/base/PropertyContainer.h',
  'svcore/base/RecentFiles.h',
  'svcore/base/Thread.h',
  'svcore/base/UnitDatabase.h',
  'svcore/base/ViewManagerBase.h',
  'svcore/data/fileio/AudioFileReader.h',
  'svcore/data/fileio/BQAFileReader.h',
  'svcore/data/fileio/BZipFileDevice.h',
  'svcore/data/fileio/CodedAudioFileReader.h',
  'svcore/data/fileio/CSVFileWriter.h',
  'svcore/data/fileio/DecodingWavFileReader.h',
  'svcore/data/fileio/FileReadThread.h',
  'svcore/data/fileio/FileSource.h',
  'svcore/data/fileio/MIDIFileReader.h',
  'svcore/data/fileio/MP3FileReader.h',
  'svcore/data/midi/MIDIInput.h',
  'svcore/data/model/AggregateWaveModel.h',
  'svcore/data/model/AlignmentModel.h',
  'svcore/data/model/BasicCompressedDenseThreeDimensionalModel.h',
  'svcore/data/model/Dense3DModelPeakCache.h',
  'svcore/data/model/DenseThreeDimensionalModel.h',
  'svcore/data/model/DenseTimeValueModel.h',
  'svcore/data/model/EditableDenseThreeDimensionalModel.h',
  'svcore/data/model/FFTModel.h',
  'svcore/data/model/ImageModel.h',
  'svcore/data/model/Labeller.h',
  'svcore/data/model/Model.h',
  'svcore/data/model/ModelDataTableModel.h',
  'svcore/data/model/NoteModel.h',
  'svcore/data/model/RangeSummarisableTimeValueModel.h',
  'svcore/data/model/RegionModel.h',
  'svcore/data/model/SparseOneDimensionalModel.h',
  'svcore/data/model/SparseTimeValueModel.h',
  'svcore/data/model/TextModel.h',
  'svcore/data/model/BoxModel.h',
  'svcore/data/model/WaveFileModel.h',
  'svcore/data/model/ReadOnlyWaveFileModel.h',
  'svcore/data/model/WritableWaveFileModel.h',
  'svcore/data/osc/OSCQueue.h',
  'svcore/rdf/RDFImporter.h',
  'svcore/rdf/RDFTransformFactory.h',
  'svcore/transform/FeatureExtractionModelTransformer.h',
  'svcore/transform/TransformFactory.h',
  'svcore/transform/ModelTransformerFactory.h',
],
  moc_extra_arguments: svcore_moc_args,
)

svcore_lib = static_library(
  'svcore',
  capnp_gen_cpp_target,
  bq_files,
  vamphostsdk_files,
  dataquay_moc_files,
  svcore_moc_files,
  dataquay_files,
  svcore_files,
  include_directories: [
    all_include_dirs,
    feature_include_dirs,
  ],
  dependencies: [
    capnp_gen_header_dep,
    qt_dep,
    feature_dependencies,
    dl_dep,
  ],
  cpp_args: [
    feature_defines,
    general_defines,
  ],
)

svcore_dep = declare_dependency(
  link_with: svcore_lib,
  dependencies: [
    capnp_gen_header_dep,
  ],
  include_directories: [
    all_include_dirs,
    feature_include_dirs,
  ],
)

svcore_base_test_moc_files = qt.preprocess(
  moc_headers: [
  'svcore/base/test/TestById.h',
  'svcore/base/test/TestColumnOp.h',
  'svcore/base/test/TestLogRange.h',
  'svcore/base/test/TestMovingMedian.h',
  'svcore/base/test/TestOurRealTime.h',
  'svcore/base/test/TestPitch.h',
  'svcore/base/test/TestEventSeries.h',
  'svcore/base/test/TestRangeMapper.h',
  'svcore/base/test/TestScaleTickIntervals.h',
  'svcore/base/test/TestStringBits.h',
  'svcore/base/test/TestVampRealTime.h',
  'svcore/base/test/StressEventSeries.h',
],
  moc_extra_arguments: svcore_moc_args,
)

svcore_system_test_moc_files = qt.preprocess(
  moc_headers: [
  'svcore/system/test/TestEnv.h',
])

svcore_data_model_test_moc_files = qt.preprocess(
  moc_headers: [
  'svcore/data/model/test/MockWaveModel.h',
  'svcore/data/model/test/TestFFTModel.h',
  'svcore/data/model/test/TestSparseModels.h',
  'svcore/data/model/test/TestWaveformOversampler.h',
  'svcore/data/model/test/TestZoomConstraints.h',
])

svcore_data_fileio_test_moc_files = qt.preprocess(
  moc_headers: [
  'svcore/data/model/test/MockWaveModel.h',
  'svcore/data/fileio/test/AudioFileReaderTest.h',
  'svcore/data/fileio/test/BogusAudioFileReaderTest.h',
  'svcore/data/fileio/test/AudioFileWriterTest.h',
  'svcore/data/fileio/test/EncodingTest.h',
  'svcore/data/fileio/test/MIDIFileReaderTest.h',
  'svcore/data/fileio/test/CSVFormatTest.h',
  'svcore/data/fileio/test/CSVReaderTest.h',
  'svcore/data/fileio/test/CSVStreamWriterTest.h',
])

svgui_files = [
  'svgui/layer/Colour3DPlotExporter.cpp',
  'svgui/layer/Colour3DPlotLayer.cpp',
  'svgui/layer/Colour3DPlotRenderer.cpp',
  'svgui/layer/ColourDatabase.cpp',
  'svgui/layer/ColourMapper.cpp',
  'svgui/layer/ColourScale.cpp',
  'svgui/layer/FlexiNoteLayer.cpp',
  'svgui/layer/HorizontalFrequencyScale.cpp',
  'svgui/layer/ImageLayer.cpp',
  'svgui/layer/ImageRegionFinder.cpp',
  'svgui/layer/Layer.cpp',
  'svgui/layer/LayerFactory.cpp',
  'svgui/layer/LinearNumericalScale.cpp',
  'svgui/layer/LogNumericalScale.cpp',
  'svgui/layer/LinearColourScale.cpp',
  'svgui/layer/LogColourScale.cpp',
  'svgui/layer/NoteLayer.cpp',
  'svgui/layer/PaintAssistant.cpp',
  'svgui/layer/PianoScale.cpp',
  'svgui/layer/RegionLayer.cpp',
  'svgui/layer/ScrollableImageCache.cpp',
  'svgui/layer/ScrollableMagRangeCache.cpp',
  'svgui/layer/SingleColourLayer.cpp',
  'svgui/layer/SliceLayer.cpp',
  'svgui/layer/SpectrogramLayer.cpp',
  'svgui/layer/SpectrumLayer.cpp',
  'svgui/layer/TextLayer.cpp',
  'svgui/layer/TimeInstantLayer.cpp',
  'svgui/layer/BoxLayer.cpp',
  'svgui/layer/TimeRulerLayer.cpp',
  'svgui/layer/TimeValueLayer.cpp',
  'svgui/layer/WaveformLayer.cpp',
  'svgui/view/AlignmentView.cpp',
  'svgui/view/Overview.cpp',
  'svgui/view/Pane.cpp',
  'svgui/view/PaneStack.cpp',
  'svgui/view/View.cpp',
  'svgui/view/ViewManager.cpp',
  'svgui/widgets/ActivityLog.cpp',
  'svgui/widgets/AudioDial.cpp',
  'svgui/widgets/ColourComboBox.cpp',
  'svgui/widgets/ColourMapComboBox.cpp',
  'svgui/widgets/ColourNameDialog.cpp',
  'svgui/widgets/CommandHistory.cpp',
  'svgui/widgets/CSVAudioFormatDialog.cpp',
  'svgui/widgets/CSVExportDialog.cpp',
  'svgui/widgets/CSVFormatDialog.cpp',
  'svgui/widgets/InteractiveFileFinder.cpp',
  'svgui/widgets/IconLoader.cpp',
  'svgui/widgets/ImageDialog.cpp',
  'svgui/widgets/ItemEditDialog.cpp',
  'svgui/widgets/KeyReference.cpp',
  'svgui/widgets/LabelCounterInputDialog.cpp',
  'svgui/widgets/LayerTree.cpp',
  'svgui/widgets/LayerTreeDialog.cpp',
  'svgui/widgets/LEDButton.cpp',
  'svgui/widgets/LevelPanToolButton.cpp',
  'svgui/widgets/LevelPanWidget.cpp',
  'svgui/widgets/ListInputDialog.cpp',
  'svgui/widgets/MIDIFileImportDialog.cpp',
  'svgui/widgets/ModelDataTableDialog.cpp',
  'svgui/widgets/NotifyingCheckBox.cpp',
  'svgui/widgets/NotifyingComboBox.cpp',
  'svgui/widgets/NotifyingPushButton.cpp',
  'svgui/widgets/NotifyingTabBar.cpp',
  'svgui/widgets/NotifyingToolButton.cpp',
  'svgui/widgets/Panner.cpp',
  'svgui/widgets/PluginParameterBox.cpp',
  'svgui/widgets/PluginParameterDialog.cpp',
  'svgui/widgets/PluginPathConfigurator.cpp',
  'svgui/widgets/PluginReviewDialog.cpp',
  'svgui/widgets/ProgressDialog.cpp',
  'svgui/widgets/PropertyBox.cpp',
  'svgui/widgets/PropertyStack.cpp',
  'svgui/widgets/RangeInputDialog.cpp',
  'svgui/widgets/SelectableLabel.cpp',
  'svgui/widgets/SubdividingMenu.cpp',
  'svgui/widgets/TextAbbrev.cpp',
  'svgui/widgets/Thumbwheel.cpp',
  'svgui/widgets/TransformFinder.cpp',
  'svgui/widgets/UnitConverter.cpp',
  'svgui/widgets/WindowShapePreview.cpp',
  'svgui/widgets/WindowTypeSelector.cpp',
]

svgui_moc_files = qt.preprocess(
  moc_headers: [
  'svgui/layer/Colour3DPlotExporter.h',
  'svgui/layer/Colour3DPlotLayer.h',
  'svgui/layer/ColourDatabase.h',
  'svgui/layer/FlexiNoteLayer.h',
  'svgui/layer/ImageLayer.h',
  'svgui/layer/Layer.h',
  'svgui/layer/NoteLayer.h',
  'svgui/layer/RegionLayer.h',
  'svgui/layer/SingleColourLayer.h',
  'svgui/layer/SliceableLayer.h',
  'svgui/layer/SliceLayer.h',
  'svgui/layer/SpectrogramLayer.h',
  'svgui/layer/SpectrumLayer.h',
  'svgui/layer/TextLayer.h',
  'svgui/layer/TimeInstantLayer.h',
  'svgui/layer/BoxLayer.h',
  'svgui/layer/TimeRulerLayer.h',
  'svgui/layer/TimeValueLayer.h',
  'svgui/layer/WaveformLayer.h',
  'svgui/view/AlignmentView.h',
  'svgui/view/Overview.h',
  'svgui/view/Pane.h',
  'svgui/view/PaneStack.h',
  'svgui/view/View.h',
  'svgui/view/ViewManager.h',
  'svgui/widgets/ActivityLog.h',
  'svgui/widgets/AudioDial.h',
  'svgui/widgets/ClickableLabel.h',
  'svgui/widgets/ColourComboBox.h',
  'svgui/widgets/ColourMapComboBox.h',
  'svgui/widgets/ColourNameDialog.h',
  'svgui/widgets/CommandHistory.h',
  'svgui/widgets/CSVAudioFormatDialog.h',
  'svgui/widgets/CSVExportDialog.h',
  'svgui/widgets/CSVFormatDialog.h',
  'svgui/widgets/InteractiveFileFinder.h',
  'svgui/widgets/ImageDialog.h',
  'svgui/widgets/ItemEditDialog.h',
  'svgui/widgets/KeyReference.h',
  'svgui/widgets/LabelCounterInputDialog.h',
  'svgui/widgets/LayerTree.h',
  'svgui/widgets/LayerTreeDialog.h',
  'svgui/widgets/LEDButton.h',
  'svgui/widgets/LevelPanToolButton.h',
  'svgui/widgets/LevelPanWidget.h',
  'svgui/widgets/ListInputDialog.h',
  'svgui/widgets/MIDIFileImportDialog.h',
  'svgui/widgets/ModelDataTableDialog.h',
  'svgui/widgets/NotifyingCheckBox.h',
  'svgui/widgets/NotifyingComboBox.h',
  'svgui/widgets/NotifyingPushButton.h',
  'svgui/widgets/NotifyingTabBar.h',
  'svgui/widgets/NotifyingToolButton.h',
  'svgui/widgets/Panner.h',
  'svgui/widgets/PluginParameterBox.h',
  'svgui/widgets/PluginParameterDialog.h',
  'svgui/widgets/PluginPathConfigurator.h',
  'svgui/widgets/PluginReviewDialog.h',
  'svgui/widgets/ProgressDialog.h',
  'svgui/widgets/PropertyBox.h',
  'svgui/widgets/PropertyStack.h',
  'svgui/widgets/RangeInputDialog.h',
  'svgui/widgets/SelectableLabel.h',
  'svgui/widgets/SubdividingMenu.h',
  'svgui/widgets/Thumbwheel.h',
  'svgui/widgets/TransformFinder.h',
  'svgui/widgets/UnitConverter.h',
  'svgui/widgets/WindowShapePreview.h',
  'svgui/widgets/WindowTypeSelector.h',
])
  
svapp_files = [
  'svapp/align/Align.cpp',
  'svapp/align/ExternalProgramAligner.cpp',
  'svapp/align/LinearAligner.cpp',
  'svapp/align/MATCHAligner.cpp',
  'svapp/align/TransformDTWAligner.cpp',
  'svapp/audio/AudioCallbackPlaySource.cpp',
  'svapp/audio/AudioCallbackRecordTarget.cpp',
  'svapp/audio/AudioGenerator.cpp',
  'svapp/audio/ClipMixer.cpp',
  'svapp/audio/ContinuousSynth.cpp',
  'svapp/audio/EffectWrapper.cpp',
  'svapp/audio/PlaySpeedRangeMapper.cpp',
  'svapp/audio/TimeStretchWrapper.cpp',
  'svapp/framework/Document.cpp',
  'svapp/framework/MainWindowBase.cpp',
  'svapp/framework/SVFileReader.cpp',
  'svapp/framework/TransformUserConfigurator.cpp',
  'svapp/framework/VersionTester.cpp',
]

svapp_moc_files = qt.preprocess(
  moc_headers: [
  'svapp/align/Align.h',
  'svapp/align/Aligner.h',
  'svapp/align/ExternalProgramAligner.h',
  'svapp/align/LinearAligner.h',
  'svapp/align/MATCHAligner.h',
  'svapp/align/TransformDTWAligner.h',
  'svapp/audio/AudioCallbackPlaySource.h',
  'svapp/audio/AudioCallbackRecordTarget.h',
  'svapp/audio/AudioGenerator.h',
  'svapp/framework/Document.h',
  'svapp/framework/MainWindowBase.h',
  'svapp/framework/OSCScript.h',
  'svapp/framework/SVFileReader.h',
  'svapp/framework/VersionTester.h',
])

sv_main_files = [
  'main/main.cpp',
  'main/OSCHandler.cpp',
  'main/MainWindow.cpp',
  'main/NetworkPermissionTester.cpp',
  'main/Surveyer.cpp',
  'main/SVSplash.cpp',
  'main/PreferencesDialog.cpp',
]

sv_main_moc_files = qt.preprocess(
  moc_headers: [
  'main/MainWindow.h',
  'main/Surveyer.h',
  'main/SVSplash.h',
  'main/PreferencesDialog.h',
])

checker_lib_files = [
  'checker/src/plugincandidates.cpp',
  'checker/src/knownplugincandidates.cpp',
  'checker/src/knownplugins.cpp',
]

qt_resource_files = qt.preprocess(
  qresources: [
  'sonic-visualiser.qrc',
])

if compiler == 'msvc'
  os = shared_library(
    'os',
    [ 'svcore/system/os-win10.cpp' ],
    cpp_args: [
      general_defines,
    ],
    link_args: [
      '-EXPORT:OSReportsDarkThemeActive',
      '-EXPORT:OSQueryAccentColour',
      '-lWindowsApp',
      general_link_args,
    ])
  shared_library(
    'os_other',
    [ 'svcore/system/os-other.cpp' ],
    cpp_args: [
      general_defines,
    ],
    link_args: [
      '-EXPORT:OSReportsDarkThemeActive',
      '-EXPORT:OSQueryAccentColour',
      general_link_args,
    ])
  os_dep = declare_dependency(
    link_with: os
  )
else
  os_dep = []
endif

if system == 'linux'
  sv_main_name = 'sonic-visualiser'
else
  sv_main_name = 'Sonic Visualiser'
endif # system

install_data(
  'sonic-visualiser.desktop',
  install_dir : get_option('datadir') / 'applications',
)

install_data(
  'icons/sonic-visualiser.svg',
  install_dir : get_option('datadir') / 'icons/hicolor/scalable/apps',
)

executable(
  sv_main_name,
  qt_resource_files,
  svgui_moc_files,
  svapp_moc_files,
  sv_main_moc_files,
  svgui_files,
  svapp_files,
  checker_lib_files,
  sv_main_files,
  rc,
  dependencies: [
    svcore_dep,
    qt_dep,
    feature_dependencies,
    os_dep,
    dl_dep
  ],
  cpp_args: [
    feature_defines,
    general_defines,
  ],
  link_args: [
    feature_additional_libs,
    general_link_args,
  ],
  win_subsystem: 'windows',
  install: true,
)

svcore_base_test_exe = executable(
  'test-svcore-base',
  svcore_base_test_moc_files,
  'svcore/base/test/svcore-base-test.cpp',
  dependencies: [
    svcore_dep,
    qt_dep,
    feature_dependencies,
    dl_dep,
  ],
  cpp_args: [
    feature_defines,
    general_defines,
  ],
  link_args: [
    feature_additional_libs,
    general_link_args,
  ],
  win_subsystem: 'console'
)

svcore_system_test_exe = executable(
  'test-svcore-system',
  svcore_system_test_moc_files,
  'svcore/system/test/svcore-system-test.cpp',
  dependencies: [
    svcore_dep,
    qt_dep,
    feature_dependencies,
    dl_dep,
  ],
  cpp_args: [
    feature_defines,
    general_defines,
  ],
  link_args: [
    feature_additional_libs,
    general_link_args,
  ],
  win_subsystem: 'console'
)

svcore_data_model_test_exe = executable(
  'test-svcore-data-model',
  svcore_data_model_test_moc_files,
  'svcore/data/model/test/MockWaveModel.cpp',
  'svcore/data/model/test/svcore-data-model-test.cpp',
  dependencies: [
    svcore_dep,
    qt_dep,
    feature_dependencies,
    dl_dep,
  ],
  cpp_args: [
    feature_defines,
    general_defines,
  ],
  link_args: [
    feature_additional_libs,
    general_link_args,
  ],
  win_subsystem: 'console'
)

svcore_data_fileio_test_exe = executable(
  'test-svcore-data-fileio',
  svcore_data_fileio_test_moc_files,
  'svcore/data/model/test/MockWaveModel.cpp',
  'svcore/data/fileio/test/UnsupportedFormat.cpp',
  'svcore/data/fileio/test/svcore-data-fileio-test.cpp',
  dependencies: [
    svcore_dep,
    qt_dep,
    feature_dependencies,
    dl_dep,
  ],
  cpp_args: [
    feature_defines,
    general_defines,
  ],
  link_args: [
    feature_additional_libs,
    general_link_args,
  ],
  win_subsystem: 'console'
)

test('svcore-base', svcore_base_test_exe)
test('svcore-system', svcore_system_test_exe)
test('svcore-data-model', svcore_data_model_test_exe)
test('svcore-data-fileio', svcore_data_fileio_test_exe,
     args: [
       '--testdir', meson.current_source_dir() / 'svcore/data/fileio/test'
     ])

executable(
  'vamp-plugin-load-checker',
  'checker/src/helper.cpp',
  include_directories: [
  ],
  cpp_args: [
    general_defines,
  ],
  dependencies: [
    dl_dep,
  ],
  link_args: [
    general_link_args,
  ],
  win_subsystem: 'console',
  install: true,
)

executable(
  'piper-vamp-simple-server',
  'piper-vamp-cpp/ext/json11/json11.cpp',
  'piper-vamp-cpp/vamp-server/simple-server.cpp',
  capnp_gen_cpp_target,
  vamphostsdk_files,
  include_directories: [
    'vamp-plugin-sdk',
    'piper-vamp-cpp',
    'piper-vamp-cpp/ext',
    feature_include_dirs,
  ],
  cpp_args: [
    general_defines,
  ],
  dependencies: [
    capnp_gen_header_dep,
    server_dependencies,
  ],
  link_args: [
    feature_additional_libs,
    general_link_args,
  ],
  win_subsystem: 'console',
  install: true,
)

executable(
  'piper-convert',
  'piper-vamp-cpp/ext/json11/json11.cpp',
  'piper-vamp-cpp/vamp-server/convert.cpp',
  capnp_gen_cpp_target,
  vamphostsdk_files,
  include_directories: [
    'vamp-plugin-sdk',
    'piper-vamp-cpp',
    'piper-vamp-cpp/ext',
    feature_include_dirs,
  ],
  cpp_args: [
    general_defines,
  ],
  dependencies: [
    capnp_gen_header_dep,
    server_dependencies,
  ],
  link_args: [
    feature_additional_libs,
    general_link_args,
  ],
  win_subsystem: 'console',
  install: true,
)

summary({'prefix': get_option('prefix'),
         'bindir': get_option('bindir'),
         'libdir': get_option('libdir'),
         'datadir': get_option('datadir'),
        }, section: 'Directories')

summary(config_summary, section: 'Configuration', bool_yn: true)
