/***********************************************************************
MathValueCoders - Value coder classes for math objects.
Copyright (c) 2009-2022 Oliver Kreylos

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

#include <Math/MathValueCoders.icpp>

#include <Math/Rational.h>

namespace Misc {

/********************************************************
Methods of class ValueCoder<Math::Complex<ScalarParam> >:
********************************************************/

std::string ValueCoder<Math::Rational>::encode(const Math::Rational& value)
	{
	std::string result=ValueCoder<int>::encode(value.getNumerator());
	if(value.getDenominator()!=1)
		{
		result.push_back('/');
		result.append(ValueCoder<int>::encode(value.getDenominator()));
		}
	
	return result;
	}

Math::Rational ValueCoder<Math::Rational>::decode(const char* start,const char* end,const char** decodeEnd)
	{
	try
		{
		const char* cPtr=start;
		
		/* Decode the numerator: */
		int numerator=ValueCoder<int>::decode(cPtr,end,&cPtr);
		
		/* Check for a division sign: */
		int denominator=1;
		if(cPtr!=end&&*cPtr=='/')
			{
			++cPtr;
			
			/* Decode the denominator: */
			denominator=ValueCoder<int>::decode(cPtr,end,&cPtr);
			}
		
		/* Set the end of decoding pointer: */
		if(decodeEnd!=0)
			*decodeEnd=cPtr;
		
		/* Return the decoded value: */
		return Math::Rational(numerator,denominator);
		}
	catch(const std::runtime_error& err)
		{
		throw DecodingError(std::string("Unable to convert ")+std::string(start,end)+std::string(" to Math::Rational due to ")+err.what());
		}
	}

/******************************************************
Force instantiation of all standard ValueCoder classes:
******************************************************/

template class ValueCoder<Math::Complex<float> >;
template class ValueCoder<Math::Complex<double> >;

template class ValueCoder<Math::Interval<int> >;
template class ValueCoder<Math::Interval<float> >;
template class ValueCoder<Math::Interval<double> >;

template class ValueCoder<Math::BrokenLine<float> >;
template class ValueCoder<Math::BrokenLine<double> >;

}
