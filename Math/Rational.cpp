/***********************************************************************
Rational - Class for rational numbers.
Copyright (c) 2022-2024 Oliver Kreylos

This file is part of the Templatized Math Library (Math).

The Templatized Math Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Templatized Math Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Templatized Math Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <Math/Rational.h>

#include <stdlib.h>
#include <utility>
#include <Misc/StdError.h>

namespace Math {

/*********************************
Static elements of class Rational:
*********************************/

const Rational Rational::infinity(1,0);
const Rational Rational::nan(0,0);

/*************************
Methods of class Rational:
*************************/

long Rational::gcd(long a,long b)
	{
	/* Calculate the gcd using Euclid's algorithm: */
	if(a<b)
		std::swap(a,b);
	while(b!=0)
		{
		long newB=a%b;
		a=b;
		b=newB;
		}
	
	return a;
	}

void Rational::normalize(long newNumerator,long newDenominator)
	{
	/* Ensure that the denominator is non-negative: */
	if(newDenominator<0)
		{
		newNumerator=-newNumerator;
		newDenominator=-newDenominator;
		}
	
	/* Ensure that the fraction is fully reduced: */
	if(newDenominator>0)
		{
		/* Find the greatest common divisor between the numerator and denominator: */
		long d=gcd(labs(newNumerator),newDenominator);
		
		/* Reduce the fraction: */
		newNumerator/=d;
		newDenominator/=d;
		}
	else if(newNumerator<0)
		newNumerator=-1;
	else if(newNumerator>0)
		newNumerator=1;
	
	/* Assign the normalized fraction: */
	numerator=int(newNumerator);
	denominator=int(newDenominator);
	}

int Rational::floor(void) const
	{
	/* Check for non-finite numbers: */
	if(denominator==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"Number is not finite");
	
	if(numerator>=0)
		return numerator/denominator;
	else
		return (numerator-(denominator-1))/denominator;
	}

}
