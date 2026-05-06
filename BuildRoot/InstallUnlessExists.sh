#!/bin/bash
########################################################################
# Script to install a set of source files in a target directory, but not
# to overwrite any existing files and instead mark new versions of
# existing files with a .new suffix.
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

# $1 - name of target directory
# $2-$n - names of files to install

# Get the name of the target directory:
DIR="$1"
shift

# Get the current user name and group:
USER=$(id -u)
GROUP=$(id -g)

if [ -d "$DIR" ]; then
	# Install source files one at a time:
	while [ $# -gt 0 ]; do
		# Get the name of the source file:
		FILE=$(basename "$1")
		
		# Check if a file of the same name as the source file exists in the target directory:
		if [ -e "$DIR/$FILE" ]; then
			# Check if the existing file and the source file are different:
			diff -q "$DIR/$FILE" "$1" > /dev/null
			if [ $? -eq 1 ]; then
				# Install the source file in the target directory with a .new suffix:
				NEWNAME=$DIR/$FILE.new
				echo "Installing changed new file $NEWNAME"
				cp "$1" "$NEWNAME"
				chown "$USER:$GROUP" "$NEWNAME"
				chmod u=rw,go=r "$NEWNAME"
			fi
		else
			# Install the source file in the target directory:
			install -m u=rw,go=r "$1" "$DIR"
		fi
		
		# Go to the next source file:
		shift
	done
fi

exit 0
