
# How to compile Sonic Visualiser from source (on a Mac)

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

This document has two sections:

 * a description of how to proceed using command-line tools only
 * a description of the same thing using an IDE (Qt Creator).

Keep scrolling down if the second one is what you want.


## 1. Step-by-step using command-line tools only

(This is what I generally do)

### 1.1. Install prerequisites

These are

 * [Qt5](https://qt.io), GPL edition
 * [Meson](https://mesonbuild.com) and [Ninja](https://ninja-build.org)
build tools
 * An SML compiler, e.g. [Poly/ML](https://polyml.org)
 * The Xcode command-line tools (e.g. the C++ compiler, `c++`)

The first three are available through Homebrew, although the official
Qt installer is also quite convenient.

### 1.2. Get the Sonic Visualiser code

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

### 1.3. Configure and build

Ensure your `QTDIR` and `PATH` environment variables are set up to
point to the Qt libraries and tools so that Meson can find them, e.g.

```
$ export QTDIR=$HOME/Qt/5.12.10/clang_64
$ export PATH=$PATH:$QTDIR/bin
```

Then configure the build by running Meson. This creates a subdirectory
called `build` in which the magic will happen:

```
$ meson build
```

(If this fails with an error like `Include dir bqvec does not exist`,
go back and re-read step 1.2 above.)

And now compile the code into the `build` directory using the Ninja
build tool:

```
$ ninja -C build
```

### 1.4. Run Sonic Visualiser from the build directory

If the build succeeds, you should now have all the necessary
executables in the `build` directory. They are not yet in the form of
an app bundle that you can install or run straight from Finder, but it
is possible to run them from the build directory.

To do so you first need to set the `DYLD_FRAMEWORK_PATH` environment
variable to point to the location of your Qt frameworks, typically:

```
$ export DYLD_FRAMEWORK_PATH=$QTDIR/lib
```

This is because the Qt libraries have not been deployed to the Sonic
Visualiser app bundle (which hasn't been created yet) and are not
considered to be standard system libraries, so there is no stable way
for the executable to refer to them.

After setting this, you should be able to run

```
$ ./build/"Sonic Visualiser"
```

(note the quotes, because of the space in the filename) and the Sonic
Visualiser window should appear.

(If you haven't set `DYLD_FRAMEWORK_PATH`, you'll probably see an error
like this instead:

```
dyld: Library not loaded: @rpath/QtCore.framework/Versions/5/QtCore
  Referenced from: /path/to/sonic-visualiser/./build/Sonic Visualiser
  Reason: image not found
Abort trap: 6
```
)

### 1.5. Deploy into an app bundle

To make an application you can install or copy around, run the
deployment script:

```
$ ./deploy/macos/deploy.sh "Sonic Visualiser"
```

If this runs successfully, then it should leave an app bundle named
`Sonic Visualiser.app` in the current directory. This can be run
directly from Finder, and it can also be installed by dragging to the
`/Applications` folder.

It is still not code-signed, so if you distribute it to anyone they
will need to override the operating system's initial refusal to run it
(Ctrl-click in Finder, say "Open...", and say yes to the resulting
dialog). Code signing for distribution is beyond the scope of this
document.


## 2. Step-by-step using an IDE (Qt Creator)

### 2.1. Install prerequisites

These are the same as for a command-line build:

 * [Qt5](https://qt.io), GPL edition, with Qt Creator IDE
 * [Meson](https://mesonbuild.com) and [Ninja](https://ninja-build.org)
build tools
 * An SML compiler, e.g. [Poly/ML](https://polyml.org)
 * The Xcode command-line tools (e.g. the C++ compiler, `c++`)

Open Qt Creator and make sure you have the Meson plugin enabled. You
can find this under the Qt Creator main menu -> `About Plugins...` ->
`Build Systems` -> tick the box next to `MesonProjectManager`. If you
don't see this option listed, you may possibly need a newer version of
Qt Creator.

### 2.2. Get the Sonic Visualiser code

In Qt Creator, go to `File` -> `New File or Project...` and select Git
Clone with the clone URL
`https://github.com/sonic-visualiser/sonic-visualiser`. For the most
recent code, leave the branch blank; for a specific release, e.g. 4.3,
enter the tag name e.g. `sv_v4.3`.

When the project has been cloned and opened, then there is still one
step for which you will need to go to a terminal window. Open
Terminal, `cd` to the directory that Qt Creator just checked the code
out into, and run

```
$ ./repoint install
```

to get or update the dependent libraries.

Now return to Qt Creator. If it has already configured the project,
tell it to re-configure it by right-clicking on the project name in
the Projects pane and selecting `Configure`. This should cause the
project to be populated in the IDE with all of its source files.

If configuration fails because Qt can't find the Meson or Ninja
programs, try these interventions:

 * Go to the project's `Build & Run` settings, edit the list of
   variables in the Build Environment section, and add the locations
   of Meson and Ninja executables (e.g. `/usr/local/bin`) to the
   `PATH` entry

 * Go to the `Manage Kits...` window, find Meson in the list of
   sections on the left, pick the Tools tab, and add the locations of
   Meson and Ninja if they are not there already

### 2.3. Build the code

Once you have the project configured and have selected a "build kit"
corresponding to a Qt version and compiler, run `Build` -> `Build All
Projects`.

Be sure to consider whether you want a Debug or Release build. The
default in Qt Creator is Debug, which is fine for development and
debugging but produces large binaries and runs relatively slowly.

### 2.4. Run Sonic Visualiser from the IDE

Ensure `Sonic Visualiser` is selected as the run target in the project
configuration.

As with the command-line process described above, you need to ensure
the application can find the Qt frameworks by setting
`DYLD_FRAMEWORK_PATH`. Do this in the `Projects` tab, within the `Run`
configuration: here there is an `Environment` section which by default
is specified as `Use Build Environment`.

Edit that section so as to set the `DYLD_FRAMEWORK_PATH` variable to
your Qt library directory, for example
`/Users/me/Qt/5.12.10/clang_64/lib`. (You should only have to do this
once, not every time you rebuild the project!)

After setting this, you should be able to run Sonic Visualiser by
clicking the big green arrow Run button.

(If you haven't set `DYLD_FRAMEWORK_PATH`, you'll probably see an error
like this in the Application Output window instead:

```
dyld: Library not loaded: @rpath/QtCore.framework/Versions/5/QtCore
  Referenced from: /path/to/sonic-visualiser/./build/Sonic Visualiser
  Reason: image not found
Abort trap: 6
```
)

### 2.5. Deploy into an app bundle

To make an application you can install or copy around, run the
deployment script. This part can't be done from the IDE, and requires
a terminal window.

Open Terminal and `cd` to the main Sonic Visualiser project
directory. Now run

```
$ ./deploy/macos/deploy.sh "Sonic Visualiser" "path/to/build/directory"
```

replacing `path/to/build/directory` with the build directory path as
shown in the Build Settings page within Qt Creator. (It will probably
be something quite complicated.)

If this runs successfully, then it should leave an app bundle named
`Sonic Visualiser.app` in the project directory. This can be run
directly from Finder, and it can also be installed by dragging to the
`/Applications` folder.

It is still not code-signed, so if you distribute it to anyone they
will need to override the operating system's initial refusal to run it
(Ctrl-click in Finder, say "Open...", and say yes to the resulting
dialog). Code signing for distribution is beyond the scope of this
document.

