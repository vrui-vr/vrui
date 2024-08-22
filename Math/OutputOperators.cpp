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

#include <Math/OutputOperators.icpp>

#include <iomanip>
#include <Math/Rational.h>
#include <Math/Matrix.h>

namespace std {

ostream& operator<<(ostream& os,const Math::Rational& value)
	{
	os<<value.getNumerator();
	if(value.getDenominator()!=1)
		os<<'/'<<value.getDenominator();
	
	return os;
	}

ostream& operator<<(ostream& os,const Math::Matrix& value)
	{
	streamsize width=os.width();
	os<<setw(1)<<'{';
	for(unsigned int i=0;i<value.getNumRows();++i)
		{
		if(i>0)
			os<<", ";
		os<<'{'<<setw(width)<<value(i,0);
		for(unsigned int j=1;j<value.getNumColumns();++j)
			os<<", "<<setw(width)<<value(i,j);
		os<<'}';
		}
	os<<'}';
	
	return os;
	}

/****************************************************
Force instantiation of all standard output functions:
****************************************************/

template ostream& operator<<(ostream&,const Math::Complex<float>&);
template ostream& operator<<(ostream&,const Math::Complex<double>&);

template ostream& operator<<(ostream&,const Math::Interval<int>&);
template ostream& operator<<(ostream&,const Math::Interval<float>&);
template ostream& operator<<(ostream&,const Math::Interval<double>&);

template ostream& operator<<(ostream&,const Math::BrokenLine<float>&);
template ostream& operator<<(ostream&,const Math::BrokenLine<double>&);

}
