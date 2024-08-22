/***********************************************************************
Types - Forward declarations of data types used by the Vrui library.
Copyright (c) 2005-2023 Oliver Kreylos

This file is part of the Virtual Reality User Interface Library (Vrui).

The Virtual Reality User Interface Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Virtual Reality User Interface Library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Virtual Reality User Interface Library; if not, write to the
Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef VRUI_TYPES_INCLUDED
#define VRUI_TYPES_INCLUDED

/********************
Forward declarations:
********************/

namespace Misc {
template <unsigned int numComponentsParam>
class Offset;
template <unsigned int numComponentsParam>
class Size;
template <unsigned int numComponentsParam>
class Rect;
}
namespace Realtime {
class TimeVector;
class TimePointMonotonic;
class TimePointRealtime;
}
namespace Geometry {
template <class ScalarParam,int dimensionParam>
class ComponentArray;
template <class ScalarParam,int dimensionParam>
class Point;
template <class ScalarParam,int dimensionParam>
class Vector;
template <class ScalarParam,int dimensionParam>
class HVector;
template <class ScalarParam,int dimensionParam>
class Rotation;
template <class ScalarParam,int dimensionParam>
class OrthonormalTransformation;
template <class ScalarParam,int dimensionParam>
class OrthogonalTransformation;
template <class ScalarParam,int dimensionParam>
class AffineTransformation;
template <class ScalarParam,int dimensionParam>
class ProjectiveTransformation;
template <class ScalarParam,int dimensionParam>
class Ray;
template <class ScalarParam,int dimensionParam>
class Plane;
}
template <class ScalarParam,int numComponentsParam>
class GLColor;

/**************************
Declarations of Vrui types:
**************************/

namespace Vrui {

/* Basic types: */
typedef Misc::Offset<2> IOffset; // Type for 2D integer offsets for windows, images, frame buffers, etc.
typedef Misc::Size<2> ISize; // Type for 2D integer sizes for windows, images, frame buffers, etc.
typedef Misc::Rect<2> IRect; // Type for 2D integer rectangles
typedef Realtime::TimeVector TimeVector; // Type for time intervals
typedef Realtime::TimePointMonotonic TimePoint; // Type for absolute points on a time line not affected by updates
typedef Realtime::TimePointRealtime WallClockTime; // Type for absolute points on the real-time time line

/* Basic (non-usage specific) geometry data types: */
typedef double Scalar; // Scalar type of Vrui's affine space
typedef Geometry::ComponentArray<Scalar,3> Size; // Size of an object along the primary axes
typedef Geometry::Point<Scalar,3> Point; // Point in affine space
typedef Geometry::Vector<Scalar,3> Vector; // Vector in affine space
typedef Geometry::HVector<Scalar,3> HVector; // Homogeneous vector in projective space
typedef Geometry::Rotation<Scalar,3> Rotation; // Affine rotation
typedef Geometry::OrthonormalTransformation<Scalar,3> ONTransform; // Rigid body transformation (translation+rotation)
typedef Geometry::OrthogonalTransformation<Scalar,3> OGTransform; // Rigid body transformation with uniform scaling
typedef Geometry::AffineTransformation<Scalar,3> ATransform; // General affine transformation
typedef Geometry::ProjectiveTransformation<Scalar,3> PTransform; // General projective transformation
typedef Geometry::Ray<Scalar,3> Ray; // Affine ray (half-line)
typedef Geometry::Plane<Scalar,3> Plane; // Plane (half-space)

/* Usage-specific geometry data types: */
typedef ONTransform TrackerState; // Type for raw (untransformed) tracker states
typedef OGTransform NavTransform; // Type for navigation transformations
typedef OGTransform NavTrackerState; // Type for user (transformed) tracker states

/* Basic rendering types: */
typedef GLColor<float,4> Color; // Type for colors

}

#endif
