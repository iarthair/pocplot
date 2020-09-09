# PocPlot

A simple GTK3/GObject graph plotting library.

This project was born from the need for making some nice looking plots in
another project I was working on.  Many years ago I had used the plotting
facilities from GtkExtra but this module was never ported to GTK3.  Having
looked into porting the GtkExtra modules from GTK2 I decided it was too complex
a task, given my requirements, so I started looking around for something else.
I found Goat Plot but this had issues with recent GTK3. However it was almost
what I wanted. Ultimately after a false start at fixing the issues there I
decided to sketch out some ideas and write my own.

PocPlot is the result of that effort. I've been using this with no problems
other than occasional bug-fixes for about ten months at the time of writing
(September 2020).

## Installation

PocPlot uses the
[Meson build system](https://mesonbuild.com/Getting-meson.html).
Refer to the Meson manual for standard configuration options.

Meson supports multiple build system backends.  To build with
[Ninja](https://ninja-build.org/) do the following:

``` sh
$ meson [options] --buildtype=release builddir
$ ninja -C builddir install
```

The following build options are supported (defaults in bold text):
* -Ddocs=true/**false** - build and install the documentation.
* -Dintrospection=**true**/false - enable GObject introspection.
* -Dvapi=true/**false** - use `vapigen` to build Vala support.

A catalogue file, `poc-catalog.xml` is installed for use with Glade.

Note that the meson/ninja installer does not require an explicit `sudo`,
instead it will prompt for a password during install.

## Dependencies

PocPlot depends only on recent gtk3 and glib.

## Documentation

Documentation is built during installation and is accessible using Devhelp.
Usage should be fairly straightforward.

## Why PocPlot?

The name PocPlot is an hommage to GoatPlot (poc is Irish for a he-goat).
Being short, `poc` is also a convenient GObject namespace.

## Licence

PocPlot is licensed under the GNU Lesser General Public License version 2.1.
Please refer to LICENSE for full details.
