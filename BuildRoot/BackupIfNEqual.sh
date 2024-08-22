#!/bin/bash
########################################################################
# Script to back up files in a target directory that are different from
# files of the same name in a source directory.
# Copyright (c) 2023 Oliver Kreylos
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
# $2-$n - names of files to compare and back up if different

# Get the name of the target directory:
DIR="$1"
shift

if [ -d "$DIR" ]; then
	# Compare and back up command line arguments one at a time:
	while [ $# -gt 0 ]; do
		# Get the name of the source file:
		FILE=$(basename "$1")
		
		# Check if a file of the same name as the source file exists in the target directory:
		if [ -e "$DIR/$FILE" ]; then
			# Compare the source file and the target directory file:
			diff -q "$DIR/$FILE" "$1" > /dev/null
			if [ $? -eq 1 ]; then
				# Check if a backup file of that name already exists:
				if [ -e "$DIR/$FILE~" ]; then
					# Print a warning
					echo File $DIR/$FILE cannot be backed up because a backup already exists
				else
					# Back up the target directory file:
					echo Backing up $DIR/$FILE...
					mv "$DIR/$FILE" "$DIR/$FILE~"
				fi
			fi
		fi
		
		# Go to the next source file:
		shift
	done
fi

exit 0
