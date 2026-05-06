#!/bin/bash
########################################################################
# Script to install a set of source files in a target directory as links
# to the original source files.
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

# Ensure that the target directory exists:
install -m u=rwx,go=rx -d "$DIR"

# Create symbolic links to all source files in the target directory:
while [ $# -gt 0 ]; do
	# Get the name of the source file:
	FILE=$(basename "$1")
	
	# Check if the source file is an absolute or relative path:
	case "$1" in
		/*) ln -sf "$1" "$DIR/$FILE" ;;
		*) ln -sf "$PWD/$1" "$DIR/$FILE" ;;
	esac
	
	# Go to the next source file:
	shift
done

exit 0
