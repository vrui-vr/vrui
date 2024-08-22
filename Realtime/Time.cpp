/***********************************************************************
Time - Wrapper classes for absolute and relative time measured from one
of a variety of POSIX clocks.
Copyright (c) 2014-2024 Oliver Kreylos

This file is part of the Realtime Processing Library (Realtime).

The Realtime Processing Library is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Realtime Processing Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Realtime Processing Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <Realtime/Time.h>

#include <errno.h>
#include <sys/time.h>
#include <Misc/StdError.h>

namespace Realtime {

/*********************
Methods of class Time:
*********************/

#if !REALTIME_CONFIG_HAVE_POSIX_CLOCKS

void Time::getCurrentTime(timespec* time)
	{
	timeval currentTime;
	gettimeofday(&currentTime,0);
	time->tv_sec=currentTime.tv_sec;
	time->tv_nsec=long(currentTime.tv_usec)*1000L;
	}

#endif

void Time::sleepUntil(clockid_t clockId,const Time& wakeupTime)
	{
	#if REALTIME_CONFIG_HAVE_POSIX_CLOCKS	
	
	/* Call clock_nanosleep repeatedly if it gets interrupted: */
	int sleepResult;
	while((sleepResult=clock_nanosleep(clockId,TIMER_ABSTIME,&wakeupTime,0))==EINTR)
		;
	
	/* Throw an exception if the sleep did not succeed: */
	if(sleepResult!=0)
		throw Misc::makeLibcErr(__PRETTY_FUNCTION__,sleepResult,"Unable to sleep");
	
	#else
	
	/* Emulate clock_nanosleep using nanosleep: */
	while(true)
		{
		/* Get the current time: */
		timeval current;
		gettimeofday(&current,0);
		
		/* Calculate the sleep interval: */
		Time sleep(wakeupTime);
		sleep.subtract(current.tv_sec,current.tv_usec*1000L);
		
		/* Check if already passed: */
		if(sleep.tv_sec<0)
			break;
		
		/* Sleep for a while: */
		nanosleep(&sleep,0);
		}
	
	#endif
	}












}
