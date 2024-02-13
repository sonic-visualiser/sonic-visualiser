
# How to compile Sonic Visualiser from source (on Linux)

From Sonic Visualiser v4.3 on, the [Meson](https://mesonbuild.com)
build system is used.

## 1. Install prerequisites

The following additional libraries or tools are required and should be
installed through your system package manager:

        Qt v6                   https://www.qt.io/
        Meson                   https://mesonbuild.com/
        Ninja                   https://ninja-build.org/
        MLton                   https://mlton.org/
        Rubber Band Library     https://www.breakfastquay.com/rubberband/
        libsndfile              https://www.mega-nerd.com/libsndfile/
        libsamplerate           https://www.mega-nerd.com/SRC/
        FFTW3                   https://www.fftw.org/
        bzip2 library           https://www.bzip.org/
        Sord and Serd libraries https://drobilla.net/software/
        Cap'n Proto             https://capnproto.org/
        MAD mp3 decoder         https://www.underbit.com/products/mad/
        Oggz and fishsound      https://xiph.org/oggz/
        Opus                    https://www.opus-codec.org/
        liblo OSC library       https://www.plugin.org.uk/liblo/
        JACK                    https://www.jackaudio.org/
        PulseAudio              https://www.pulseaudio.org/
        PortAudio v19           https://www.portaudio.com/

Versions of Sonic Visualiser prior to v5 require Qt5 instead of Qt6.

You will also need the ALSA libraries (used for MIDI).

For Cap'n Proto you will need v0.6 or newer.

If you happen to be using a Debian-based Linux, you probably want to
apt install something like the following packages:

build-essential libbz2-dev libfftw3-dev libfishsound1-dev libid3tag0-dev liblo-dev liblrdf0-dev libmad0-dev liboggz2-dev libopus-dev libopusfile-dev libpulse-dev libsamplerate-dev libsndfile-dev libsord-dev libxml2-utils portaudio19-dev qt6-base-dev qt6-pdf-dev qt6-base-dev-tools libqt6svg6-dev raptor2-utils git mercurial autoconf automake libtool smlnj capnproto libcapnp-dev ninja-build libglib2.0-dev

## 2. Get the Sonic Visualiser code

If you have an official Sonic Visualiser release source package (with
a name like `sonic-visualiser-X.Y.tar.gz`), unpack it, `cd` to the
unpacked directory, and skip straight to step 3.

Otherwise:

For the most recent code and its history,

```
$ git clone https://github.com/sonic-visualiser/sonic-visualiser
```

For a specific release, e.g. v4.3,

```
$ git clone --branch sv_v4.3 https://github.com/sonic-visualiser/sonic-visualiser
```

Then `cd` to the newly checked-out `sonic-visualiser` directory. (The
rest of these instructions assume you are in that directory.)

Next get or update the dependent libraries:

```
$ ./repoint install
```

Without this step, you will have the Sonic Visualiser application
shell but none of the code that does the real work, and the build will
quickly fail.

## 3. Configure and build

Configure the build by running Meson. This creates a subdirectory
called `build` in which the magic will happen:

```
$ meson build
```

(If this fails with an error like `Include dir bqvec does not exist`,
go back and re-read step 2 above.)

And now compile the code into the `build` directory using the Ninja
build tool:

```
$ ninja -C build
```

## 4. Install

You should be able to run your new build straight from the build
directory (`./build/sonic-visualiser`) after compiling. But to install
it to the default installation location (typically `/usr/local`), run

```
$ meson install
```
