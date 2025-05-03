#!/bin/sh
########################################################################
# Script to install prerequisite system packages used by the Vrui VR
# toolkit and its component libraries.
# Copyright (c) 2025 Oliver Kreylos
# 
# This file is part of the Virtual Reality User Interface Library
# (Vrui).
# 
# The Virtual Reality User Interface Library is free software; you can
# redistribute it and/or modify it under the terms of the GNU General
# Public License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
# 
# The Virtual Reality User Interface Library is distributed in the hope
# that it will be useful, but WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
# PURPOSE.  See the GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with the Virtual Reality User Interface Library; if not, write
# to the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
# Boston, MA 02111-1307 USA
########################################################################

########################################################################
# Determine the Linux distribution base and the command to install OS
# packages
########################################################################

LINUXDISTRO=Unknown
PACKAGE_INSTALL_CMD=
LINUXDISTRO_ID=$(sed -n "s/^ID=\(.*\)$/\1/p" /etc/os-release)
LINUXDISTRO_ID=$(sed "s/^\(\"\)\(.*\)\1\$/\2/g" <<<"$LINUXDISTRO_ID")
if [[ "${LINUXDISTRO_ID}" == "fedora" || "${LINUXDISTRO_ID}" == "centos" || "${LINUXDISTRO_ID}" == "rhel" ]]; then
	LINUXDISTRO=Fedora
	PACKAGE_INSTALL_CMD="dnf install"
elif [[ "${LINUXDISTRO_ID}" == "ubuntu" ]]; then
	LINUXDISTRO=Ubuntu
	PACKAGE_INSTALL_CMD="apt-get install"
elif [[ "${LINUXDISTRO_ID}" == "linuxmint" ]]; then
	LINUXDISTRO=Mint
	PACKAGE_INSTALL_CMD="apt-get install"
else
	echo -e "\033[0;31mUnable to determine the Linux distribution type on this host, prerequisites need to be installed manually\033[0m"
	exit 1
fi
LINUXDISTRO_VERSION=$(sed -n "s/^VERSION_ID=\(.*\)$/\1/p" /etc/os-release)

########################################################################
# Build lists of required, optional, and very optional system packages
########################################################################

if [[ "${LINUXDISTRO}" == "Fedora" ]]; then
	#
	# Build a list of required packages:
	#
	
	# Basic software build support
	REQUIRED_PKGS=(gcc-c++)
	
	# Basic packages
	REQUIRED_PKGS+=(zlib-devel openssl-devel)
	
	# Hardware abstraction packages
	REQUIRED_PKGS+=(libudev-devel libusb1-devel)
	
	# 2D and 3D desktop graphics and input device abstraction packages
	REQUIRED_PKGS+=(mesa-libGL-devel mesa-libGLU-devel)
	
	# If SteamVR is installed, add the Vulkan libraries and GLSL compiler for the VR compositor
	if [ -n "${STEAMVRVERSION}" ]; then
		REQUIRED_PKGS+=(vulkan-headers vulkan-loader-devel vulkan-validation-layers-devel glslc)
	fi
	
	#
	# Build a list of optional packages:
	#
	
	# Basic packages
	OPTIONAL_PKGS=(dbus-devel freetype-devel)
	
	# Hardware abstraction packages
	OPTIONAL_PKGS+=(bluez-libs-devel)
	
	# Image file format handling libraries
	OPTIONAL_PKGS+=(libpng-devel libjpeg-devel libtiff-devel)
	
	# Sound libraries
	OPTIONAL_PKGS+=(alsa-lib-devel pulseaudio-libs-devel openal-soft-devel speex-devel)
	
	# Video codec and video device support libraries
	OPTIONAL_PKGS+=(libv4l-devel libdc1394-devel libogg-devel libtheora-devel)
	
	# 2D and 3D desktop graphics and input device abstraction packages
	OPTIONAL_PKGS+=(libXi-devel libXrandr-devel)
	
	# Basic TrueType fonts
	OPTIONAL_PKGS+=(gnu-free-fonts-common gnu-free-serif-fonts gnu-free-sans-fonts gnu-free-mono-fonts)
	
	#
	# Build a list of very optional packages:
	#
	
	OPTIONAL2_PKGS=(ffmpeg xine-ui xine-lib-devel)
elif [[ "${LINUXDISTRO}" == "Ubuntu" || "${LINUXDISTRO}" == "Mint" ]]; then
	#
	# Build a list of required packages:
	#
	
	# Basic software build support
	REQUIRED_PKGS=(build-essential g++)
	
	# Basic packages
	REQUIRED_PKGS+=(zlib1g-dev libssl-dev)
	
	# Hardware abstraction packages
	REQUIRED_PKGS+=(libudev-dev libusb-1.0-0-dev)
	
	# 2D and 3D desktop graphics and input device abstraction packages
	REQUIRED_PKGS+=(mesa-common-dev libgl1-mesa-dev libglu1-mesa-dev)
	
	# If SteamVR is installed, add the Vulkan libraries for the VR compositor
	if [ -n "${STEAMVRVERSION}" ]; then
		REQUIRED_PKGS+=(libvulkan-dev vulkan-validationlayers-dev)
	fi
	
	#
	# Build a list of optional packages:
	#
	
	# Basic packages
	OPTIONAL_PKGS=(libdbus-1-dev libfreetype6-dev)
	
	# Hardware abstraction packages
	OPTIONAL_PKGS+=(libbluetooth-dev)
	
	# Image file format handling libraries
	OPTIONAL_PKGS+=(libpng-dev libjpeg-dev libtiff-dev)
	
	# Sound libraries
	OPTIONAL_PKGS+=(libasound2-dev libpulse-dev libopenal-dev libspeex-dev libopus-dev)
	
	# Video codec and video device support libraries
	OPTIONAL_PKGS+=(libv4l-dev libogg-dev libtheora-dev)
	
	# 2D and 3D desktop graphics and input device abstraction packages
	OPTIONAL_PKGS+=(libxi-dev libxrandr-dev)
	
	# Basic TrueType fonts
	OPTIONAL_PKGS+=(fonts-freefont-ttf)
	
	#
	# Build a list of very optional packages:
	#
	
	OPTIONAL2_PKGS=()
	if [[ "${LINUXDISTRO}" == "Ubuntu" ]]; then
		OPTIONAL2_PKGS+=(libcd1394-dev)
	elif [[ "${LINUXDISTRO}" == "Mint" ]]; then
		OPTIONAL2_PKGS+=(libcd1394-dev)
	fi
	OPTIONAL2_PKGS+=(ffmpeg libxine2-dev libxine2-plugins libxine2-ffmpeg)
else
	echo -e "\033[0;31mUnsupported Linux distribution ${LINUXDISTRO}\033[0m"
	exit 2
fi

# Try installing required and all optional packages
sudo ${PACKAGE_INSTALL_CMD} "${REQUIRED_PKGS[@]}" "${OPTIONAL_PKGS[@]}" "${OPTIONAL2_PKGS[@]}"
if [[ $? -ne 0 ]]; then
	# Try installing required packages and main optional packages
	sudo ${PACKAGE_INSTALL_CMD} "${REQUIRED_PKGS[@]}" "${OPTIONAL_PKGS[@]}"
	if [[ $? -ne 0 ]]; then
		# Warn the user that some functionality will be disabled
		echo -e "\033[0;32mWarning: Unable to install some optional OS packages; some functionality might be disabled\033[0m"
	
		# Install required packages only
		sudo ${PACKAGE_INSTALL_CMD} "${REQUIRED_PKGS[@]}"
		if [[ $? -ne 0 ]]; then
			echo -e "\033[0;31mUnable to install some required OS packages; please fix the issue and try again\033[0m"
			exit 3
		else
			echo -e "\033[0;32mAll required system packages successfully installed\033[0m"
		fi
	else
		echo -e "\033[0;32mAll required and optional system packages successfully installed\033[0m"
	fi
else
	echo -e "\033[0;32mAll required, optional, and very optional system packages successfully installed\033[0m"
fi
