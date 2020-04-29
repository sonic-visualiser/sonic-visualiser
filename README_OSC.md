
OSC control of Sonic Visualiser
===============================

Sonic Visualiser can be controlled remotely using the Open Sound
Control protocol.  This facility requires the
[liblo](http://liblo.sourceforge.net/) (Lite OSC) library by Steve
Harris and Stephen Sinclair to have been available when Sonic
Visualiser was built.

Sonic Visualiser opens a single OSC port on startup, provided that OSC
support is compiled in, the `--no-osc` command-line option has not
been supplied, and permission to use the network was not refused by
the user the first time Sonic Visualiser was run.

The URL of the OSC port is printed to standard output on startup, or
can be read from the About box on the Help menu.


General outline
---------------

OSC commands accepted by Sonic Visualiser take the form:

```
<scheme>://<host>:<port>/<method> [<arg> ...]
```

For example, `osc.udp://localhost:12654/play 2.0` will play the
current session from time 2.0 seconds.

Methods that manipulate panes or layers act on the currently selected
pane or layer.  Use the `setcurrent` method to choose the right target
for subsequent such methods.


OSC clients and invocation
--------------------------

If you need an OSC client, there is a small program in the svcore
library at `svcore/data/osc/sv-osc-send.c` that sends an OSC method
and arguments to a given URL -- this is not specific to SV but will
work with it. To compile that program you should only have to run

```
$ gcc sv-osc-send.c -o sv-osc-send -llo
```

provided you have liblo installed.

Then there is a small shell script in the same directory, called
`sv-command`, that provides a basic command shell for Sonic
Visualiser.  Start SV first, then `sv-command` should find its OSC
port from the system process table when you start it.  For example:

```
$ PATH=.:$PATH ./sv-command      # Set PATH so it can find sv-osc-send
/open snare_hex.wav
/add spectrogram
/set layer Colour Sunset
/play
/quit
$
```

You can also start Sonic Visualiser directly in "OSC script" mode by
supplying the `--osc-script <filename>` command-line option. The
argument to this should be the path of a text file containing a series
of OSC commands (like those in the example above). These will be
executed in succession as quickly as possible. A line that contains
only a number, instead of a command, will cause SV to pause for that
number of seconds before going on with the script. Lines beginning
with `#` are ignored.

If you supply "-" as the filename argument to the `--osc-script`
option, SV will instead read OSC commands from standard input. So you
can do something like

```
$ rlwrap sonic-visualiser --osc-script - /path/to/filename.wav
```

to obtain a rather limited Sonic Visualiser OSC read-eval loop.


Method reference
----------------

### File management

```
/open <filename>
```

Open a new file (of type determined by Sonic Visualiser).  If it is an
audio file, use it to replace the existing main audio file (if any).

```
/openadditional <filename> 
```

Open a new file.  If it is an audio file, open it in a new pane in
addition to the existing audio file (if any).

```
/recent <n>
/last
```

Open the `<n>`th most recent file from the Recent Files menu, counting
from 1 for the most recent file opened.  "last" is a synonym for
"recent 1".

```
/save <filename>
```

Save the current session in `<filename>` as an SV session file.

This action will try to fail rather than overwrite an existing file,
but you probably shouldn't rely on that.

```
/export <filename>
```

Export the main audio file to `<filename>` in floating-point extended
WAV format. If a region is selected, export only that region.

This action will try to fail rather than overwrite an existing
file, but you probably shouldn't rely on that.

```
/exportlayer <filename>
```

Export the current layer to `<filename>`. See `/setcurrent` for how to
change which layer is current. The format to use is determined from
the file extension of `<filename>`:

 * `xml`, `svl` - Sonic Visualiser layer file formats
 * `mid`, `midi` - MIDI (note layers only)
 * `ttl`, `n3` - Audio Features Ontology RDF/Turtle (only some layer types)
 * `csv` - Comma-separated text
 * anything else - Tab-separated text

If a region is currently selected, and the file type is CSV,
tab-separated, or MIDI, export only that region. Otherwise export
the whole layer.

This action will try to fail rather than overwrite an existing
file, but you probably shouldn't rely on that.

```
/exportimage <filename>
```

Export a PNG image of the contents of the current pane to
`<filename>`. See `/setcurrent` for how to change which pane is
current. If a region is currently selected, export only that
region. Otherwise export the whole pane. Currently the export will
fail if the area to be exported is more than 32767 pixels wide.

To export to an image of a fixed width regardless of model duration,
consider using `/zoom fit` to zoom the model contents to fit the
window width, then optionally zooming in or out a known number of
steps, before exporting.

This action will try to fail rather than overwrite an existing file,
but you probably shouldn't rely on that.

```
/exportsvg <filename>
```

Export a SVG vector graphic of the contents of the current pane to
`<filename>`.  See `/setcurrent` for how to change which pane is
current.  If a region is currently selected, export only that
region. Otherwise export the whole pane.

This action will try to fail rather than overwrite an existing file,
but you probably shouldn't rely on that.


### Playback control

```
/jump <t>
/jump end
/jump selection
```

Jump the playback position to time `<t>` (in seconds); or to the end
of the file; or to the start of the current selection.

```
/play
/play <t>
/play selection
```

Start playback.  If a time `<t>` is given, start from that time in
seconds.  If the word `selection` is given instead, play the current
selection.

```
/stop
```

Stop playback.

```
/loop on
/loop off
```

Switch playback loop mode on or off.


### Region selection

```
/select <t0> <t1>
/select all
/select none
```

Select the region from times `<t0>` to `<t1>` in seconds; or select
the whole file; or clear the selection.  If there is a layer selected
that can be used as a snap guide for the selection, then the selection
will be snapped to it (in the same manner as when making selections
interactively).

```
/addselect <t0> <t1>
```

Make an additional selection (leaving any existing selection in place)
from times `<t0>` to `<t1>` in seconds.


### Panes, layers, and the window

```
/add <layertype>
/add <layertype> <channel>
```

Add a new pane containing a layer of the given type, based on the
given channel of the main audio file (numbered from 0). If no
`<channel>` is specified, mix all channels.

Useful values of `<layertype>` are:

```
  waveform
  spectrogram
  melodicrange
  peakfrequency
  spectrum
  timeruler
```

The following `<layertype>`s are also accepted:

```
  timeinstants
  timevalues
  notes
  text
  colour3dplot
```

But they are less useful, as they create empty layers which there is
currently no OSC support for editing.

```
/setcurrent <pane>
/setcurrent <pane> <layer>
```

Make the given `<pane>` (a number counting from 1 for the topmost
pane) and optionally the given `<layer>` on that pane (a number
counting from 1 for the "frontmost" layer) the current pane and layer
for subsequent pane and layer operations.

```
/delete pane
/delete layer
```

Delete the current pane or layer.

```
/set <control> <value>
/set pane <control> <value>
/set layer <control> <value>
```

Set a main window control; a property of the current pane; or a
property of the current layer.

Accepted main window controls are:

 * `gain` - whose values are linear multipliers (i.e. 1.0 == unity gain).
 * `speed` - takes a value of a percentage change in playback speed,
so 100 is the default playback speed, 200 sets double the default
speed, and 50 sets half the default.
 * `overlays` - controls the verbosity level of the text overlays on
each pane, from 0 (everything off) to 2 (everything on).
 * `zoomwheels` - controls whether the zoom wheels are displayed (1)
or not (0).
 * `propertyboxes` - controls whether the property boxes are displayed
(1) or not (0).

For pane and layer properties, the control name is the displayed name
of the given property (though you may use "-" or "_" in place of any
spaces in the name if it's easier for you).  The value may be the
displayed value or underlying integer for the property.

Some examples:

```
    /set pane Global-Scroll off
    /set pane Follow_Playback Scroll
    /set layer Colour Blue
    /set layer Scale-Units dB
    /set layer Frequency-Scale Log
```

Note that while you can use "-" or "_" in place of spaces in the
property name, you cannot currently do so in the value text.  If
this is a problem for you, you might be able to set the value
as an integer instead (all layer properties can be set this way).

```
/zoom <level>
/zoom in
/zoom out
/zoom fit
/zoom default
```

Zoom to a given zoom `<level>`, given in audio sample frames per
pixel; or zoom in or out one step from the current level; or zoom so
that the whole model duration fits in the current view width; or
return to the default zoom level.  This method acts on the current
pane (it only affects all panes if set to Global Zoom, which is the
default).

```
/zoomvertical <min> <max>
/zoomvertical in
/zoomvertical out
/zoomvertical default
```

Change the vertical zoom and origin so as to show the value range from
`<min>` to `<max>` in the vertical scale; or zoom in or out
vertically; or return to the default vertical zoom level.  The effect
of this method is heavily dependent on the current layer.

```
/transform <name>
```

Transform the current main audio file using the named transform.
Transforms are named according to the scheme

```
    type:source:plugin:output
```

For example, the percussion onset detector from the Vamp example
plugin set can be invoked via

```
    /transform vamp:vamp-example-plugins:percussiononsets:onsets
```

If the output is omitted, the first is used.  Note that you need to
use the plugin and output name, not description: in this case
`percussiononsets` rather than "Simple Percussion Onset Detector".

There is not yet any way to run a transform via OSC on any but the
main audio file, nor with any but its default parameters, processing
block/step size, or channel selection.

```
/resize <w> <h>
```

Resize the main window to width `<w>` and height `<h>` (if the window
system permits).


### Housekeeping

```
/undo
/redo
```

Undo the last editing operation; redo the last undone operation.  Note
that most of the classic editing operations (copy and paste etc) are
not controllable via OSC, but undo may still be useful because Sonic
Visualiser considers actions such as adding a pane to be undoable
editing operations as well.

```
/quit
```

Exit the program abruptly without saving.


### Missing things

Handy features still missing from the OSC interface include:

 * the ability to run transforms with non-default parameters or
   starting from different source models
 * the ability to add layers to a pane (without transform)
 * the ability to add panes (and layers) showing any but the
   main model
 * the ability to set play parameters on a layer/model and show/hide it
 * the ability to import layers
 * a working pane resize
 * the ability to rename a layer


