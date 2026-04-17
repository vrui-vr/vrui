#!/bin/bash
########################################################################
# Script to configure the etc/OpenVRDevices.conf file based on the type
# of head-mounted display connected to the host.
# Copyright (c) 2026 Oliver Kreylos
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

########################################################################
# Script argument list:
########################################################################

# $1 - Vrui binary directory
# $2 - Vrui etc directory
# $3 - Name of configuration file

VRUIBINDIR="$1"
VRUIETCDIR="$2"
CONFIGFILE="$3"

# Determine the type of HMD:
HMD_TYPE=$($VRUIBINDIR/FindHMD)

# Copy the template configuration file:
cp "$VRUIETCDIR/$CONFIGFILE.template" "$VRUIETCDIR/$CONFIGFILE"

case "$HMD_TYPE" in
	"HTC Vive"*)
		sed -i -e "s@HMD_TYPE=.*@HMD_TYPE=\"$HMD_TYPE\"@" "$VRUIETCDIR/$CONFIGFILE"
		sed -i -e "s@HMD_REFRESH=.*@HMD_REFRESH=\"90\"@" "$VRUIETCDIR/$CONFIGFILE"
		sed -i -e "s@CONTROLLER_TYPE=.*@CONTROLLER_TYPE=\"HTC Vive Wands\"@" "$VRUIETCDIR/$CONFIGFILE"
		;;
	"Valve Index")
		sed -i -e "s@HMD_TYPE=.*@HMD_TYPE=\"$HMD_TYPE\"@" "$VRUIETCDIR/$CONFIGFILE"
		sed -i -e "s@HMD_REFRESH=.*@HMD_REFRESH=\"90\"@" "$VRUIETCDIR/$CONFIGFILE"
		sed -i -e "s@CONTROLLER_TYPE=.*@CONTROLLER_TYPE=\"Valve Index Controllers\"@" "$VRUIETCDIR/$CONFIGFILE"
		;;
	*)
		sed -i -e "s@HMD_TYPE=.*@HMD_TYPE=\"\"@" "$VRUIETCDIR/$CONFIGFILE"
		sed -i -e "s@HMD_REFRESH=.*@HMD_REFRESH=\"90\"@" "$VRUIETCDIR/$CONFIGFILE"
		sed -i -e "s@CONTROLLER_TYPE=.*@CONTROLLER_TYPE=\"\"@" "$VRUIETCDIR/$CONFIGFILE"
		;;
esac
