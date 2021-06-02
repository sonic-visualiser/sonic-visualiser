
How to compile Sonic Visualiser from source (on a Mac)
======================================================

From Sonic Visualiser v4.3 on, the [Meson](https://mesonbuild.com)
build system is used.

On MacOS it is usually easiest to build from a repository clone rather
than from the official Sonic Visualiser source package. This is
because the repository build process will pull in some required
library builds before building, while the source package contains only
Sonic Visualiser code. Therefore, these instructions assume you are
going to do that.

If you are building from a repository clone, be sure to checkout the
appropriate tag if you want to build a specific release. The most
recent code in the repository (on the `default` branch) is not always
as stable as the release tags.

This document has two sections: first a description of how to proceed
using command-line tools, and then a description using an IDE (Qt
Creator). Scroll down if the second one is what you want.


Step-by-step using command-line tools only
------------------------------------------

(This is what I generally do)

### 1. Install prerequisites

These are

 * [Qt5](https://qt.io), GPL edition, which can be installed either
via Homebrew or from the official installer

 * [Meson](https://mesonbuild.com) and [Ninja](https://ninja-build.org)
build tools

 * The Xcode command-line tools (e.g. the C++ compiler, `c++`)

 * An SML compiler, e.g. [Poly/ML](https://polyml.org), which is also
available through Homebrew

### 2. Get the Sonic Visualiser code

For the most recent code and its history,

```
$ git clone https://github.com/sonic-visualiser/sonic-visualiser
```

For a specific release, e.g. v4.3,

```
$ git clone --branch sv_v4.3 https://github.com/sonic-visualiser/sonic-visualiser
```

Then `cd` to the newly checked-out directory. The rest of these
instructions assume you are in that directory.

### The build

First get or update the dependent libraries:

```
$ ./repoint install
```

Ensure your `QTDIR` and `PATH` environment variables are set up to
point to the Qt libraries and tools so that Meson can find them, e.g.

```
$ export QTDIR=$HOME/Qt/5.12.10/clang_64
$ export PATH=$PATH:$QTDIR/bin
```

Then configure the build. This creates a subdirectory called `build`
in which all the magic will happen:

```
$ meson build
```

And now compile the code into the `build` directory:

```
$ ninja -C build
```

### Running Sonic Visualiser from the build directory

If the build succeeds, you should have all the necessary executables
in the `build` directory - but not yet in the form of an app bundle
that you can install or run straight from Finder.

To run from the command line, you first need to set the
`DYLD_FRAMEWORK_PATH` environment variable to point to the location of
your Qt frameworks, for example:

```
$ export DYLD_FRAMEWORK_PATH=/Users/whatever/Qt/5.12.10/clang_64/lib/
```

This is because the Qt libraries are still neither part of the Sonic
Visualiser app bundle (which hasn't been created yet) nor installed in
a standard system location, and so there is no stable way for the
executable to refer to them.

After setting this, you should be able to run

```
$ ./build/"Sonic Visualiser"
```

and the Sonic Visualiser window should appear.

(If you haven't set `DYLD_FRAMEWORK_PATH`, you'll probably see this
error instead:

```
dyld: Library not loaded: @rpath/QtCore.framework/Versions/5/QtCore
  Referenced from: /Users/cannam/opensource/sonic-visualiser/./build/Sonic Visualiser
  Reason: image not found
Abort trap: 6
```
)

### Deploying into an app bundle

To make an application you can install or copy around, run the
deployment script:

```
$ ./deploy/macos/deploy.sh "Sonic Visualiser"
```

If this runs successfully, then it should leave an app bundle named
`Sonic Visualiser.app` in the current directory. This can be opened by
e.g. locating it in Finder and double-clicking, and it can also be
installed by dragging to the `/Applications` folder.

However, it is still not code-signed, so if you distribute it to
anyone they will need to override the operating system's initial
refusal to run it (Ctrl-click in Finder, say "Open...", and say yes to
the resulting dialog). Code signing for distribution is beyond the
scope of this document.

