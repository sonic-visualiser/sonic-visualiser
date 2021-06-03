
# How to compile Sonic Visualiser from source (on Windows)

From Sonic Visualiser v4.3 on, the [Meson](https://mesonbuild.com)
build system is used.

On Windows it is usually easiest to build from a repository clone
rather than from the official Sonic Visualiser source package. This is
because the repository build process will pull in some required
library builds before building, while the source package contains only
Sonic Visualiser code. Therefore, these instructions assume you are
going to do that.

If you are building from a repository clone, be sure to checkout the
appropriate tag if you want to build a specific release. The most
recent code in the repository (on the `default` branch) is not always
as stable as the release tags.

(NOTE: Refer also to the contents of the file `.appveyor.yml` to see
how the continuous-integration process runs a build for Windows)

The build is for 64-bit Windows and uses Visual Studio 2019 tools, but
is run from a command prompt using a shell script.

 * Install the Meson and Ninja build tools (https://mesonbuild.com)
 * Install SML/NJ (https://smlnj.org) for use by Repoint
 * Install Qt open source edition (https://qt.io)
 * Edit the file `deploy\win64\build-64.bat` and adjust the QTDIR setting
   to match the location of your Qt install
 * Run `deploy\win64\build-64.bat` in a command prompt
 
