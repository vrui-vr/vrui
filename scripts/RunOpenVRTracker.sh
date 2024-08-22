#!/bin/bash
########################################################################
# Script to set up an environment to run VRDeviceDaemon's OpenVRHost
# device driver module, to receive tracking data for OpenVR / Lighthouse
# tracked devices.
# Copyright (c) 2016-2023 Oliver Kreylos
#
# This file is part of the Vrui VR Device Driver Daemon
# (VRDeviceDaemon).
# 
# The Vrui VR Device Driver Daemon is free software; you can
# redistribute it and/or modify it under the terms of the GNU General
# Public License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
# 
# The Vrui VR Device Driver Daemon is distributed in the hope that it
# will be useful, but WITHOUT ANY WARRANTY; without even the implied
# warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
# the GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with the Vrui VR Device Driver Daemon; if not, write to the Free
# Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
# 02111-1307 USA
########################################################################

# Configure driver and library environment:
STEAMDIR=${HOME}/.steam/steam
RUNTIMEDIR1=${STEAMDIR}/ubuntu12_32/steam-runtime/amd64/lib/x86_64-linux-gnu
RUNTIMEDIR2=${STEAMDIR}/ubuntu12_32/steam-runtime/amd64/usr/lib/x86_64-linux-gnu
STEAMVRDIR=${STEAMDIR}/SteamApps/common/SteamVR
DRIVERDIR=${STEAMVRDIR}/drivers/lighthouse/bin/linux64
STEAMLDPATH=${LD_LIBRARY_PATH}:${RUNTIMEDIR1}:${RUNTIMEDIR2}:${DRIVERDIR}

VRUIBINDIR=/usr/local/bin
VRUIETCDIR=/usr/local/etc

# Include the VR devices configuration file:
source "${VRUIETCDIR}/OpenVRDevices.conf"

# Create the command line to run VRDeviceDaemon:
CMDLINE=(-rootSection OpenVR)
CMDLINE+=(-mergeConfig "VRDevices-${HMD_TYPE}")
CMDLINE+=(-mergeConfig "VRDevices-${CONTROLLER_TYPE}")

# Run VRDeviceDaemon:
( LD_LIBRARY_PATH=${STEAMLDPATH} "${VRUIBINDIR}/VRDeviceDaemon" "${CMDLINE[@]}" )

# Use the following command if things are working, and you don't want to
# see all those "Broken pipe" messages:
# ( LD_LIBRARY_PATH=${STEAMLDPATH} "${VRUIBINDIR}/VRDeviceDaemon" "${CMDLINE[@]}" 2>&1 | fgrep -v ": Broken pipe" )
