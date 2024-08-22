/***********************************************************************
StripPackages - Utility to strip duplicate packages out of a list of
prerequisite packages for improved linking.
Copyright (c) 2020-2021 Oliver Kreylos

This file is part of the WhyTools Build Environment.

The WhyTools Build Environment is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The WhyTools Build Environment is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the WhyTools Build Environment; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <string.h>
#include <stdio.h>

int main(int argc,char* argv[])
	{
	/* Strip duplicate command line arguments from the back: */
	for(int stripIndex=1;argc-stripIndex>1;++stripIndex)
		{
		/* Get the next argument to strip out: */
		char* strip=argv[argc-stripIndex];
		
		/* Remove the argument from earlier in the command line: */
		int insert=1;
		int newArgc=argc;
		for(int test=1;test<argc-stripIndex;++test)
			{
			if(strcmp(strip,argv[test])!=0)
				{
				/* Keep the argument: */
				argv[insert]=argv[test];
				++insert;
				}
			else
				{
				/* Remove the argument: */
				--newArgc;
				}
			}
		
		/* Keep the rest of the command line: */
		for(int i=argc-stripIndex;i<argc;++i,++insert)
			argv[insert]=argv[i];
		argc=newArgc;
		}
	
	/* Print the stripped command line: */
	if(argc>1)
		{
		printf("%s",argv[1]);
		for(int i=2;i<argc;++i)
			printf(" %s",argv[i]);
		printf("\n");
		}
	
	return 0;
	}
