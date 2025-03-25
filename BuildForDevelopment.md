Building Vrui for Development
=============================

When Vrui is built in the standard way, by running

```
$ make
$ sudo make install
```

then it will build itself in "production mode," meaning that all 
dynamic libraries will be properly versioned, and sub-directories 
containing settings, resources, etc. will contain the current Vrui 
version number in their names. This is good to ensure stability for 
production environments, as existing applications will not be broken by 
installing new versions of Vrui, but may be inconvenient for 
development on Vrui itself, where it is preferable that code changes 
immediately show up in all dependent applications without having to 
rebuild them.

To achieve this, Vrui can be built in "development mode," where dynamic 
libraries will not be versioned, meaning Vrui applications built 
against development Vrui will always use the most current versions of 
all libraries, and where directory names will not contain version 
numbers, either. Additionally, there is a "devinstall" target in the 
makefile that creates a "live" installation in the selected location, 
which will symbolically link against created files in the Vrui source 
tree itself, meaning that it is almost never necessary to run "make 
install" after making changes and rebuilding Vrui via "make," and any 
changes will be immediately pushed to Vrui applications.

Development mode is configured by changing a bunch of settings in the 
main makefile, which is not ideal because the makefile is itself under 
git version control. One could, alternatively, pass all the needed 
settings on make's command line, but that requires long command lines 
and is very error-prone.

We suggest using a script that wraps the necessary invocations of make 
and contains all changes that typically depend on a developer's or 
user's local system. The following bash script can serve as a template. 
If named "Make.sh" and dropped into the root directory of Vrui's source 
code, it will automatically be ignored by git version control.

```bash
#!/bin/bash
########################################################################
# Make.sh - Bash script to automate building Vrui in development mode.
# Copyright (c) 2024-2025 Oliver Kreylos
#
# This file is part of the WhyTools Build Environment.
# 
# The WhyTools Build Environment is free software; you can redistribute
# it and/or modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.
# 
# The WhyTools Build Environment is distributed in the hope that it will
# be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with the WhyTools Build Environment; if not, write to the Free
# Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
# 02111-1307 USA
########################################################################

# Define version numbers for Vrui in development mode:
PROJECT_MAJOR=dev
PROJECT_MINOR=dev
PROJECT_VERSION=dev

# If you want to build Vrui with support for SteamVR headsets, put the
# location of SteamVR here, as in:
# STEAMVRDIR=/opt/SteamVR-1.15.19/steamapps/common/SteamVR
STEAMVRDIR=

# Define the location where Vrui will be installed:
INSTALLDIR=/opt/Vrui-${PROJECT_VERSION}
# Set NEED_SUDO to 0 if the installation directory is user-writable:
NEED_SUDO=1

# Define the number of CPU cores to use for parallel builds:
NUM_CPUS=8

########################################################################
# Nothing below here should need to be changed
########################################################################

# Parse this script's command line:
DEBUG=0 # If !=0, build or install Vrui in debug mode
INSTALL=0 # If != 0, install Vrui in the selected target location
OTHER_ARGUMENTS=() # Any other arguments will be passed on to make
while [[ $# -gt 0 ]]; do
	case $1 in
	debug)
		DEBUG=1
		;;
	install)
		INSTALL=1
		;;
	*)
		OTHER_ARGUMENTS+=("$1")
	esac
	shift
done

# Assemble make's command line:
ARGUMENTS=()
ARGUMENTS+=("PROJECT_MAJOR=${PROJECT_MAJOR}")
ARGUMENTS+=("PROJECT_MINOR=${PROJECT_MINOR}")
ARGUMENTS+=("PROJECT_VERSION=${PROJECT_VERSION}")
ARGUMENTS+=("STEAMVRDIR=${STEAMVRDIR}")
ARGUMENTS+=("INSTALLDIR=${INSTALLDIR}")
if [ ${DEBUG} -ne 0 ]; then
	ARGUMENTS+=(DEBUG=1)
fi

# Run make depending on command line:
if [ ${INSTALL} -ne 0 ]; then
	# Install Vrui in development mode:
	if [ ${NEED_SUDO} -ne 0 ]; then
		sudo make "${ARGUMENTS[@]}" "${OTHER_ARGUMENTS[@]}" devinstall
	else
		make "${ARGUMENTS[@]}" "${OTHER_ARGUMENTS[@]}" devinstall
	fi
else
	# Build Vrui in development mode, forwarding additional arguments
	# passed to this script:
	make "${ARGUMENTS[@]}" -j${NUM_CPUS} "${OTHER_ARGUMENTS[@]}"
fi
```

To adjust the template script, put in your preferred installation 
location and the number of CPU cores you want to use for parallel 
builds, i.e., the number of cores (or threads) on your local computer. 
If you want to build Vrui with support for SteamVR headsets (HTC Vive, 
HTC Vive Pro, Valve Index) you should set the location of your SteamVR 
installation as indicated in the script. Vrui's makefile attempts to 
auto-detect the presence of SteamVR, but might not succeed.

Building Vrui in Debug Mode
---------------------------

Vrui can be built in debug mode by passing `DEBUG=1` to `make`, or by 
passing `debug` to `Make.sh`. Release and debug versions of Vrui can 
coexist in the same source and installation directories, but need to be 
built and installed separately. I.e., to build and install both 
versions, run

```
$ ./Make.sh
$ ./Make.sh install
$ ./Make.sh debug
$ ./Make.sh debug install
```

Debug versions of executables can be found in the "debug" subdirectory 
of the installation's "bin" subdirectory, and debug versions of dynamic 
libraries can be found in the "debug" subdirectory of the usual library 
subdirectory used by the local computer's Linux distribution ("lib", 
"lib64", etc.).

If Vrui applications follow the standard Vrui build system, their 
makefiles will automatically do the right thing and build against the 
release version of Vrui by default, or against the debug version if 
`DEBUG=1` is passed to `make` when building them.

Building Vrui Applications for Development
------------------------------------------

All Vrui add-on packages and applications released under this github 
organization use the standard Vrui build system, meaning their 
makefiles are based on the template makefile found in the "share/make" 
subdirectory of Vrui's installation. Typically, the only configuration 
necessary to build them is to point them at that subdirectory. This is 
always the first setting in a package's makefile:

```
VRUI_MAKEDIR := /usr/local/share/Vrui-<major>.<minor>/make
```

If Vrui was installed in a different location, this setting needs to 
be changed. Since package makefiles are under git version control, it 
is best to do this by passing the correct value on make's command line, 
e.g.,

```
$ make VRUI_MAKEDIR=/opt/Vrui-dev/share/make
```

It is also possible to build Vrui applications themselves in 
"development mode," similarly to how it's done for Vrui. To keep all 
the necessary changes in one place, it might be a good idea to create a 
wrapper script for make similar to the one for Vrui itself. The 
following can serve as a template:

```bash
#!/bin/bash
########################################################################
# Make.sh - Bash script to automate building Vrui packages.
# Copyright (c) 2024-2025 Oliver Kreylos
#
# This file is part of the WhyTools Build Environment.
# 
# The WhyTools Build Environment is free software; you can redistribute
# it and/or modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.
# 
# The WhyTools Build Environment is distributed in the hope that it will
# be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with the WhyTools Build Environment; if not, write to the Free
# Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
# 02111-1307 USA
########################################################################

# Define the location of Vrui's build system:
VRUI_MAKEDIR=/opt/Vrui-dev/share/make

# Define version numbers for this project:
PROJECT_MAJOR=dev
PROJECT_MINOR=dev
PROJECT_VERSION=dev

# Define the location where this project will be installed. If set to 
# ${PWD}, the project can be run directly from its source directory 
# without having to be installed first.
INSTALLDIR=${PWD}
# Set NEED_SUDO to 1 if the installation directory is not user-writable:
NEED_SUDO=0

# Define the number of CPU cores to use for parallel builds:
NUM_CPUS=8

########################################################################
# Nothing below here should need to be changed
########################################################################

# Parse this script's command line:
DEBUG=0 # If !=0, build or install the package in debug mode
INSTALL=0 # If != 0, install the package in the selected target location
OTHER_ARGUMENTS=() # Any other arguments will be passed on to make
while [[ $# -gt 0 ]]; do
	case $1 in
	debug)
		DEBUG=1
		;;
	install)
		INSTALL=1
		;;
	*)
		OTHER_ARGUMENTS+=("$1")
	esac
	shift
done

# Assemble make's command line:
ARGUMENTS=()
ARGUMENTS+=("VRUI_MAKEDIR=${VRUI_MAKEDIR}")
ARGUMENTS+=("PROJECT_MAJOR=${PROJECT_MAJOR}")
ARGUMENTS+=("PROJECT_MINOR=${PROJECT_MINOR}")
ARGUMENTS+=("PROJECT_VERSION=${PROJECT_VERSION}")
ARGUMENTS+=("INSTALLDIR=${INSTALLDIR}")
if [ ${DEBUG} -ne 0 ]; then
	ARGUMENTS+=(DEBUG=1)
fi

# Run make depending on command line:
if [ ${INSTALL} -ne 0 ]; then
	if [ "${INSTALLDIR}" != "${PWD}" ]; then
		# Install the package in development mode:
		if [ ${NEED_SUDO} -ne 0 ]; then
			sudo make "${ARGUMENTS[@]}" "${OTHER_ARGUMENTS[@]}" devinstall
		else
			make "${ARGUMENTS[@]}" "${OTHER_ARGUMENTS[@]}" devinstall
		fi
	else
		echo "Installation not needed; run package from source tree!"
	fi
else
	# Build the package in development mode, forwarding additional arguments
	# passed to this script:
	make "${ARGUMENTS[@]}" -j${NUM_CPUS} "${OTHER_ARGUMENTS[@]}"
fi
```

Just like Vrui's make wrapper script, this one understands the "debug" 
and "install" command line options. If the "INSTALLDIR" variable was 
not changed from the default of "${PWD}" (the root directory of the 
package's source tree), then the package does not need to be installed 
to be run, and `make install` or `make devinstall` *must not* be 
called.

After building, and, if necessary, installing, a package's executables 
will be found in the "bin" subdirectory of its installation directory, 
or in "bin/debug" in debug mode.
