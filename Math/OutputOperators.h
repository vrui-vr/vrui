/***********************************************************************
OutputOperators - Basic iostream output operators for Math objects.
Copyright (c) 2022 Oliver Kreylos

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

#ifndef MATH_OUTPUTOPERATORS_INCLUDED
#define MATH_OUTPUTOPERATORS_INCLUDED

#include <iostream>

/* Forward declarations: */
namespace Math {
class Rational;
template <class ScalarParam>
class Complex;
template <class ScalarParam>
class Interval;
class Matrix;
template <class ScalarParam>
class BrokenLine;
}


namespace std{

ostream& operator<<(ostream& os,const Math::Rational& value);
template <class ScalarParam>
ostream& operator<<(ostream& os,const Math::Complex<ScalarParam>& value);
template <class ScalarParam>
ostream& operator<<(ostream& os,const Math::Interval<ScalarParam>& value);
ostream& operator<<(ostream& os,const Math::Matrix& value);
template <class ScalarParam>
ostream& operator<<(ostream& os,const Math::BrokenLine<ScalarParam>& value);

}

#endif
