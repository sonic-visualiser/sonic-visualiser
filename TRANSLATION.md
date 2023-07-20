
Translation strings
===================

Sonic Visualiser loads translation strings from the Qt `.qm` compiled
files found in the `i18n/` directory. These are generated from the
editable XML `.ts` strings files in the same location, using the
standard Qt5 language tools (`lupdate`, `lrelease`, and the Linguist
interactive editor).

Everything that appears within a `tr()` call anywhere in the source
code is available for translation: this means nearly 2000 strings,
although a great many are obscure or unimportant.

The translations are not currently updated automatically during the
build, but there is a script (that works on Linux and macOS) in
`misc/update-i18n.sh` that runs the update. This script should be run
without any command-line arguments, and it:

 1. Brings any new strings from the source code into the `.ts` files,
 merging translations, using `lupdate`

 2. Compiles all of the `.ts` files into `.qm` files ready to be
 included into the next build, using `lrelease`.

This means you can run it both before and after making a change to a
translation - run it beforehand to ensure the merged strings are
up-to-date, then again afterwards to compile the newly updated strings
into the `.qm` file for build and testing.

The `.qm` files are then included into the compiled binary as Qt
resources, by virtue of being listed in the resource file
`sonic-visualiser.qrc`.

Adding a new language
---------------------

I believe the process should go like this:

 1. Add your language's ISO code to the list in the `LANGUAGES`
 variable at the top of the `misc/update-i18n.sh` script

 2. Run the `misc/update-i18n.sh` script to update the existing `.ts`
 XML files in the i18n/ directory and generate the new one
 
 3. Use Qt5 Linguist to edit the `.ts` file and add translations

 4. (Make a copy of the edited `.ts` file - or commit it locally - in case
 the next script invocation goes wrong! You don't want to lose work)
 
 5. Run `misc/update-i18n.sh` again to create compiled `.qm` files
 from the `.ts` files
 
 6. Edit the XML file `sonic-visualiser.qrc` in a text editor, and add
 the new compiled `.qm` file to the list of `.qm` files near the
 bottom so that it gets compiled into the binary
 
 7. Build Sonic Visualiser following the `COMPILE` instructions
 elsewhere and test.

Corrections, enhancements, & efficiency improvements welcome!
