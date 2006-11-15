#!/bin/bash

audio=$HOME/music
preferred=$audio/ogg/linux
list=audiofiles.txt
used=audiofiles-used.txt

sv-command quit

pick_file()
{
    count=`wc -l "$list" 2>/dev/null | awk '{ print $1 }'`
    if [ ! -f "$list" ] || [ "$count" -eq "0" ] ; then
	find "$audio" -name \*.ogg -print >> "$list"
	find "$audio" -name \*.mp3 -print >> "$list"
	find "$audio" -name \*.wav -print >> "$list"
	find "$preferred" -name \*.ogg -print >> "$list"
	find "$preferred" -name \*.mp3 -print >> "$list"
	find "$preferred" -name \*.wav -print >> "$list"
	count=`wc -l "$list" 2>/dev/null | awk '{ print $1 }'`
    fi
    while [ -z "$file" ]; do
	index=$((RANDOM % $count))
	file=`tail +"$index" "$list" | head -1`
	[ -f "$file" ] || continue
    done
    fgrep -v "$file" "$list" > "$list"_ && mv "$list"_ "$list"
    echo "$file"
}

load_a_file()
{
    file=`pick_file`
    if ! sv-command open "$file"; then
	pid="`pidof sonic-visualiser`"
	if [ -z "$pid" ]; then
	    ( setsid ../sonic-visualiser & )
	    sleep 2
	    load_a_file
	else
	    echo "ERROR: Unable to contact sonic-visualiser pid $pid" 1>&2
	    exit 1
	fi
    fi
}

show_stuff()
{
    sv-command set overlays 2
    sv-command set zoomwheels 1
    sv-command set propertyboxes 1
}

hide_stuff()
{
    sv-command set overlays 0
    sv-command set zoomwheels 0
    sv-command set propertyboxes 0
}

reset()
{
    for pane in 1 2 3 4 5; do
	for layer in 1 2 3 4 5 6 7 8 9 10; do
	    sv-command delete layer
	done
	sv-command delete pane
    done
    sv-command zoom default
    sv-command add waveform
    show_stuff
}

scroll_and_zoom()
{
    sv-command set overlays 0
    sv-command set zoomwheels 0
    sv-command set propertyboxes 0
    sv-command setcurrent 1 1
    sv-command delete layer
    sv-command setcurrent 1 1
    sv-command set layer Colour Red
    sleep 1
    sv-command set pane Global-Zoom off
    sv-command set pane Global-Scroll off
    sv-command set pane Follow-Playback Scroll
    for zoom in 950 900 850 800 750 700 650 600 550 512 450 400 350 300 256 192 160 128 96 64 48 32 24 16; do
	sv-command zoom $zoom
	sleep 0.1
    done
}

play()
{
    sv-command play "$@"
}

fade_in()
{
    sv-command set gain 0
    sleep 0.5
    play "$@"
    for gain in 0.001 0.01 0.05 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1; do
	sv-command set gain $gain
	sleep 0.1
    done
}

fade_out()
{
    for gain in 0.9 0.8 0.7 0.6 0.5 0.4 0.3 0.2 0.1 0.05 0.01 0.001; do
	sv-command set gain $gain
	sleep 0.1
    done
    stop
    sv-command set gain 1
}

slow()
{
    for speed in -1 -10 -20 -30 -40 -50 -60 -70 -80 -100 -140 -200 -250 -300 -400 -500 -700 -800 -900 -1000; do
	sv-command set speedup "$speed"
	sleep 1
    done
}

stop()
{
    sv-command stop "$@"
    sv-command set speedup 0
}

quit()
{
    sv-command quit
}

add_melodic_range_spectrogram()
{
    sv-command set propertyboxes 1
    sv-command add spectrogram
    sv-command set layer Window-Size 4096
#    sv-command set layer Window-Overlap 4
    sv-command set layer Window-Overlap 3
    sv-command set layer Frequency-Scale Log
    sv-command set layer Colour-Scale Meter
}

zoom_in_spectrogram() 
{
    sv-command zoomvertical 43 8000
    for x in 1 2 3 4 5 6; do
	max=$((8000 - 1000*$x))
	sv-command zoomvertical 43 "$max"
	sleep 0.1
    done
    for x in 1 2 3 4 5; do
	max=$((2000 - 100 * $x))
	sv-command zoomvertical 43 "$max"
	sleep 0.1
    done
}

zoom_in_spectrogram_further() 
{
    for x in 1 2 3 4 5; do
	sv-command zoomvertical in
    done
}

playback_bits()
{
    sv-command set pane Follow-Playback Scroll
    sv-command jump 10
    sv-command setcurrent 1 2
    sv-command set layer Colour Blue
    sleep 5
    hide_stuff
    fade_in
    sleep 10
    scroll_and_zoom
    sleep 10
    slow
    #sv-command set layer Normalize-Visible-Area on
    for zoom in 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1; do
	sv-command zoom $zoom
	sleep 0.1
    done
    sleep 8
    fade_out
    done_playback_bits=1
}

spectrogram_bits()
{
    add_melodic_range_spectrogram
    sv-command zoom 256
    sleep 10
    zoom_in_spectrogram
    sleep 20

    if [ -n "$done_playback_bits" ]; then
	sv-command setcurrent 1
	sv-command zoom out
	sv-command zoom out
	sv-command zoom out
	sv-command zoom out
	sv-command zoom out
	sv-command setcurrent 2
    fi
    
    hide_stuff
    fade_in
    sleep 10
    sv-command set layer Bin-Display Frequencies
    sv-command set layer Normalize-Columns on
    sleep 20
    sv-command set layer Bin-Display "All Bins"
    sv-command set layer Colour "Red on Blue"
    sleep 10
    fade_out
    sleep 1
    done_spectrogram_bits=1

#    zoom_in_spectrogram_further
}

onset_bits()
{
    show_stuff
    sv-command setcurrent 1
    sv-command set pane Global-Zoom on
    sv-command set pane Global-Scroll on
    sleep 5
    sv-command zoom default
    sv-command zoom in
    sv-command zoom in
    sv-command zoom in
    sv-command add timeruler
    sv-command transform vamp:vamp-aubio:aubioonset:detectionfunction
    sleep 5
    sv-command set layer Plot-Type Curve
    sleep 5
    sv-command setcurrent 1
    sv-command set pane Follow-Playback Page
    sv-command transform vamp:vamp-aubio:aubioonset:detectionfunction
    sv-command set layer Colour Red
    sleep 5
    sv-command set layer Plot-Type Segmentation
#    sv-command set layer Vertical-Scale "Log Scale"
    sv-command jump 30
    sleep 5
#    hide_stuff
    if [ -n "$done_spectrogram_bits" ]; then
	sv-command setcurrent 2
	sv-command delete pane
    fi
    sleep 10
    sv-command setcurrent 1 1
    sv-command set layer Colour Black
    sv-command setcurrent 1 2
    fade_in
    sleep 5
    sv-command transform vamp:vamp-aubio:aubioonset:onsets
    sv-command set layer Colour Black
    sleep 20
    sv-command setcurrent 2
#    sv-command transform vamp:qm-vamp-plugins:qm-tempotracker:beats
    sv-command transform vamp:vamp-aubio:aubiotempo:beats
    sleep 10
    fade_out
    show_stuff
}

selection_bits()
{
#    reset
    sv-command set overlays 1
    sv-command set zoomwheels 0
    sv-command setcurrent 2
    sv-command delete pane
    if [ -n "$done_playback_bits" ]; then
	sv-command setcurrent 1 2
    else
	sv-command setcurrent 1 3
    fi
    sv-command delete layer
    if [ -n "$done_playback_bits" ]; then
	sv-command setcurrent 1 2
    else
	sv-command setcurrent 1 3
    fi
    sv-command delete layer
#    sv-command transform vamp:qm-vamp-plugins:qm-tempotracker:beats
    sv-command transform vamp:vamp-aubio:aubiotempo:beats
    sv-command setcurrent 1 2
    sv-command set layer Colour Orange
    sv-command setcurrent 1 3
    sv-command resize 1024 500
    sv-command zoom default
    sleep 10
    sv-command loop on
    base=$((RANDOM % 100))
    sv-command select $base $base.3
    fade_in selection
    sleep 12
    base=$((base + 4))
    sv-command addselect $base $base.1
    sleep 12
    base=$((base + 2))
    sv-command addselect $base $base.1
    sleep 6
    base=$((base + 2))
    sv-command addselect $base $base.3
    sleep 6
    base=$((base + 3))
    sv-command addselect $base $base.3
    sleep 6
    base=$((base + 2))
    sv-command addselect $base $base.3
    sleep 12
    sv-command set speedup -50
    sleep 14
    sv-command set speedup 50
    sleep 8
    sv-command set speedup 100
    sleep 5
    sv-command set speedup 200
    fade_out
    sleep 20
    sv-command select none
    sv-command setcurrent 1 3
    sv-command delete layer
    sv-command setcurrent 1 2
    sv-command set layer Colour Black
}

chromagram_bits()
{
    add_melodic_range_spectrogram
    sleep 10
    sv-command add timeruler
    sv-command transform vamp:qm-vamp-plugins:qm-chromagram:chromagram
    sleep 2
    sv-command jump 20
    sleep 10
    sv-command zoom out
    fade_in
    sleep 15
    fade_out
}

while /bin/true; do

sleep 2
load_a_file
sv-command loop on

sv-command resize 1024 500
show_stuff
sleep 10
playback_bits

sv-command resize 1024 700
show_stuff
#sleep 10
spectrogram_bits

sv-command jump 0
sv-command resize 1024 700
show_stuff
sleep 10
onset_bits

selection_bits

sv-command resize 1024 700

sv-command jump 0
chromagram_bits

sleep 20

pid="`pidof sonic-visualiser`"
if [ -n "$pid" ]; then
    kill "$pid"
    sleep 5
else
    reset
fi

done
