#!/bin/bash
########################################################################
# Script to run Vrui's VR compositing server.
# Copyright (c) 2023-2024 Oliver Kreylos
# 
# This file is part of the Vrui VR Compositing Server (VRCompositor).
# 
# The Vrui VR Compositing Server is free software; you can redistribute
# it and/or modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.
# 
# The Vrui VR Compositing Server is distributed in the hope that it will
# be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with the Vrui VR Compositing Server; if not, write to the Free
# Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
# 02111-1307 USA
########################################################################

# Configure library environment:
VRUIBINDIR=/usr/local/bin
VRUIETCDIR=/usr/local/etc

# Include the VR devices configuration file:
source "${VRUIETCDIR}/OpenVRDevices.conf"

# Create the command line to run VRCompositingServer:
CMDLINE=(-hmd "${HMD_TYPE}")
CMDLINE+=(-frameRate "${HMD_REFRESH}")

# Run VRCompositingServer:
"$VRUIBINDIR/VRCompositingServer" "${CMDLINE[@]}"
