/***********************************************************************
Rational - Class for rational numbers.
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

#ifndef MATH_RATIONAL_INCLUDED
#define MATH_RATIONAL_INCLUDED

#include <stddef.h>

namespace Math {

class Rational
	{
	/* Elements: */
	public:
	static const Rational infinity; // Pseudo-rational number representing positive infinity
	static const Rational nan; // Pseudo-rational number representing "not a number"
	private:
	int numerator,denominator; // The two components of the rational number. Invariants: denominator>0; gcd(numerator,denominator)==1
	
	/* Private methods: */
	static long gcd(long a,long b); // Calculates the greatest common divisor of the two non-negative integers
	void normalize(long newNumerator,long newDenominator); // Normalizes the given rational number to satisfy the invariants
	
	/* Constructors and destructors: */
	public:
	Rational(void) // Dummy constructor; creates not-a-number
		:numerator(0),denominator(0)
		{
		}
	explicit Rational(int sNumerator) // Creates rational number from integer
		:numerator(sNumerator),denominator(1)
		{
		}
	Rational(int sNumerator,int sDenominator) // Element-wise constructor
		{
		normalize(sNumerator,sDenominator);
		}
	
	/* Methods: */
	int getNumerator(void) const // Returns the numerator
		{
		return numerator;
		}
	int getDenominator(void) const // Returns the denominator
		{
		return denominator;
		}
	bool isFinite(void) const // Returns true if the rational number is really a number
		{
		return denominator!=0;
		}
	operator double(void) const // Converts the rational number into a floating-point number
		{
		return double(numerator)/double(denominator);
		}
	int floor(void) const; // Returns the largest integer not larger than the rational number; throws exception if number is not finite
	
	/* Comparison operators: */
	bool operator==(const Rational& other) const
		{
		return numerator==other.numerator&&denominator==other.denominator;
		}
	bool operator!=(const Rational& other) const
		{
		return numerator!=other.numerator||denominator!=other.denominator;
		}
	bool operator<(const Rational& other) const
		{
		return long(numerator)*long(other.denominator)<long(other.numerator)*long(denominator);
		}
	bool operator<=(const Rational& other) const
		{
		return long(numerator)*long(other.denominator)<=long(other.numerator)*long(denominator);
		}
	bool operator>=(const Rational& other) const
		{
		return long(numerator)*long(other.denominator)>=long(other.numerator)*long(denominator);
		}
	bool operator>(const Rational& other) const
		{
		return long(numerator)*long(other.denominator)>long(other.numerator)*long(denominator);
		}
	
	/* Unary arithmetic operators: */
	Rational operator+(void) const // Unary plus
		{
		return *this;
		}
	Rational operator-(void) const // Unary minus
		{
		return Rational(-numerator,denominator);
		}
	Rational& invert(void) // Inverts the number
		{
		normalize(denominator,numerator);
		return *this;
		}
	Rational inverse(void) // Returns the inverse number
		{
		return Rational(denominator,numerator);
		}
	
	/* Assignment arithmetic operators: */
	Rational& operator++(void) // Pre-increment
		{
		numerator+=denominator;
		return *this;
		}
	Rational operator++(int) // Post-increment
		{
		Rational result(*this);
		numerator+=denominator;
		return result;
		}
	Rational& operator--(void) // Pre-decrement
		{
		numerator-=denominator;
		return *this;
		}
	Rational operator--(int) // Post-decrement
		{
		Rational result(*this);
		numerator+=denominator;
		return result;
		}
	Rational& operator+=(const Rational& other)
		{
		/* Multiply out the addition and normalize the result: */
		normalize(long(numerator)*long(other.denominator)+long(other.numerator)*long(denominator),long(denominator)*long(other.denominator));
		return *this;
		}
	Rational& operator-=(const Rational& other)
		{
		/* Multiply out the subtraction and normalize the result: */
		normalize(long(numerator)*long(other.denominator)-long(other.numerator)*long(denominator),long(denominator)*long(other.denominator));
		return *this;
		}
	Rational& operator*=(const Rational& other)
		{
		/* Multiply the fractions and normalize the result: */
		normalize(long(numerator)*long(other.numerator),long(denominator)*long(other.denominator));
		return *this;
		}
	Rational& operator/=(const Rational& other)
		{
		/* Divide the fractions and normalize the result: */
		normalize(long(numerator)*long(other.denominator),long(denominator)*long(other.numerator));
		return *this;
		}
	
	/* Arithmetic operators: */
	Rational operator+(const Rational& other) const
		{
		Rational result(*this);
		return result+=other;
		}
	Rational operator-(const Rational& other) const
		{
		Rational result(*this);
		return result-=other;
		}
	Rational operator*(const Rational& other) const
		{
		Rational result(*this);
		return result*=other;
		}
	Rational operator/(const Rational& other) const
		{
		Rational result(*this);
		return result/=other;
		}
	
	static size_t rawHash(const Rational& source) // Raw hash function
		{
		return (size_t(source.numerator)<<32)+size_t(source.numerator)+(size_t(source.denominator)<<16);
		}
	static size_t hash(const Rational& source,size_t tableSize) // Hash function compatible with Misc::HashTable
		{
		return rawHash(source)%tableSize;
		}
	};

}

#endif
