/***********************************************************************
GLEXTDirectStateAccess - OpenGL extension class for the
GL_EXT_direct_state_access extension.
Copyright (c) 2022 Oliver Kreylos

This file is part of the OpenGL Support Library (GLSupport).

The OpenGL Support Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The OpenGL Support Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the OpenGL Support Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef GLEXTENSIONS_GLEXTDIRECTSTATEACCESS_INCLUDED
#define GLEXTENSIONS_GLEXTDIRECTSTATEACCESS_INCLUDED

#include <GL/gl.h>
#include <GL/TLSHelper.h>
#include <GL/Extensions/GLExtension.h>

/********************************
Extension-specific parts of gl.h:
********************************/

#ifndef GL_EXT_direct_state_access
#define GL_EXT_direct_state_access 1

/* Extension-specific functions: */
typedef void (APIENTRY* PFNGLMATRIXLOADFEXTPROC)(GLenum mode,const GLfloat* m);
typedef void (APIENTRY* PFNGLMATRIXLOADDEXTPROC)(GLenum mode,const GLdouble* m);
typedef void (APIENTRY* PFNGLMATRIXMULTFEXTPROC)(GLenum mode,const GLfloat* m);
typedef void (APIENTRY* PFNGLMATRIXMULTDEXTPROC)(GLenum mode,const GLdouble* m);
typedef void (APIENTRY* PFNGLMATRIXLOADIDENTITYEXTPROC)(GLenum mode);
typedef void (APIENTRY* PFNGLMATRIXROTATEFEXTPROC)(GLenum mode,GLfloat angle,GLfloat x,GLfloat y,GLfloat z);
typedef void (APIENTRY* PFNGLMATRIXROTATEDEXTPROC)(GLenum mode,GLdouble angle,GLdouble x,GLdouble y,GLdouble z);
typedef void (APIENTRY* PFNGLMATRIXSCALEFEXTPROC)(GLenum mode,GLfloat x,GLfloat y,GLfloat z);
typedef void (APIENTRY* PFNGLMATRIXSCALEDEXTPROC)(GLenum mode,GLdouble x,GLdouble y,GLdouble z);
typedef void (APIENTRY* PFNGLMATRIXTRANSLATEFEXTPROC)(GLenum mode,GLfloat x,GLfloat y,GLfloat z);
typedef void (APIENTRY* PFNGLMATRIXTRANSLATEDEXTPROC)(GLenum mode,GLdouble x,GLdouble y,GLdouble z);
typedef void (APIENTRY* PFNGLMATRIXFRUSTUMEXTPROC)(GLenum mode,GLdouble left,GLdouble right,GLdouble bottom,GLdouble top,GLdouble zNear,GLdouble zFar);
typedef void (APIENTRY* PFNGLMATRIXORTHOEXTPROC)(GLenum mode,GLdouble left,GLdouble right,GLdouble bottom,GLdouble top,GLdouble zNear,GLdouble zFar);
typedef void (APIENTRY* PFNGLMATRIXPOPEXTPROC)(GLenum mode);
typedef void (APIENTRY* PFNGLMATRIXPUSHEXTPROC)(GLenum mode);
typedef void (APIENTRY* PFNGLCLIENTATTRIBDEFAULTEXTPROC)(GLbitfield mask);
typedef void (APIENTRY* PFNGLPUSHCLIENTATTRIBDEFAULTEXTPROC)(GLbitfield mask);
typedef void (APIENTRY* PFNGLTEXTUREPARAMETERFEXTPROC)(GLuint texture,GLenum target,GLenum pname,GLfloat param);
typedef void (APIENTRY* PFNGLTEXTUREPARAMETERFVEXTPROC)(GLuint texture,GLenum target,GLenum pname,const GLfloat* params);
typedef void (APIENTRY* PFNGLTEXTUREPARAMETERIEXTPROC)(GLuint texture,GLenum target,GLenum pname,GLint param);
typedef void (APIENTRY* PFNGLTEXTUREPARAMETERIVEXTPROC)(GLuint texture,GLenum target,GLenum pname,const GLint* params);
typedef void (APIENTRY* PFNGLTEXTUREIMAGE1DEXTPROC)(GLuint texture,GLenum target,GLint level,GLint internalformat,GLsizei width,GLint border,GLenum format,GLenum type,const void* pixels);
typedef void (APIENTRY* PFNGLTEXTUREIMAGE2DEXTPROC)(GLuint texture,GLenum target,GLint level,GLint internalformat,GLsizei width,GLsizei height,GLint border,GLenum format,GLenum type,const void* pixels);
typedef void (APIENTRY* PFNGLTEXTURESUBIMAGE1DEXTPROC)(GLuint texture,GLenum target,GLint level,GLint xoffset,GLsizei width,GLenum format,GLenum type,const void* pixels);
typedef void (APIENTRY* PFNGLTEXTURESUBIMAGE2DEXTPROC)(GLuint texture,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLsizei width,GLsizei height,GLenum format,GLenum type,const void* pixels);
typedef void (APIENTRY* PFNGLCOPYTEXTUREIMAGE1DEXTPROC)(GLuint texture,GLenum target,GLint level,GLenum internalformat,GLint x,GLint y,GLsizei width,GLint border);
typedef void (APIENTRY* PFNGLCOPYTEXTUREIMAGE2DEXTPROC)(GLuint texture,GLenum target,GLint level,GLenum internalformat,GLint x,GLint y,GLsizei width,GLsizei height,GLint border);
typedef void (APIENTRY* PFNGLCOPYTEXTURESUBIMAGE1DEXTPROC)(GLuint texture,GLenum target,GLint level,GLint xoffset,GLint x,GLint y,GLsizei width);
typedef void (APIENTRY* PFNGLCOPYTEXTURESUBIMAGE2DEXTPROC)(GLuint texture,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLint x,GLint y,GLsizei width,GLsizei height);
typedef void (APIENTRY* PFNGLGETTEXTUREIMAGEEXTPROC)(GLuint texture,GLenum target,GLint level,GLenum format,GLenum type,void* pixels);
typedef void (APIENTRY* PFNGLGETTEXTUREPARAMETERFVEXTPROC)(GLuint texture,GLenum target,GLenum pname,GLfloat* params);
typedef void (APIENTRY* PFNGLGETTEXTUREPARAMETERIVEXTPROC)(GLuint texture,GLenum target,GLenum pname,GLint* params);
typedef void (APIENTRY* PFNGLGETTEXTURELEVELPARAMETERFVEXTPROC)(GLuint texture,GLenum target,GLint level,GLenum pname,GLfloat* params);
typedef void (APIENTRY* PFNGLGETTEXTURELEVELPARAMETERIVEXTPROC)(GLuint texture,GLenum target,GLint level,GLenum pname,GLint* params);
typedef void (APIENTRY* PFNGLTEXTUREIMAGE3DEXTPROC)(GLuint texture,GLenum target,GLint level,GLint internalformat,GLsizei width,GLsizei height,GLsizei depth,GLint border,GLenum format,GLenum type,const void* pixels);
typedef void (APIENTRY* PFNGLTEXTURESUBIMAGE3DEXTPROC)(GLuint texture,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLint zoffset,GLsizei width,GLsizei height,GLsizei depth,GLenum format,GLenum type,const void* pixels);
typedef void (APIENTRY* PFNGLCOPYTEXTURESUBIMAGE3DEXTPROC)(GLuint texture,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLint zoffset,GLint x,GLint y,GLsizei width,GLsizei height);
typedef void (APIENTRY* PFNGLBINDMULTITEXTUREEXTPROC)(GLenum texunit,GLenum target,GLuint texture);
typedef void (APIENTRY* PFNGLMULTITEXCOORDPOINTEREXTPROC)(GLenum texunit,GLint size,GLenum type,GLsizei stride,const void* pointer);
typedef void (APIENTRY* PFNGLMULTITEXENVFEXTPROC)(GLenum texunit,GLenum target,GLenum pname,GLfloat param);
typedef void (APIENTRY* PFNGLMULTITEXENVFVEXTPROC)(GLenum texunit,GLenum target,GLenum pname,const GLfloat* params);
typedef void (APIENTRY* PFNGLMULTITEXENVIEXTPROC)(GLenum texunit,GLenum target,GLenum pname,GLint param);
typedef void (APIENTRY* PFNGLMULTITEXENVIVEXTPROC)(GLenum texunit,GLenum target,GLenum pname,const GLint* params);
typedef void (APIENTRY* PFNGLMULTITEXGENDEXTPROC)(GLenum texunit,GLenum coord,GLenum pname,GLdouble param);
typedef void (APIENTRY* PFNGLMULTITEXGENDVEXTPROC)(GLenum texunit,GLenum coord,GLenum pname,const GLdouble* params);
typedef void (APIENTRY* PFNGLMULTITEXGENFEXTPROC)(GLenum texunit,GLenum coord,GLenum pname,GLfloat param);
typedef void (APIENTRY* PFNGLMULTITEXGENFVEXTPROC)(GLenum texunit,GLenum coord,GLenum pname,const GLfloat* params);
typedef void (APIENTRY* PFNGLMULTITEXGENIEXTPROC)(GLenum texunit,GLenum coord,GLenum pname,GLint param);
typedef void (APIENTRY* PFNGLMULTITEXGENIVEXTPROC)(GLenum texunit,GLenum coord,GLenum pname,const GLint* params);
typedef void (APIENTRY* PFNGLGETMULTITEXENVFVEXTPROC)(GLenum texunit,GLenum target,GLenum pname,GLfloat* params);
typedef void (APIENTRY* PFNGLGETMULTITEXENVIVEXTPROC)(GLenum texunit,GLenum target,GLenum pname,GLint* params);
typedef void (APIENTRY* PFNGLGETMULTITEXGENDVEXTPROC)(GLenum texunit,GLenum coord,GLenum pname,GLdouble* params);
typedef void (APIENTRY* PFNGLGETMULTITEXGENFVEXTPROC)(GLenum texunit,GLenum coord,GLenum pname,GLfloat* params);
typedef void (APIENTRY* PFNGLGETMULTITEXGENIVEXTPROC)(GLenum texunit,GLenum coord,GLenum pname,GLint* params);
typedef void (APIENTRY* PFNGLMULTITEXPARAMETERIEXTPROC)(GLenum texunit,GLenum target,GLenum pname,GLint param);
typedef void (APIENTRY* PFNGLMULTITEXPARAMETERIVEXTPROC)(GLenum texunit,GLenum target,GLenum pname,const GLint* params);
typedef void (APIENTRY* PFNGLMULTITEXPARAMETERFEXTPROC)(GLenum texunit,GLenum target,GLenum pname,GLfloat param);
typedef void (APIENTRY* PFNGLMULTITEXPARAMETERFVEXTPROC)(GLenum texunit,GLenum target,GLenum pname,const GLfloat* params);
typedef void (APIENTRY* PFNGLMULTITEXIMAGE1DEXTPROC)(GLenum texunit,GLenum target,GLint level,GLint internalformat,GLsizei width,GLint border,GLenum format,GLenum type,const void* pixels);
typedef void (APIENTRY* PFNGLMULTITEXIMAGE2DEXTPROC)(GLenum texunit,GLenum target,GLint level,GLint internalformat,GLsizei width,GLsizei height,GLint border,GLenum format,GLenum type,const void* pixels);
typedef void (APIENTRY* PFNGLMULTITEXSUBIMAGE1DEXTPROC)(GLenum texunit,GLenum target,GLint level,GLint xoffset,GLsizei width,GLenum format,GLenum type,const void* pixels);
typedef void (APIENTRY* PFNGLMULTITEXSUBIMAGE2DEXTPROC)(GLenum texunit,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLsizei width,GLsizei height,GLenum format,GLenum type,const void* pixels);
typedef void (APIENTRY* PFNGLCOPYMULTITEXIMAGE1DEXTPROC)(GLenum texunit,GLenum target,GLint level,GLenum internalformat,GLint x,GLint y,GLsizei width,GLint border);
typedef void (APIENTRY* PFNGLCOPYMULTITEXIMAGE2DEXTPROC)(GLenum texunit,GLenum target,GLint level,GLenum internalformat,GLint x,GLint y,GLsizei width,GLsizei height,GLint border);
typedef void (APIENTRY* PFNGLCOPYMULTITEXSUBIMAGE1DEXTPROC)(GLenum texunit,GLenum target,GLint level,GLint xoffset,GLint x,GLint y,GLsizei width);
typedef void (APIENTRY* PFNGLCOPYMULTITEXSUBIMAGE2DEXTPROC)(GLenum texunit,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLint x,GLint y,GLsizei width,GLsizei height);
typedef void (APIENTRY* PFNGLGETMULTITEXIMAGEEXTPROC)(GLenum texunit,GLenum target,GLint level,GLenum format,GLenum type,void* pixels);
typedef void (APIENTRY* PFNGLGETMULTITEXPARAMETERFVEXTPROC)(GLenum texunit,GLenum target,GLenum pname,GLfloat* params);
typedef void (APIENTRY* PFNGLGETMULTITEXPARAMETERIVEXTPROC)(GLenum texunit,GLenum target,GLenum pname,GLint* params);
typedef void (APIENTRY* PFNGLGETMULTITEXLEVELPARAMETERFVEXTPROC)(GLenum texunit,GLenum target,GLint level,GLenum pname,GLfloat* params);
typedef void (APIENTRY* PFNGLGETMULTITEXLEVELPARAMETERIVEXTPROC)(GLenum texunit,GLenum target,GLint level,GLenum pname,GLint* params);
typedef void (APIENTRY* PFNGLMULTITEXIMAGE3DEXTPROC)(GLenum texunit,GLenum target,GLint level,GLint internalformat,GLsizei width,GLsizei height,GLsizei depth,GLint border,GLenum format,GLenum type,const void* pixels);
typedef void (APIENTRY* PFNGLMULTITEXSUBIMAGE3DEXTPROC)(GLenum texunit,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLint zoffset,GLsizei width,GLsizei height,GLsizei depth,GLenum format,GLenum type,const void* pixels);
typedef void (APIENTRY* PFNGLCOPYMULTITEXSUBIMAGE3DEXTPROC)(GLenum texunit,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLint zoffset,GLint x,GLint y,GLsizei width,GLsizei height);
typedef void (APIENTRY* PFNGLENABLECLIENTSTATEINDEXEDEXTPROC)(GLenum array,GLuint index);
typedef void (APIENTRY* PFNGLDISABLECLIENTSTATEINDEXEDEXTPROC)(GLenum array,GLuint index);
typedef void (APIENTRY* PFNGLGETFLOATINDEXEDVEXTPROC)(GLenum target,GLuint index,GLfloat* data);
typedef void (APIENTRY* PFNGLGETDOUBLEINDEXEDVEXTPROC)(GLenum target,GLuint index,GLdouble* data);
typedef void (APIENTRY* PFNGLGETPOINTERINDEXEDVEXTPROC)(GLenum target,GLuint index,void** data);
typedef void (APIENTRY* PFNGLENABLEINDEXEDEXTPROC)(GLenum target,GLuint index);
typedef void (APIENTRY* PFNGLDISABLEINDEXEDEXTPROC)(GLenum target,GLuint index);
typedef GLboolean (APIENTRY* PFNGLISENABLEDINDEXEDEXTPROC)(GLenum target,GLuint index);
typedef void (APIENTRY* PFNGLGETINTEGERINDEXEDVEXTPROC)(GLenum target,GLuint index,GLint* data);
typedef void (APIENTRY* PFNGLGETBOOLEANINDEXEDVEXTPROC)(GLenum target,GLuint index,GLboolean* data);
typedef void (APIENTRY* PFNGLCOMPRESSEDTEXTUREIMAGE3DEXTPROC)(GLuint texture,GLenum target,GLint level,GLenum internalformat,GLsizei width,GLsizei height,GLsizei depth,GLint border,GLsizei imageSize,const void* bits);
typedef void (APIENTRY* PFNGLCOMPRESSEDTEXTUREIMAGE2DEXTPROC)(GLuint texture,GLenum target,GLint level,GLenum internalformat,GLsizei width,GLsizei height,GLint border,GLsizei imageSize,const void* bits);
typedef void (APIENTRY* PFNGLCOMPRESSEDTEXTUREIMAGE1DEXTPROC)(GLuint texture,GLenum target,GLint level,GLenum internalformat,GLsizei width,GLint border,GLsizei imageSize,const void* bits);
typedef void (APIENTRY* PFNGLCOMPRESSEDTEXTURESUBIMAGE3DEXTPROC)(GLuint texture,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLint zoffset,GLsizei width,GLsizei height,GLsizei depth,GLenum format,GLsizei imageSize,const void* bits);
typedef void (APIENTRY* PFNGLCOMPRESSEDTEXTURESUBIMAGE2DEXTPROC)(GLuint texture,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLsizei width,GLsizei height,GLenum format,GLsizei imageSize,const void* bits);
typedef void (APIENTRY* PFNGLCOMPRESSEDTEXTURESUBIMAGE1DEXTPROC)(GLuint texture,GLenum target,GLint level,GLint xoffset,GLsizei width,GLenum format,GLsizei imageSize,const void* bits);
typedef void (APIENTRY* PFNGLGETCOMPRESSEDTEXTUREIMAGEEXTPROC)(GLuint texture,GLenum target,GLint lod,void* img);
typedef void (APIENTRY* PFNGLCOMPRESSEDMULTITEXIMAGE3DEXTPROC)(GLenum texunit,GLenum target,GLint level,GLenum internalformat,GLsizei width,GLsizei height,GLsizei depth,GLint border,GLsizei imageSize,const void* bits);
typedef void (APIENTRY* PFNGLCOMPRESSEDMULTITEXIMAGE2DEXTPROC)(GLenum texunit,GLenum target,GLint level,GLenum internalformat,GLsizei width,GLsizei height,GLint border,GLsizei imageSize,const void* bits);
typedef void (APIENTRY* PFNGLCOMPRESSEDMULTITEXIMAGE1DEXTPROC)(GLenum texunit,GLenum target,GLint level,GLenum internalformat,GLsizei width,GLint border,GLsizei imageSize,const void* bits);
typedef void (APIENTRY* PFNGLCOMPRESSEDMULTITEXSUBIMAGE3DEXTPROC)(GLenum texunit,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLint zoffset,GLsizei width,GLsizei height,GLsizei depth,GLenum format,GLsizei imageSize,const void* bits);
typedef void (APIENTRY* PFNGLCOMPRESSEDMULTITEXSUBIMAGE2DEXTPROC)(GLenum texunit,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLsizei width,GLsizei height,GLenum format,GLsizei imageSize,const void* bits);
typedef void (APIENTRY* PFNGLCOMPRESSEDMULTITEXSUBIMAGE1DEXTPROC)(GLenum texunit,GLenum target,GLint level,GLint xoffset,GLsizei width,GLenum format,GLsizei imageSize,const void* bits);
typedef void (APIENTRY* PFNGLGETCOMPRESSEDMULTITEXIMAGEEXTPROC)(GLenum texunit,GLenum target,GLint lod,void* img);
typedef void (APIENTRY* PFNGLMATRIXLOADTRANSPOSEFEXTPROC)(GLenum mode,const GLfloat* m);
typedef void (APIENTRY* PFNGLMATRIXLOADTRANSPOSEDEXTPROC)(GLenum mode,const GLdouble* m);
typedef void (APIENTRY* PFNGLMATRIXMULTTRANSPOSEFEXTPROC)(GLenum mode,const GLfloat* m);
typedef void (APIENTRY* PFNGLMATRIXMULTTRANSPOSEDEXTPROC)(GLenum mode,const GLdouble* m);
typedef void (APIENTRY* PFNGLNAMEDBUFFERDATAEXTPROC)(GLuint buffer,GLsizeiptr size,const void* data,GLenum usage);
typedef void (APIENTRY* PFNGLNAMEDBUFFERSUBDATAEXTPROC)(GLuint buffer,GLintptr offset,GLsizeiptr size,const void* data);
typedef void* (APIENTRY* PFNGLMAPNAMEDBUFFEREXTPROC)(GLuint buffer,GLenum access);
typedef GLboolean (APIENTRY* PFNGLUNMAPNAMEDBUFFEREXTPROC)(GLuint buffer);
typedef void (APIENTRY* PFNGLGETNAMEDBUFFERPARAMETERIVEXTPROC)(GLuint buffer,GLenum pname,GLint* params);
typedef void (APIENTRY* PFNGLGETNAMEDBUFFERPOINTERVEXTPROC)(GLuint buffer,GLenum pname,void** params);
typedef void (APIENTRY* PFNGLGETNAMEDBUFFERSUBDATAEXTPROC)(GLuint buffer,GLintptr offset,GLsizeiptr size,void* data);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORM1FEXTPROC)(GLuint program,GLint location,GLfloat v0);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORM2FEXTPROC)(GLuint program,GLint location,GLfloat v0,GLfloat v1);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORM3FEXTPROC)(GLuint program,GLint location,GLfloat v0,GLfloat v1,GLfloat v2);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORM4FEXTPROC)(GLuint program,GLint location,GLfloat v0,GLfloat v1,GLfloat v2,GLfloat v3);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORM1IEXTPROC)(GLuint program,GLint location,GLint v0);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORM2IEXTPROC)(GLuint program,GLint location,GLint v0,GLint v1);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORM3IEXTPROC)(GLuint program,GLint location,GLint v0,GLint v1,GLint v2);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORM4IEXTPROC)(GLuint program,GLint location,GLint v0,GLint v1,GLint v2,GLint v3);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORM1FVEXTPROC)(GLuint program,GLint location,GLsizei count,const GLfloat* value);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORM2FVEXTPROC)(GLuint program,GLint location,GLsizei count,const GLfloat* value);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORM3FVEXTPROC)(GLuint program,GLint location,GLsizei count,const GLfloat* value);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORM4FVEXTPROC)(GLuint program,GLint location,GLsizei count,const GLfloat* value);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORM1IVEXTPROC)(GLuint program,GLint location,GLsizei count,const GLint* value);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORM2IVEXTPROC)(GLuint program,GLint location,GLsizei count,const GLint* value);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORM3IVEXTPROC)(GLuint program,GLint location,GLsizei count,const GLint* value);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORM4IVEXTPROC)(GLuint program,GLint location,GLsizei count,const GLint* value);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORMMATRIX2FVEXTPROC)(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLfloat* value);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORMMATRIX3FVEXTPROC)(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLfloat* value);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORMMATRIX4FVEXTPROC)(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLfloat* value);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORMMATRIX2X3FVEXTPROC)(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLfloat* value);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORMMATRIX3X2FVEXTPROC)(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLfloat* value);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORMMATRIX2X4FVEXTPROC)(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLfloat* value);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORMMATRIX4X2FVEXTPROC)(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLfloat* value);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORMMATRIX3X4FVEXTPROC)(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLfloat* value);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORMMATRIX4X3FVEXTPROC)(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLfloat* value);
typedef void (APIENTRY* PFNGLTEXTUREBUFFEREXTPROC)(GLuint texture,GLenum target,GLenum internalformat,GLuint buffer);
typedef void (APIENTRY* PFNGLMULTITEXBUFFEREXTPROC)(GLenum texunit,GLenum target,GLenum internalformat,GLuint buffer);
typedef void (APIENTRY* PFNGLTEXTUREPARAMETERIIVEXTPROC)(GLuint texture,GLenum target,GLenum pname,const GLint* params);
typedef void (APIENTRY* PFNGLTEXTUREPARAMETERIUIVEXTPROC)(GLuint texture,GLenum target,GLenum pname,const GLuint* params);
typedef void (APIENTRY* PFNGLGETTEXTUREPARAMETERIIVEXTPROC)(GLuint texture,GLenum target,GLenum pname,GLint* params);
typedef void (APIENTRY* PFNGLGETTEXTUREPARAMETERIUIVEXTPROC)(GLuint texture,GLenum target,GLenum pname,GLuint* params);
typedef void (APIENTRY* PFNGLMULTITEXPARAMETERIIVEXTPROC)(GLenum texunit,GLenum target,GLenum pname,const GLint* params);
typedef void (APIENTRY* PFNGLMULTITEXPARAMETERIUIVEXTPROC)(GLenum texunit,GLenum target,GLenum pname,const GLuint* params);
typedef void (APIENTRY* PFNGLGETMULTITEXPARAMETERIIVEXTPROC)(GLenum texunit,GLenum target,GLenum pname,GLint* params);
typedef void (APIENTRY* PFNGLGETMULTITEXPARAMETERIUIVEXTPROC)(GLenum texunit,GLenum target,GLenum pname,GLuint* params);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORM1UIEXTPROC)(GLuint program,GLint location,GLuint v0);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORM2UIEXTPROC)(GLuint program,GLint location,GLuint v0,GLuint v1);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORM3UIEXTPROC)(GLuint program,GLint location,GLuint v0,GLuint v1,GLuint v2);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORM4UIEXTPROC)(GLuint program,GLint location,GLuint v0,GLuint v1,GLuint v2,GLuint v3);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORM1UIVEXTPROC)(GLuint program,GLint location,GLsizei count,const GLuint* value);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORM2UIVEXTPROC)(GLuint program,GLint location,GLsizei count,const GLuint* value);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORM3UIVEXTPROC)(GLuint program,GLint location,GLsizei count,const GLuint* value);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORM4UIVEXTPROC)(GLuint program,GLint location,GLsizei count,const GLuint* value);
typedef void (APIENTRY* PFNGLNAMEDPROGRAMLOCALPARAMETERS4FVEXTPROC)(GLuint program,GLenum target,GLuint index,GLsizei count,const GLfloat* params);
typedef void (APIENTRY* PFNGLNAMEDPROGRAMLOCALPARAMETERI4IEXTPROC)(GLuint program,GLenum target,GLuint index,GLint x,GLint y,GLint z,GLint w);
typedef void (APIENTRY* PFNGLNAMEDPROGRAMLOCALPARAMETERI4IVEXTPROC)(GLuint program,GLenum target,GLuint index,const GLint* params);
typedef void (APIENTRY* PFNGLNAMEDPROGRAMLOCALPARAMETERSI4IVEXTPROC)(GLuint program,GLenum target,GLuint index,GLsizei count,const GLint* params);
typedef void (APIENTRY* PFNGLNAMEDPROGRAMLOCALPARAMETERI4UIEXTPROC)(GLuint program,GLenum target,GLuint index,GLuint x,GLuint y,GLuint z,GLuint w);
typedef void (APIENTRY* PFNGLNAMEDPROGRAMLOCALPARAMETERI4UIVEXTPROC)(GLuint program,GLenum target,GLuint index,const GLuint* params);
typedef void (APIENTRY* PFNGLNAMEDPROGRAMLOCALPARAMETERSI4UIVEXTPROC)(GLuint program,GLenum target,GLuint index,GLsizei count,const GLuint* params);
typedef void (APIENTRY* PFNGLGETNAMEDPROGRAMLOCALPARAMETERIIVEXTPROC)(GLuint program,GLenum target,GLuint index,GLint* params);
typedef void (APIENTRY* PFNGLGETNAMEDPROGRAMLOCALPARAMETERIUIVEXTPROC)(GLuint program,GLenum target,GLuint index,GLuint* params);
typedef void (APIENTRY* PFNGLENABLECLIENTSTATEIEXTPROC)(GLenum array,GLuint index);
typedef void (APIENTRY* PFNGLDISABLECLIENTSTATEIEXTPROC)(GLenum array,GLuint index);
typedef void (APIENTRY* PFNGLGETFLOATI_VEXTPROC)(GLenum pname,GLuint index,GLfloat* params);
typedef void (APIENTRY* PFNGLGETDOUBLEI_VEXTPROC)(GLenum pname,GLuint index,GLdouble* params);
typedef void (APIENTRY* PFNGLGETPOINTERI_VEXTPROC)(GLenum pname,GLuint index,void** params);
typedef void (APIENTRY* PFNGLNAMEDPROGRAMSTRINGEXTPROC)(GLuint program,GLenum target,GLenum format,GLsizei len,const void* string);
typedef void (APIENTRY* PFNGLNAMEDPROGRAMLOCALPARAMETER4DEXTPROC)(GLuint program,GLenum target,GLuint index,GLdouble x,GLdouble y,GLdouble z,GLdouble w);
typedef void (APIENTRY* PFNGLNAMEDPROGRAMLOCALPARAMETER4DVEXTPROC)(GLuint program,GLenum target,GLuint index,const GLdouble* params);
typedef void (APIENTRY* PFNGLNAMEDPROGRAMLOCALPARAMETER4FEXTPROC)(GLuint program,GLenum target,GLuint index,GLfloat x,GLfloat y,GLfloat z,GLfloat w);
typedef void (APIENTRY* PFNGLNAMEDPROGRAMLOCALPARAMETER4FVEXTPROC)(GLuint program,GLenum target,GLuint index,const GLfloat* params);
typedef void (APIENTRY* PFNGLGETNAMEDPROGRAMLOCALPARAMETERDVEXTPROC)(GLuint program,GLenum target,GLuint index,GLdouble* params);
typedef void (APIENTRY* PFNGLGETNAMEDPROGRAMLOCALPARAMETERFVEXTPROC)(GLuint program,GLenum target,GLuint index,GLfloat* params);
typedef void (APIENTRY* PFNGLGETNAMEDPROGRAMIVEXTPROC)(GLuint program,GLenum target,GLenum pname,GLint* params);
typedef void (APIENTRY* PFNGLGETNAMEDPROGRAMSTRINGEXTPROC)(GLuint program,GLenum target,GLenum pname,void* string);
typedef void (APIENTRY* PFNGLNAMEDRENDERBUFFERSTORAGEEXTPROC)(GLuint renderbuffer,GLenum internalformat,GLsizei width,GLsizei height);
typedef void (APIENTRY* PFNGLGETNAMEDRENDERBUFFERPARAMETERIVEXTPROC)(GLuint renderbuffer,GLenum pname,GLint* params);
typedef void (APIENTRY* PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)(GLuint renderbuffer,GLsizei samples,GLenum internalformat,GLsizei width,GLsizei height);
typedef void (APIENTRY* PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLECOVERAGEEXTPROC)(GLuint renderbuffer,GLsizei coverageSamples,GLsizei colorSamples,GLenum internalformat,GLsizei width,GLsizei height);
typedef GLenum (APIENTRY* PFNGLCHECKNAMEDFRAMEBUFFERSTATUSEXTPROC)(GLuint framebuffer,GLenum target);
typedef void (APIENTRY* PFNGLNAMEDFRAMEBUFFERTEXTURE1DEXTPROC)(GLuint framebuffer,GLenum attachment,GLenum textarget,GLuint texture,GLint level);
typedef void (APIENTRY* PFNGLNAMEDFRAMEBUFFERTEXTURE2DEXTPROC)(GLuint framebuffer,GLenum attachment,GLenum textarget,GLuint texture,GLint level);
typedef void (APIENTRY* PFNGLNAMEDFRAMEBUFFERTEXTURE3DEXTPROC)(GLuint framebuffer,GLenum attachment,GLenum textarget,GLuint texture,GLint level,GLint zoffset);
typedef void (APIENTRY* PFNGLNAMEDFRAMEBUFFERRENDERBUFFEREXTPROC)(GLuint framebuffer,GLenum attachment,GLenum renderbuffertarget,GLuint renderbuffer);
typedef void (APIENTRY* PFNGLGETNAMEDFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC)(GLuint framebuffer,GLenum attachment,GLenum pname,GLint* params);
typedef void (APIENTRY* PFNGLGENERATETEXTUREMIPMAPEXTPROC)(GLuint texture,GLenum target);
typedef void (APIENTRY* PFNGLGENERATEMULTITEXMIPMAPEXTPROC)(GLenum texunit,GLenum target);
typedef void (APIENTRY* PFNGLFRAMEBUFFERDRAWBUFFEREXTPROC)(GLuint framebuffer,GLenum mode);
typedef void (APIENTRY* PFNGLFRAMEBUFFERDRAWBUFFERSEXTPROC)(GLuint framebuffer,GLsizei n,const GLenum* bufs);
typedef void (APIENTRY* PFNGLFRAMEBUFFERREADBUFFEREXTPROC)(GLuint framebuffer,GLenum mode);
typedef void (APIENTRY* PFNGLGETFRAMEBUFFERPARAMETERIVEXTPROC)(GLuint framebuffer,GLenum pname,GLint* params);
typedef void (APIENTRY* PFNGLNAMEDCOPYBUFFERSUBDATAEXTPROC)(GLuint readBuffer,GLuint writeBuffer,GLintptr readOffset,GLintptr writeOffset,GLsizeiptr size);
typedef void (APIENTRY* PFNGLNAMEDFRAMEBUFFERTEXTUREEXTPROC)(GLuint framebuffer,GLenum attachment,GLuint texture,GLint level);
typedef void (APIENTRY* PFNGLNAMEDFRAMEBUFFERTEXTURELAYEREXTPROC)(GLuint framebuffer,GLenum attachment,GLuint texture,GLint level,GLint layer);
typedef void (APIENTRY* PFNGLNAMEDFRAMEBUFFERTEXTUREFACEEXTPROC)(GLuint framebuffer,GLenum attachment,GLuint texture,GLint level,GLenum face);
typedef void (APIENTRY* PFNGLTEXTURERENDERBUFFEREXTPROC)(GLuint texture,GLenum target,GLuint renderbuffer);
typedef void (APIENTRY* PFNGLMULTITEXRENDERBUFFEREXTPROC)(GLenum texunit,GLenum target,GLuint renderbuffer);
typedef void (APIENTRY* PFNGLVERTEXARRAYVERTEXOFFSETEXTPROC)(GLuint vaobj,GLuint buffer,GLint size,GLenum type,GLsizei stride,GLintptr offset);
typedef void (APIENTRY* PFNGLVERTEXARRAYCOLOROFFSETEXTPROC)(GLuint vaobj,GLuint buffer,GLint size,GLenum type,GLsizei stride,GLintptr offset);
typedef void (APIENTRY* PFNGLVERTEXARRAYEDGEFLAGOFFSETEXTPROC)(GLuint vaobj,GLuint buffer,GLsizei stride,GLintptr offset);
typedef void (APIENTRY* PFNGLVERTEXARRAYINDEXOFFSETEXTPROC)(GLuint vaobj,GLuint buffer,GLenum type,GLsizei stride,GLintptr offset);
typedef void (APIENTRY* PFNGLVERTEXARRAYNORMALOFFSETEXTPROC)(GLuint vaobj,GLuint buffer,GLenum type,GLsizei stride,GLintptr offset);
typedef void (APIENTRY* PFNGLVERTEXARRAYTEXCOORDOFFSETEXTPROC)(GLuint vaobj,GLuint buffer,GLint size,GLenum type,GLsizei stride,GLintptr offset);
typedef void (APIENTRY* PFNGLVERTEXARRAYMULTITEXCOORDOFFSETEXTPROC)(GLuint vaobj,GLuint buffer,GLenum texunit,GLint size,GLenum type,GLsizei stride,GLintptr offset);
typedef void (APIENTRY* PFNGLVERTEXARRAYFOGCOORDOFFSETEXTPROC)(GLuint vaobj,GLuint buffer,GLenum type,GLsizei stride,GLintptr offset);
typedef void (APIENTRY* PFNGLVERTEXARRAYSECONDARYCOLOROFFSETEXTPROC)(GLuint vaobj,GLuint buffer,GLint size,GLenum type,GLsizei stride,GLintptr offset);
typedef void (APIENTRY* PFNGLVERTEXARRAYVERTEXATTRIBOFFSETEXTPROC)(GLuint vaobj,GLuint buffer,GLuint index,GLint size,GLenum type,GLboolean normalized,GLsizei stride,GLintptr offset);
typedef void (APIENTRY* PFNGLVERTEXARRAYVERTEXATTRIBIOFFSETEXTPROC)(GLuint vaobj,GLuint buffer,GLuint index,GLint size,GLenum type,GLsizei stride,GLintptr offset);
typedef void (APIENTRY* PFNGLENABLEVERTEXARRAYEXTPROC)(GLuint vaobj,GLenum array);
typedef void (APIENTRY* PFNGLDISABLEVERTEXARRAYEXTPROC)(GLuint vaobj,GLenum array);
typedef void (APIENTRY* PFNGLENABLEVERTEXARRAYATTRIBEXTPROC)(GLuint vaobj,GLuint index);
typedef void (APIENTRY* PFNGLDISABLEVERTEXARRAYATTRIBEXTPROC)(GLuint vaobj,GLuint index);
typedef void (APIENTRY* PFNGLGETVERTEXARRAYINTEGERVEXTPROC)(GLuint vaobj,GLenum pname,GLint* param);
typedef void (APIENTRY* PFNGLGETVERTEXARRAYPOINTERVEXTPROC)(GLuint vaobj,GLenum pname,void** param);
typedef void (APIENTRY* PFNGLGETVERTEXARRAYINTEGERI_VEXTPROC)(GLuint vaobj,GLuint index,GLenum pname,GLint* param);
typedef void (APIENTRY* PFNGLGETVERTEXARRAYPOINTERI_VEXTPROC)(GLuint vaobj,GLuint index,GLenum pname,void** param);
typedef void* (APIENTRY* PFNGLMAPNAMEDBUFFERRANGEEXTPROC)(GLuint buffer,GLintptr offset,GLsizeiptr length,GLbitfield access);
typedef void (APIENTRY* PFNGLFLUSHMAPPEDNAMEDBUFFERRANGEEXTPROC)(GLuint buffer,GLintptr offset,GLsizeiptr length);
typedef void (APIENTRY* PFNGLNAMEDBUFFERSTORAGEEXTPROC)(GLuint buffer,GLsizeiptr size,const void* data,GLbitfield flags);
typedef void (APIENTRY* PFNGLCLEARNAMEDBUFFERDATAEXTPROC)(GLuint buffer,GLenum internalformat,GLenum format,GLenum type,const void* data);
typedef void (APIENTRY* PFNGLCLEARNAMEDBUFFERSUBDATAEXTPROC)(GLuint buffer,GLenum internalformat,GLsizeiptr offset,GLsizeiptr size,GLenum format,GLenum type,const void* data);
typedef void (APIENTRY* PFNGLNAMEDFRAMEBUFFERPARAMETERIEXTPROC)(GLuint framebuffer,GLenum pname,GLint param);
typedef void (APIENTRY* PFNGLGETNAMEDFRAMEBUFFERPARAMETERIVEXTPROC)(GLuint framebuffer,GLenum pname,GLint* params);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORM1DEXTPROC)(GLuint program,GLint location,GLdouble x);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORM2DEXTPROC)(GLuint program,GLint location,GLdouble x,GLdouble y);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORM3DEXTPROC)(GLuint program,GLint location,GLdouble x,GLdouble y,GLdouble z);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORM4DEXTPROC)(GLuint program,GLint location,GLdouble x,GLdouble y,GLdouble z,GLdouble w);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORM1DVEXTPROC)(GLuint program,GLint location,GLsizei count,const GLdouble* value);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORM2DVEXTPROC)(GLuint program,GLint location,GLsizei count,const GLdouble* value);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORM3DVEXTPROC)(GLuint program,GLint location,GLsizei count,const GLdouble* value);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORM4DVEXTPROC)(GLuint program,GLint location,GLsizei count,const GLdouble* value);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORMMATRIX2DVEXTPROC)(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLdouble* value);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORMMATRIX3DVEXTPROC)(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLdouble* value);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORMMATRIX4DVEXTPROC)(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLdouble* value);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORMMATRIX2X3DVEXTPROC)(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLdouble* value);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORMMATRIX2X4DVEXTPROC)(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLdouble* value);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORMMATRIX3X2DVEXTPROC)(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLdouble* value);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORMMATRIX3X4DVEXTPROC)(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLdouble* value);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORMMATRIX4X2DVEXTPROC)(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLdouble* value);
typedef void (APIENTRY* PFNGLPROGRAMUNIFORMMATRIX4X3DVEXTPROC)(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLdouble* value);
typedef void (APIENTRY* PFNGLTEXTUREBUFFERRANGEEXTPROC)(GLuint texture,GLenum target,GLenum internalformat,GLuint buffer,GLintptr offset,GLsizeiptr size);
typedef void (APIENTRY* PFNGLTEXTURESTORAGE1DEXTPROC)(GLuint texture,GLenum target,GLsizei levels,GLenum internalformat,GLsizei width);
typedef void (APIENTRY* PFNGLTEXTURESTORAGE2DEXTPROC)(GLuint texture,GLenum target,GLsizei levels,GLenum internalformat,GLsizei width,GLsizei height);
typedef void (APIENTRY* PFNGLTEXTURESTORAGE3DEXTPROC)(GLuint texture,GLenum target,GLsizei levels,GLenum internalformat,GLsizei width,GLsizei height,GLsizei depth);
typedef void (APIENTRY* PFNGLTEXTURESTORAGE2DMULTISAMPLEEXTPROC)(GLuint texture,GLenum target,GLsizei samples,GLenum internalformat,GLsizei width,GLsizei height,GLboolean fixedsamplelocations);
typedef void (APIENTRY* PFNGLTEXTURESTORAGE3DMULTISAMPLEEXTPROC)(GLuint texture,GLenum target,GLsizei samples,GLenum internalformat,GLsizei width,GLsizei height,GLsizei depth,GLboolean fixedsamplelocations);
typedef void (APIENTRY* PFNGLVERTEXARRAYBINDVERTEXBUFFEREXTPROC)(GLuint vaobj,GLuint bindingindex,GLuint buffer,GLintptr offset,GLsizei stride);
typedef void (APIENTRY* PFNGLVERTEXARRAYVERTEXATTRIBFORMATEXTPROC)(GLuint vaobj,GLuint attribindex,GLint size,GLenum type,GLboolean normalized,GLuint relativeoffset);
typedef void (APIENTRY* PFNGLVERTEXARRAYVERTEXATTRIBIFORMATEXTPROC)(GLuint vaobj,GLuint attribindex,GLint size,GLenum type,GLuint relativeoffset);
typedef void (APIENTRY* PFNGLVERTEXARRAYVERTEXATTRIBLFORMATEXTPROC)(GLuint vaobj,GLuint attribindex,GLint size,GLenum type,GLuint relativeoffset);
typedef void (APIENTRY* PFNGLVERTEXARRAYVERTEXATTRIBBINDINGEXTPROC)(GLuint vaobj,GLuint attribindex,GLuint bindingindex);
typedef void (APIENTRY* PFNGLVERTEXARRAYVERTEXBINDINGDIVISOREXTPROC)(GLuint vaobj,GLuint bindingindex,GLuint divisor);
typedef void (APIENTRY* PFNGLVERTEXARRAYVERTEXATTRIBLOFFSETEXTPROC)(GLuint vaobj,GLuint buffer,GLuint index,GLint size,GLenum type,GLsizei stride,GLintptr offset);
typedef void (APIENTRY* PFNGLTEXTUREPAGECOMMITMENTEXTPROC)(GLuint texture,GLint level,GLint xoffset,GLint yoffset,GLint zoffset,GLsizei width,GLsizei height,GLsizei depth,GLboolean commit);
typedef void (APIENTRY* PFNGLVERTEXARRAYVERTEXATTRIBDIVISOREXTPROC)(GLuint vaobj,GLuint index,GLuint divisor);

/* Extension-specific constants: */
#define GL_PROGRAM_MATRIX_EXT             0x8E2D
#define GL_TRANSPOSE_PROGRAM_MATRIX_EXT   0x8E2E
#define GL_PROGRAM_MATRIX_STACK_DEPTH_EXT 0x8E2F

#endif

/* Forward declarations of friend functions: */
void glMatrixLoadfEXT(GLenum mode,const GLfloat* m);
void glMatrixLoaddEXT(GLenum mode,const GLdouble* m);
void glMatrixMultfEXT(GLenum mode,const GLfloat* m);
void glMatrixMultdEXT(GLenum mode,const GLdouble* m);
void glMatrixLoadIdentityEXT(GLenum mode);
void glMatrixRotatefEXT(GLenum mode,GLfloat angle,GLfloat x,GLfloat y,GLfloat z);
void glMatrixRotatedEXT(GLenum mode,GLdouble angle,GLdouble x,GLdouble y,GLdouble z);
void glMatrixScalefEXT(GLenum mode,GLfloat x,GLfloat y,GLfloat z);
void glMatrixScaledEXT(GLenum mode,GLdouble x,GLdouble y,GLdouble z);
void glMatrixTranslatefEXT(GLenum mode,GLfloat x,GLfloat y,GLfloat z);
void glMatrixTranslatedEXT(GLenum mode,GLdouble x,GLdouble y,GLdouble z);
void glMatrixFrustumEXT(GLenum mode,GLdouble left,GLdouble right,GLdouble bottom,GLdouble top,GLdouble zNear,GLdouble zFar);
void glMatrixOrthoEXT(GLenum mode,GLdouble left,GLdouble right,GLdouble bottom,GLdouble top,GLdouble zNear,GLdouble zFar);
void glMatrixPopEXT(GLenum mode);
void glMatrixPushEXT(GLenum mode);
void glClientAttribDefaultEXT(GLbitfield mask);
void glPushClientAttribDefaultEXT(GLbitfield mask);
void glTextureParameterfEXT(GLuint texture,GLenum target,GLenum pname,GLfloat param);
void glTextureParameterfvEXT(GLuint texture,GLenum target,GLenum pname,const GLfloat* params);
void glTextureParameteriEXT(GLuint texture,GLenum target,GLenum pname,GLint param);
void glTextureParameterivEXT(GLuint texture,GLenum target,GLenum pname,const GLint* params);
void glTextureImage1DEXT(GLuint texture,GLenum target,GLint level,GLint internalformat,GLsizei width,GLint border,GLenum format,GLenum type,const void* pixels);
void glTextureImage2DEXT(GLuint texture,GLenum target,GLint level,GLint internalformat,GLsizei width,GLsizei height,GLint border,GLenum format,GLenum type,const void* pixels);
void glTextureSubImage1DEXT(GLuint texture,GLenum target,GLint level,GLint xoffset,GLsizei width,GLenum format,GLenum type,const void* pixels);
void glTextureSubImage2DEXT(GLuint texture,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLsizei width,GLsizei height,GLenum format,GLenum type,const void* pixels);
void glCopyTextureImage1DEXT(GLuint texture,GLenum target,GLint level,GLenum internalformat,GLint x,GLint y,GLsizei width,GLint border);
void glCopyTextureImage2DEXT(GLuint texture,GLenum target,GLint level,GLenum internalformat,GLint x,GLint y,GLsizei width,GLsizei height,GLint border);
void glCopyTextureSubImage1DEXT(GLuint texture,GLenum target,GLint level,GLint xoffset,GLint x,GLint y,GLsizei width);
void glCopyTextureSubImage2DEXT(GLuint texture,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLint x,GLint y,GLsizei width,GLsizei height);
void glGetTextureImageEXT(GLuint texture,GLenum target,GLint level,GLenum format,GLenum type,void* pixels);
void glGetTextureParameterfvEXT(GLuint texture,GLenum target,GLenum pname,GLfloat* params);
void glGetTextureParameterivEXT(GLuint texture,GLenum target,GLenum pname,GLint* params);
void glGetTextureLevelParameterfvEXT(GLuint texture,GLenum target,GLint level,GLenum pname,GLfloat* params);
void glGetTextureLevelParameterivEXT(GLuint texture,GLenum target,GLint level,GLenum pname,GLint* params);
void glTextureImage3DEXT(GLuint texture,GLenum target,GLint level,GLint internalformat,GLsizei width,GLsizei height,GLsizei depth,GLint border,GLenum format,GLenum type,const void* pixels);
void glTextureSubImage3DEXT(GLuint texture,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLint zoffset,GLsizei width,GLsizei height,GLsizei depth,GLenum format,GLenum type,const void* pixels);
void glCopyTextureSubImage3DEXT(GLuint texture,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLint zoffset,GLint x,GLint y,GLsizei width,GLsizei height);
void glBindMultiTextureEXT(GLenum texunit,GLenum target,GLuint texture);
void glMultiTexCoordPointerEXT(GLenum texunit,GLint size,GLenum type,GLsizei stride,const void* pointer);
void glMultiTexEnvfEXT(GLenum texunit,GLenum target,GLenum pname,GLfloat param);
void glMultiTexEnvfvEXT(GLenum texunit,GLenum target,GLenum pname,const GLfloat* params);
void glMultiTexEnviEXT(GLenum texunit,GLenum target,GLenum pname,GLint param);
void glMultiTexEnvivEXT(GLenum texunit,GLenum target,GLenum pname,const GLint* params);
void glMultiTexGendEXT(GLenum texunit,GLenum coord,GLenum pname,GLdouble param);
void glMultiTexGendvEXT(GLenum texunit,GLenum coord,GLenum pname,const GLdouble* params);
void glMultiTexGenfEXT(GLenum texunit,GLenum coord,GLenum pname,GLfloat param);
void glMultiTexGenfvEXT(GLenum texunit,GLenum coord,GLenum pname,const GLfloat* params);
void glMultiTexGeniEXT(GLenum texunit,GLenum coord,GLenum pname,GLint param);
void glMultiTexGenivEXT(GLenum texunit,GLenum coord,GLenum pname,const GLint* params);
void glGetMultiTexEnvfvEXT(GLenum texunit,GLenum target,GLenum pname,GLfloat* params);
void glGetMultiTexEnvivEXT(GLenum texunit,GLenum target,GLenum pname,GLint* params);
void glGetMultiTexGendvEXT(GLenum texunit,GLenum coord,GLenum pname,GLdouble* params);
void glGetMultiTexGenfvEXT(GLenum texunit,GLenum coord,GLenum pname,GLfloat* params);
void glGetMultiTexGenivEXT(GLenum texunit,GLenum coord,GLenum pname,GLint* params);
void glMultiTexParameteriEXT(GLenum texunit,GLenum target,GLenum pname,GLint param);
void glMultiTexParameterivEXT(GLenum texunit,GLenum target,GLenum pname,const GLint* params);
void glMultiTexParameterfEXT(GLenum texunit,GLenum target,GLenum pname,GLfloat param);
void glMultiTexParameterfvEXT(GLenum texunit,GLenum target,GLenum pname,const GLfloat* params);
void glMultiTexImage1DEXT(GLenum texunit,GLenum target,GLint level,GLint internalformat,GLsizei width,GLint border,GLenum format,GLenum type,const void* pixels);
void glMultiTexImage2DEXT(GLenum texunit,GLenum target,GLint level,GLint internalformat,GLsizei width,GLsizei height,GLint border,GLenum format,GLenum type,const void* pixels);
void glMultiTexSubImage1DEXT(GLenum texunit,GLenum target,GLint level,GLint xoffset,GLsizei width,GLenum format,GLenum type,const void* pixels);
void glMultiTexSubImage2DEXT(GLenum texunit,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLsizei width,GLsizei height,GLenum format,GLenum type,const void* pixels);
void glCopyMultiTexImage1DEXT(GLenum texunit,GLenum target,GLint level,GLenum internalformat,GLint x,GLint y,GLsizei width,GLint border);
void glCopyMultiTexImage2DEXT(GLenum texunit,GLenum target,GLint level,GLenum internalformat,GLint x,GLint y,GLsizei width,GLsizei height,GLint border);
void glCopyMultiTexSubImage1DEXT(GLenum texunit,GLenum target,GLint level,GLint xoffset,GLint x,GLint y,GLsizei width);
void glCopyMultiTexSubImage2DEXT(GLenum texunit,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLint x,GLint y,GLsizei width,GLsizei height);
void glGetMultiTexImageEXT(GLenum texunit,GLenum target,GLint level,GLenum format,GLenum type,void* pixels);
void glGetMultiTexParameterfvEXT(GLenum texunit,GLenum target,GLenum pname,GLfloat* params);
void glGetMultiTexParameterivEXT(GLenum texunit,GLenum target,GLenum pname,GLint* params);
void glGetMultiTexLevelParameterfvEXT(GLenum texunit,GLenum target,GLint level,GLenum pname,GLfloat* params);
void glGetMultiTexLevelParameterivEXT(GLenum texunit,GLenum target,GLint level,GLenum pname,GLint* params);
void glMultiTexImage3DEXT(GLenum texunit,GLenum target,GLint level,GLint internalformat,GLsizei width,GLsizei height,GLsizei depth,GLint border,GLenum format,GLenum type,const void* pixels);
void glMultiTexSubImage3DEXT(GLenum texunit,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLint zoffset,GLsizei width,GLsizei height,GLsizei depth,GLenum format,GLenum type,const void* pixels);
void glCopyMultiTexSubImage3DEXT(GLenum texunit,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLint zoffset,GLint x,GLint y,GLsizei width,GLsizei height);
void glEnableClientStateIndexedEXT(GLenum array,GLuint index);
void glDisableClientStateIndexedEXT(GLenum array,GLuint index);
void glGetFloatIndexedvEXT(GLenum target,GLuint index,GLfloat* data);
void glGetDoubleIndexedvEXT(GLenum target,GLuint index,GLdouble* data);
void glGetPointerIndexedvEXT(GLenum target,GLuint index,void** data);
void glEnableIndexedEXT(GLenum target,GLuint index);
void glDisableIndexedEXT(GLenum target,GLuint index);
GLboolean glIsEnabledIndexedEXT(GLenum target,GLuint index);
void glGetIntegerIndexedvEXT(GLenum target,GLuint index,GLint* data);
void glGetBooleanIndexedvEXT(GLenum target,GLuint index,GLboolean* data);
void glCompressedTextureImage3DEXT(GLuint texture,GLenum target,GLint level,GLenum internalformat,GLsizei width,GLsizei height,GLsizei depth,GLint border,GLsizei imageSize,const void* bits);
void glCompressedTextureImage2DEXT(GLuint texture,GLenum target,GLint level,GLenum internalformat,GLsizei width,GLsizei height,GLint border,GLsizei imageSize,const void* bits);
void glCompressedTextureImage1DEXT(GLuint texture,GLenum target,GLint level,GLenum internalformat,GLsizei width,GLint border,GLsizei imageSize,const void* bits);
void glCompressedTextureSubImage3DEXT(GLuint texture,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLint zoffset,GLsizei width,GLsizei height,GLsizei depth,GLenum format,GLsizei imageSize,const void* bits);
void glCompressedTextureSubImage2DEXT(GLuint texture,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLsizei width,GLsizei height,GLenum format,GLsizei imageSize,const void* bits);
void glCompressedTextureSubImage1DEXT(GLuint texture,GLenum target,GLint level,GLint xoffset,GLsizei width,GLenum format,GLsizei imageSize,const void* bits);
void glGetCompressedTextureImageEXT(GLuint texture,GLenum target,GLint lod,void* img);
void glCompressedMultiTexImage3DEXT(GLenum texunit,GLenum target,GLint level,GLenum internalformat,GLsizei width,GLsizei height,GLsizei depth,GLint border,GLsizei imageSize,const void* bits);
void glCompressedMultiTexImage2DEXT(GLenum texunit,GLenum target,GLint level,GLenum internalformat,GLsizei width,GLsizei height,GLint border,GLsizei imageSize,const void* bits);
void glCompressedMultiTexImage1DEXT(GLenum texunit,GLenum target,GLint level,GLenum internalformat,GLsizei width,GLint border,GLsizei imageSize,const void* bits);
void glCompressedMultiTexSubImage3DEXT(GLenum texunit,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLint zoffset,GLsizei width,GLsizei height,GLsizei depth,GLenum format,GLsizei imageSize,const void* bits);
void glCompressedMultiTexSubImage2DEXT(GLenum texunit,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLsizei width,GLsizei height,GLenum format,GLsizei imageSize,const void* bits);
void glCompressedMultiTexSubImage1DEXT(GLenum texunit,GLenum target,GLint level,GLint xoffset,GLsizei width,GLenum format,GLsizei imageSize,const void* bits);
void glGetCompressedMultiTexImageEXT(GLenum texunit,GLenum target,GLint lod,void* img);
void glMatrixLoadTransposefEXT(GLenum mode,const GLfloat* m);
void glMatrixLoadTransposedEXT(GLenum mode,const GLdouble* m);
void glMatrixMultTransposefEXT(GLenum mode,const GLfloat* m);
void glMatrixMultTransposedEXT(GLenum mode,const GLdouble* m);
void glNamedBufferDataEXT(GLuint buffer,GLsizeiptr size,const void* data,GLenum usage);
void glNamedBufferSubDataEXT(GLuint buffer,GLintptr offset,GLsizeiptr size,const void* data);
void* glMapNamedBufferEXT(GLuint buffer,GLenum access);
GLboolean glUnmapNamedBufferEXT(GLuint buffer);
void glGetNamedBufferParameterivEXT(GLuint buffer,GLenum pname,GLint* params);
void glGetNamedBufferPointervEXT(GLuint buffer,GLenum pname,void** params);
void glGetNamedBufferSubDataEXT(GLuint buffer,GLintptr offset,GLsizeiptr size,void* data);
void glProgramUniform1fEXT(GLuint program,GLint location,GLfloat v0);
void glProgramUniform2fEXT(GLuint program,GLint location,GLfloat v0,GLfloat v1);
void glProgramUniform3fEXT(GLuint program,GLint location,GLfloat v0,GLfloat v1,GLfloat v2);
void glProgramUniform4fEXT(GLuint program,GLint location,GLfloat v0,GLfloat v1,GLfloat v2,GLfloat v3);
void glProgramUniform1iEXT(GLuint program,GLint location,GLint v0);
void glProgramUniform2iEXT(GLuint program,GLint location,GLint v0,GLint v1);
void glProgramUniform3iEXT(GLuint program,GLint location,GLint v0,GLint v1,GLint v2);
void glProgramUniform4iEXT(GLuint program,GLint location,GLint v0,GLint v1,GLint v2,GLint v3);
void glProgramUniform1fvEXT(GLuint program,GLint location,GLsizei count,const GLfloat* value);
void glProgramUniform2fvEXT(GLuint program,GLint location,GLsizei count,const GLfloat* value);
void glProgramUniform3fvEXT(GLuint program,GLint location,GLsizei count,const GLfloat* value);
void glProgramUniform4fvEXT(GLuint program,GLint location,GLsizei count,const GLfloat* value);
void glProgramUniform1ivEXT(GLuint program,GLint location,GLsizei count,const GLint* value);
void glProgramUniform2ivEXT(GLuint program,GLint location,GLsizei count,const GLint* value);
void glProgramUniform3ivEXT(GLuint program,GLint location,GLsizei count,const GLint* value);
void glProgramUniform4ivEXT(GLuint program,GLint location,GLsizei count,const GLint* value);
void glProgramUniformMatrix2fvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLfloat* value);
void glProgramUniformMatrix3fvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLfloat* value);
void glProgramUniformMatrix4fvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLfloat* value);
void glProgramUniformMatrix2x3fvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLfloat* value);
void glProgramUniformMatrix3x2fvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLfloat* value);
void glProgramUniformMatrix2x4fvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLfloat* value);
void glProgramUniformMatrix4x2fvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLfloat* value);
void glProgramUniformMatrix3x4fvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLfloat* value);
void glProgramUniformMatrix4x3fvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLfloat* value);
void glTextureBufferEXT(GLuint texture,GLenum target,GLenum internalformat,GLuint buffer);
void glMultiTexBufferEXT(GLenum texunit,GLenum target,GLenum internalformat,GLuint buffer);
void glTextureParameterIivEXT(GLuint texture,GLenum target,GLenum pname,const GLint* params);
void glTextureParameterIuivEXT(GLuint texture,GLenum target,GLenum pname,const GLuint* params);
void glGetTextureParameterIivEXT(GLuint texture,GLenum target,GLenum pname,GLint* params);
void glGetTextureParameterIuivEXT(GLuint texture,GLenum target,GLenum pname,GLuint* params);
void glMultiTexParameterIivEXT(GLenum texunit,GLenum target,GLenum pname,const GLint* params);
void glMultiTexParameterIuivEXT(GLenum texunit,GLenum target,GLenum pname,const GLuint* params);
void glGetMultiTexParameterIivEXT(GLenum texunit,GLenum target,GLenum pname,GLint* params);
void glGetMultiTexParameterIuivEXT(GLenum texunit,GLenum target,GLenum pname,GLuint* params);
void glProgramUniform1uiEXT(GLuint program,GLint location,GLuint v0);
void glProgramUniform2uiEXT(GLuint program,GLint location,GLuint v0,GLuint v1);
void glProgramUniform3uiEXT(GLuint program,GLint location,GLuint v0,GLuint v1,GLuint v2);
void glProgramUniform4uiEXT(GLuint program,GLint location,GLuint v0,GLuint v1,GLuint v2,GLuint v3);
void glProgramUniform1uivEXT(GLuint program,GLint location,GLsizei count,const GLuint* value);
void glProgramUniform2uivEXT(GLuint program,GLint location,GLsizei count,const GLuint* value);
void glProgramUniform3uivEXT(GLuint program,GLint location,GLsizei count,const GLuint* value);
void glProgramUniform4uivEXT(GLuint program,GLint location,GLsizei count,const GLuint* value);
void glNamedProgramLocalParameters4fvEXT(GLuint program,GLenum target,GLuint index,GLsizei count,const GLfloat* params);
void glNamedProgramLocalParameterI4iEXT(GLuint program,GLenum target,GLuint index,GLint x,GLint y,GLint z,GLint w);
void glNamedProgramLocalParameterI4ivEXT(GLuint program,GLenum target,GLuint index,const GLint* params);
void glNamedProgramLocalParametersI4ivEXT(GLuint program,GLenum target,GLuint index,GLsizei count,const GLint* params);
void glNamedProgramLocalParameterI4uiEXT(GLuint program,GLenum target,GLuint index,GLuint x,GLuint y,GLuint z,GLuint w);
void glNamedProgramLocalParameterI4uivEXT(GLuint program,GLenum target,GLuint index,const GLuint* params);
void glNamedProgramLocalParametersI4uivEXT(GLuint program,GLenum target,GLuint index,GLsizei count,const GLuint* params);
void glGetNamedProgramLocalParameterIivEXT(GLuint program,GLenum target,GLuint index,GLint* params);
void glGetNamedProgramLocalParameterIuivEXT(GLuint program,GLenum target,GLuint index,GLuint* params);
void glEnableClientStateiEXT(GLenum array,GLuint index);
void glDisableClientStateiEXT(GLenum array,GLuint index);
void glGetFloati_vEXT(GLenum pname,GLuint index,GLfloat* params);
void glGetDoublei_vEXT(GLenum pname,GLuint index,GLdouble* params);
void glGetPointeri_vEXT(GLenum pname,GLuint index,void** params);
void glNamedProgramStringEXT(GLuint program,GLenum target,GLenum format,GLsizei len,const void* string);
void glNamedProgramLocalParameter4dEXT(GLuint program,GLenum target,GLuint index,GLdouble x,GLdouble y,GLdouble z,GLdouble w);
void glNamedProgramLocalParameter4dvEXT(GLuint program,GLenum target,GLuint index,const GLdouble* params);
void glNamedProgramLocalParameter4fEXT(GLuint program,GLenum target,GLuint index,GLfloat x,GLfloat y,GLfloat z,GLfloat w);
void glNamedProgramLocalParameter4fvEXT(GLuint program,GLenum target,GLuint index,const GLfloat* params);
void glGetNamedProgramLocalParameterdvEXT(GLuint program,GLenum target,GLuint index,GLdouble* params);
void glGetNamedProgramLocalParameterfvEXT(GLuint program,GLenum target,GLuint index,GLfloat* params);
void glGetNamedProgramivEXT(GLuint program,GLenum target,GLenum pname,GLint* params);
void glGetNamedProgramStringEXT(GLuint program,GLenum target,GLenum pname,void* string);
void glNamedRenderbufferStorageEXT(GLuint renderbuffer,GLenum internalformat,GLsizei width,GLsizei height);
void glGetNamedRenderbufferParameterivEXT(GLuint renderbuffer,GLenum pname,GLint* params);
void glNamedRenderbufferStorageMultisampleEXT(GLuint renderbuffer,GLsizei samples,GLenum internalformat,GLsizei width,GLsizei height);
void glNamedRenderbufferStorageMultisampleCoverageEXT(GLuint renderbuffer,GLsizei coverageSamples,GLsizei colorSamples,GLenum internalformat,GLsizei width,GLsizei height);
GLenum glCheckNamedFramebufferStatusEXT(GLuint framebuffer,GLenum target);
void glNamedFramebufferTexture1DEXT(GLuint framebuffer,GLenum attachment,GLenum textarget,GLuint texture,GLint level);
void glNamedFramebufferTexture2DEXT(GLuint framebuffer,GLenum attachment,GLenum textarget,GLuint texture,GLint level);
void glNamedFramebufferTexture3DEXT(GLuint framebuffer,GLenum attachment,GLenum textarget,GLuint texture,GLint level,GLint zoffset);
void glNamedFramebufferRenderbufferEXT(GLuint framebuffer,GLenum attachment,GLenum renderbuffertarget,GLuint renderbuffer);
void glGetNamedFramebufferAttachmentParameterivEXT(GLuint framebuffer,GLenum attachment,GLenum pname,GLint* params);
void glGenerateTextureMipmapEXT(GLuint texture,GLenum target);
void glGenerateMultiTexMipmapEXT(GLenum texunit,GLenum target);
void glFramebufferDrawBufferEXT(GLuint framebuffer,GLenum mode);
void glFramebufferDrawBuffersEXT(GLuint framebuffer,GLsizei n,const GLenum* bufs);
void glFramebufferReadBufferEXT(GLuint framebuffer,GLenum mode);
void glGetFramebufferParameterivEXT(GLuint framebuffer,GLenum pname,GLint* params);
void glNamedCopyBufferSubDataEXT(GLuint readBuffer,GLuint writeBuffer,GLintptr readOffset,GLintptr writeOffset,GLsizeiptr size);
void glNamedFramebufferTextureEXT(GLuint framebuffer,GLenum attachment,GLuint texture,GLint level);
void glNamedFramebufferTextureLayerEXT(GLuint framebuffer,GLenum attachment,GLuint texture,GLint level,GLint layer);
void glNamedFramebufferTextureFaceEXT(GLuint framebuffer,GLenum attachment,GLuint texture,GLint level,GLenum face);
void glTextureRenderbufferEXT(GLuint texture,GLenum target,GLuint renderbuffer);
void glMultiTexRenderbufferEXT(GLenum texunit,GLenum target,GLuint renderbuffer);
void glVertexArrayVertexOffsetEXT(GLuint vaobj,GLuint buffer,GLint size,GLenum type,GLsizei stride,GLintptr offset);
void glVertexArrayColorOffsetEXT(GLuint vaobj,GLuint buffer,GLint size,GLenum type,GLsizei stride,GLintptr offset);
void glVertexArrayEdgeFlagOffsetEXT(GLuint vaobj,GLuint buffer,GLsizei stride,GLintptr offset);
void glVertexArrayIndexOffsetEXT(GLuint vaobj,GLuint buffer,GLenum type,GLsizei stride,GLintptr offset);
void glVertexArrayNormalOffsetEXT(GLuint vaobj,GLuint buffer,GLenum type,GLsizei stride,GLintptr offset);
void glVertexArrayTexCoordOffsetEXT(GLuint vaobj,GLuint buffer,GLint size,GLenum type,GLsizei stride,GLintptr offset);
void glVertexArrayMultiTexCoordOffsetEXT(GLuint vaobj,GLuint buffer,GLenum texunit,GLint size,GLenum type,GLsizei stride,GLintptr offset);
void glVertexArrayFogCoordOffsetEXT(GLuint vaobj,GLuint buffer,GLenum type,GLsizei stride,GLintptr offset);
void glVertexArraySecondaryColorOffsetEXT(GLuint vaobj,GLuint buffer,GLint size,GLenum type,GLsizei stride,GLintptr offset);
void glVertexArrayVertexAttribOffsetEXT(GLuint vaobj,GLuint buffer,GLuint index,GLint size,GLenum type,GLboolean normalized,GLsizei stride,GLintptr offset);
void glVertexArrayVertexAttribIOffsetEXT(GLuint vaobj,GLuint buffer,GLuint index,GLint size,GLenum type,GLsizei stride,GLintptr offset);
void glEnableVertexArrayEXT(GLuint vaobj,GLenum array);
void glDisableVertexArrayEXT(GLuint vaobj,GLenum array);
void glEnableVertexArrayAttribEXT(GLuint vaobj,GLuint index);
void glDisableVertexArrayAttribEXT(GLuint vaobj,GLuint index);
void glGetVertexArrayIntegervEXT(GLuint vaobj,GLenum pname,GLint* param);
void glGetVertexArrayPointervEXT(GLuint vaobj,GLenum pname,void** param);
void glGetVertexArrayIntegeri_vEXT(GLuint vaobj,GLuint index,GLenum pname,GLint* param);
void glGetVertexArrayPointeri_vEXT(GLuint vaobj,GLuint index,GLenum pname,void** param);
void* glMapNamedBufferRangeEXT(GLuint buffer,GLintptr offset,GLsizeiptr length,GLbitfield access);
void glFlushMappedNamedBufferRangeEXT(GLuint buffer,GLintptr offset,GLsizeiptr length);
void glNamedBufferStorageEXT(GLuint buffer,GLsizeiptr size,const void* data,GLbitfield flags);
void glClearNamedBufferDataEXT(GLuint buffer,GLenum internalformat,GLenum format,GLenum type,const void* data);
void glClearNamedBufferSubDataEXT(GLuint buffer,GLenum internalformat,GLsizeiptr offset,GLsizeiptr size,GLenum format,GLenum type,const void* data);
void glNamedFramebufferParameteriEXT(GLuint framebuffer,GLenum pname,GLint param);
void glGetNamedFramebufferParameterivEXT(GLuint framebuffer,GLenum pname,GLint* params);
void glProgramUniform1dEXT(GLuint program,GLint location,GLdouble x);
void glProgramUniform2dEXT(GLuint program,GLint location,GLdouble x,GLdouble y);
void glProgramUniform3dEXT(GLuint program,GLint location,GLdouble x,GLdouble y,GLdouble z);
void glProgramUniform4dEXT(GLuint program,GLint location,GLdouble x,GLdouble y,GLdouble z,GLdouble w);
void glProgramUniform1dvEXT(GLuint program,GLint location,GLsizei count,const GLdouble* value);
void glProgramUniform2dvEXT(GLuint program,GLint location,GLsizei count,const GLdouble* value);
void glProgramUniform3dvEXT(GLuint program,GLint location,GLsizei count,const GLdouble* value);
void glProgramUniform4dvEXT(GLuint program,GLint location,GLsizei count,const GLdouble* value);
void glProgramUniformMatrix2dvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLdouble* value);
void glProgramUniformMatrix3dvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLdouble* value);
void glProgramUniformMatrix4dvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLdouble* value);
void glProgramUniformMatrix2x3dvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLdouble* value);
void glProgramUniformMatrix2x4dvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLdouble* value);
void glProgramUniformMatrix3x2dvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLdouble* value);
void glProgramUniformMatrix3x4dvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLdouble* value);
void glProgramUniformMatrix4x2dvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLdouble* value);
void glProgramUniformMatrix4x3dvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLdouble* value);
void glTextureBufferRangeEXT(GLuint texture,GLenum target,GLenum internalformat,GLuint buffer,GLintptr offset,GLsizeiptr size);
void glTextureStorage1DEXT(GLuint texture,GLenum target,GLsizei levels,GLenum internalformat,GLsizei width);
void glTextureStorage2DEXT(GLuint texture,GLenum target,GLsizei levels,GLenum internalformat,GLsizei width,GLsizei height);
void glTextureStorage3DEXT(GLuint texture,GLenum target,GLsizei levels,GLenum internalformat,GLsizei width,GLsizei height,GLsizei depth);
void glTextureStorage2DMultisampleEXT(GLuint texture,GLenum target,GLsizei samples,GLenum internalformat,GLsizei width,GLsizei height,GLboolean fixedsamplelocations);
void glTextureStorage3DMultisampleEXT(GLuint texture,GLenum target,GLsizei samples,GLenum internalformat,GLsizei width,GLsizei height,GLsizei depth,GLboolean fixedsamplelocations);
void glVertexArrayBindVertexBufferEXT(GLuint vaobj,GLuint bindingindex,GLuint buffer,GLintptr offset,GLsizei stride);
void glVertexArrayVertexAttribFormatEXT(GLuint vaobj,GLuint attribindex,GLint size,GLenum type,GLboolean normalized,GLuint relativeoffset);
void glVertexArrayVertexAttribIFormatEXT(GLuint vaobj,GLuint attribindex,GLint size,GLenum type,GLuint relativeoffset);
void glVertexArrayVertexAttribLFormatEXT(GLuint vaobj,GLuint attribindex,GLint size,GLenum type,GLuint relativeoffset);
void glVertexArrayVertexAttribBindingEXT(GLuint vaobj,GLuint attribindex,GLuint bindingindex);
void glVertexArrayVertexBindingDivisorEXT(GLuint vaobj,GLuint bindingindex,GLuint divisor);
void glVertexArrayVertexAttribLOffsetEXT(GLuint vaobj,GLuint buffer,GLuint index,GLint size,GLenum type,GLsizei stride,GLintptr offset);
void glTexturePageCommitmentEXT(GLuint texture,GLint level,GLint xoffset,GLint yoffset,GLint zoffset,GLsizei width,GLsizei height,GLsizei depth,GLboolean commit);
void glVertexArrayVertexAttribDivisorEXT(GLuint vaobj,GLuint index,GLuint divisor);

class GLEXTDirectStateAccess:public GLExtension
	{
	/* Elements: */
	private:
	static GL_THREAD_LOCAL(GLEXTDirectStateAccess*) current; // Pointer to extension object for current OpenGL context
	static const char* name; // Extension name
	PFNGLMATRIXLOADFEXTPROC glMatrixLoadfEXTProc;
	PFNGLMATRIXLOADDEXTPROC glMatrixLoaddEXTProc;
	PFNGLMATRIXMULTFEXTPROC glMatrixMultfEXTProc;
	PFNGLMATRIXMULTDEXTPROC glMatrixMultdEXTProc;
	PFNGLMATRIXLOADIDENTITYEXTPROC glMatrixLoadIdentityEXTProc;
	PFNGLMATRIXROTATEFEXTPROC glMatrixRotatefEXTProc;
	PFNGLMATRIXROTATEDEXTPROC glMatrixRotatedEXTProc;
	PFNGLMATRIXSCALEFEXTPROC glMatrixScalefEXTProc;
	PFNGLMATRIXSCALEDEXTPROC glMatrixScaledEXTProc;
	PFNGLMATRIXTRANSLATEFEXTPROC glMatrixTranslatefEXTProc;
	PFNGLMATRIXTRANSLATEDEXTPROC glMatrixTranslatedEXTProc;
	PFNGLMATRIXFRUSTUMEXTPROC glMatrixFrustumEXTProc;
	PFNGLMATRIXORTHOEXTPROC glMatrixOrthoEXTProc;
	PFNGLMATRIXPOPEXTPROC glMatrixPopEXTProc;
	PFNGLMATRIXPUSHEXTPROC glMatrixPushEXTProc;
	PFNGLCLIENTATTRIBDEFAULTEXTPROC glClientAttribDefaultEXTProc;
	PFNGLPUSHCLIENTATTRIBDEFAULTEXTPROC glPushClientAttribDefaultEXTProc;
	PFNGLTEXTUREPARAMETERFEXTPROC glTextureParameterfEXTProc;
	PFNGLTEXTUREPARAMETERFVEXTPROC glTextureParameterfvEXTProc;
	PFNGLTEXTUREPARAMETERIEXTPROC glTextureParameteriEXTProc;
	PFNGLTEXTUREPARAMETERIVEXTPROC glTextureParameterivEXTProc;
	PFNGLTEXTUREIMAGE1DEXTPROC glTextureImage1DEXTProc;
	PFNGLTEXTUREIMAGE2DEXTPROC glTextureImage2DEXTProc;
	PFNGLTEXTURESUBIMAGE1DEXTPROC glTextureSubImage1DEXTProc;
	PFNGLTEXTURESUBIMAGE2DEXTPROC glTextureSubImage2DEXTProc;
	PFNGLCOPYTEXTUREIMAGE1DEXTPROC glCopyTextureImage1DEXTProc;
	PFNGLCOPYTEXTUREIMAGE2DEXTPROC glCopyTextureImage2DEXTProc;
	PFNGLCOPYTEXTURESUBIMAGE1DEXTPROC glCopyTextureSubImage1DEXTProc;
	PFNGLCOPYTEXTURESUBIMAGE2DEXTPROC glCopyTextureSubImage2DEXTProc;
	PFNGLGETTEXTUREIMAGEEXTPROC glGetTextureImageEXTProc;
	PFNGLGETTEXTUREPARAMETERFVEXTPROC glGetTextureParameterfvEXTProc;
	PFNGLGETTEXTUREPARAMETERIVEXTPROC glGetTextureParameterivEXTProc;
	PFNGLGETTEXTURELEVELPARAMETERFVEXTPROC glGetTextureLevelParameterfvEXTProc;
	PFNGLGETTEXTURELEVELPARAMETERIVEXTPROC glGetTextureLevelParameterivEXTProc;
	PFNGLTEXTUREIMAGE3DEXTPROC glTextureImage3DEXTProc;
	PFNGLTEXTURESUBIMAGE3DEXTPROC glTextureSubImage3DEXTProc;
	PFNGLCOPYTEXTURESUBIMAGE3DEXTPROC glCopyTextureSubImage3DEXTProc;
	PFNGLBINDMULTITEXTUREEXTPROC glBindMultiTextureEXTProc;
	PFNGLMULTITEXCOORDPOINTEREXTPROC glMultiTexCoordPointerEXTProc;
	PFNGLMULTITEXENVFEXTPROC glMultiTexEnvfEXTProc;
	PFNGLMULTITEXENVFVEXTPROC glMultiTexEnvfvEXTProc;
	PFNGLMULTITEXENVIEXTPROC glMultiTexEnviEXTProc;
	PFNGLMULTITEXENVIVEXTPROC glMultiTexEnvivEXTProc;
	PFNGLMULTITEXGENDEXTPROC glMultiTexGendEXTProc;
	PFNGLMULTITEXGENDVEXTPROC glMultiTexGendvEXTProc;
	PFNGLMULTITEXGENFEXTPROC glMultiTexGenfEXTProc;
	PFNGLMULTITEXGENFVEXTPROC glMultiTexGenfvEXTProc;
	PFNGLMULTITEXGENIEXTPROC glMultiTexGeniEXTProc;
	PFNGLMULTITEXGENIVEXTPROC glMultiTexGenivEXTProc;
	PFNGLGETMULTITEXENVFVEXTPROC glGetMultiTexEnvfvEXTProc;
	PFNGLGETMULTITEXENVIVEXTPROC glGetMultiTexEnvivEXTProc;
	PFNGLGETMULTITEXGENDVEXTPROC glGetMultiTexGendvEXTProc;
	PFNGLGETMULTITEXGENFVEXTPROC glGetMultiTexGenfvEXTProc;
	PFNGLGETMULTITEXGENIVEXTPROC glGetMultiTexGenivEXTProc;
	PFNGLMULTITEXPARAMETERIEXTPROC glMultiTexParameteriEXTProc;
	PFNGLMULTITEXPARAMETERIVEXTPROC glMultiTexParameterivEXTProc;
	PFNGLMULTITEXPARAMETERFEXTPROC glMultiTexParameterfEXTProc;
	PFNGLMULTITEXPARAMETERFVEXTPROC glMultiTexParameterfvEXTProc;
	PFNGLMULTITEXIMAGE1DEXTPROC glMultiTexImage1DEXTProc;
	PFNGLMULTITEXIMAGE2DEXTPROC glMultiTexImage2DEXTProc;
	PFNGLMULTITEXSUBIMAGE1DEXTPROC glMultiTexSubImage1DEXTProc;
	PFNGLMULTITEXSUBIMAGE2DEXTPROC glMultiTexSubImage2DEXTProc;
	PFNGLCOPYMULTITEXIMAGE1DEXTPROC glCopyMultiTexImage1DEXTProc;
	PFNGLCOPYMULTITEXIMAGE2DEXTPROC glCopyMultiTexImage2DEXTProc;
	PFNGLCOPYMULTITEXSUBIMAGE1DEXTPROC glCopyMultiTexSubImage1DEXTProc;
	PFNGLCOPYMULTITEXSUBIMAGE2DEXTPROC glCopyMultiTexSubImage2DEXTProc;
	PFNGLGETMULTITEXIMAGEEXTPROC glGetMultiTexImageEXTProc;
	PFNGLGETMULTITEXPARAMETERFVEXTPROC glGetMultiTexParameterfvEXTProc;
	PFNGLGETMULTITEXPARAMETERIVEXTPROC glGetMultiTexParameterivEXTProc;
	PFNGLGETMULTITEXLEVELPARAMETERFVEXTPROC glGetMultiTexLevelParameterfvEXTProc;
	PFNGLGETMULTITEXLEVELPARAMETERIVEXTPROC glGetMultiTexLevelParameterivEXTProc;
	PFNGLMULTITEXIMAGE3DEXTPROC glMultiTexImage3DEXTProc;
	PFNGLMULTITEXSUBIMAGE3DEXTPROC glMultiTexSubImage3DEXTProc;
	PFNGLCOPYMULTITEXSUBIMAGE3DEXTPROC glCopyMultiTexSubImage3DEXTProc;
	PFNGLENABLECLIENTSTATEINDEXEDEXTPROC glEnableClientStateIndexedEXTProc;
	PFNGLDISABLECLIENTSTATEINDEXEDEXTPROC glDisableClientStateIndexedEXTProc;
	PFNGLGETFLOATINDEXEDVEXTPROC glGetFloatIndexedvEXTProc;
	PFNGLGETDOUBLEINDEXEDVEXTPROC glGetDoubleIndexedvEXTProc;
	PFNGLGETPOINTERINDEXEDVEXTPROC glGetPointerIndexedvEXTProc;
	PFNGLENABLEINDEXEDEXTPROC glEnableIndexedEXTProc;
	PFNGLDISABLEINDEXEDEXTPROC glDisableIndexedEXTProc;
	PFNGLISENABLEDINDEXEDEXTPROC glIsEnabledIndexedEXTProc;
	PFNGLGETINTEGERINDEXEDVEXTPROC glGetIntegerIndexedvEXTProc;
	PFNGLGETBOOLEANINDEXEDVEXTPROC glGetBooleanIndexedvEXTProc;
	PFNGLCOMPRESSEDTEXTUREIMAGE3DEXTPROC glCompressedTextureImage3DEXTProc;
	PFNGLCOMPRESSEDTEXTUREIMAGE2DEXTPROC glCompressedTextureImage2DEXTProc;
	PFNGLCOMPRESSEDTEXTUREIMAGE1DEXTPROC glCompressedTextureImage1DEXTProc;
	PFNGLCOMPRESSEDTEXTURESUBIMAGE3DEXTPROC glCompressedTextureSubImage3DEXTProc;
	PFNGLCOMPRESSEDTEXTURESUBIMAGE2DEXTPROC glCompressedTextureSubImage2DEXTProc;
	PFNGLCOMPRESSEDTEXTURESUBIMAGE1DEXTPROC glCompressedTextureSubImage1DEXTProc;
	PFNGLGETCOMPRESSEDTEXTUREIMAGEEXTPROC glGetCompressedTextureImageEXTProc;
	PFNGLCOMPRESSEDMULTITEXIMAGE3DEXTPROC glCompressedMultiTexImage3DEXTProc;
	PFNGLCOMPRESSEDMULTITEXIMAGE2DEXTPROC glCompressedMultiTexImage2DEXTProc;
	PFNGLCOMPRESSEDMULTITEXIMAGE1DEXTPROC glCompressedMultiTexImage1DEXTProc;
	PFNGLCOMPRESSEDMULTITEXSUBIMAGE3DEXTPROC glCompressedMultiTexSubImage3DEXTProc;
	PFNGLCOMPRESSEDMULTITEXSUBIMAGE2DEXTPROC glCompressedMultiTexSubImage2DEXTProc;
	PFNGLCOMPRESSEDMULTITEXSUBIMAGE1DEXTPROC glCompressedMultiTexSubImage1DEXTProc;
	PFNGLGETCOMPRESSEDMULTITEXIMAGEEXTPROC glGetCompressedMultiTexImageEXTProc;
	PFNGLMATRIXLOADTRANSPOSEFEXTPROC glMatrixLoadTransposefEXTProc;
	PFNGLMATRIXLOADTRANSPOSEDEXTPROC glMatrixLoadTransposedEXTProc;
	PFNGLMATRIXMULTTRANSPOSEFEXTPROC glMatrixMultTransposefEXTProc;
	PFNGLMATRIXMULTTRANSPOSEDEXTPROC glMatrixMultTransposedEXTProc;
	PFNGLNAMEDBUFFERDATAEXTPROC glNamedBufferDataEXTProc;
	PFNGLNAMEDBUFFERSUBDATAEXTPROC glNamedBufferSubDataEXTProc;
	PFNGLMAPNAMEDBUFFEREXTPROC glMapNamedBufferEXTProc;
	PFNGLUNMAPNAMEDBUFFEREXTPROC glUnmapNamedBufferEXTProc;
	PFNGLGETNAMEDBUFFERPARAMETERIVEXTPROC glGetNamedBufferParameterivEXTProc;
	PFNGLGETNAMEDBUFFERPOINTERVEXTPROC glGetNamedBufferPointervEXTProc;
	PFNGLGETNAMEDBUFFERSUBDATAEXTPROC glGetNamedBufferSubDataEXTProc;
	PFNGLPROGRAMUNIFORM1FEXTPROC glProgramUniform1fEXTProc;
	PFNGLPROGRAMUNIFORM2FEXTPROC glProgramUniform2fEXTProc;
	PFNGLPROGRAMUNIFORM3FEXTPROC glProgramUniform3fEXTProc;
	PFNGLPROGRAMUNIFORM4FEXTPROC glProgramUniform4fEXTProc;
	PFNGLPROGRAMUNIFORM1IEXTPROC glProgramUniform1iEXTProc;
	PFNGLPROGRAMUNIFORM2IEXTPROC glProgramUniform2iEXTProc;
	PFNGLPROGRAMUNIFORM3IEXTPROC glProgramUniform3iEXTProc;
	PFNGLPROGRAMUNIFORM4IEXTPROC glProgramUniform4iEXTProc;
	PFNGLPROGRAMUNIFORM1FVEXTPROC glProgramUniform1fvEXTProc;
	PFNGLPROGRAMUNIFORM2FVEXTPROC glProgramUniform2fvEXTProc;
	PFNGLPROGRAMUNIFORM3FVEXTPROC glProgramUniform3fvEXTProc;
	PFNGLPROGRAMUNIFORM4FVEXTPROC glProgramUniform4fvEXTProc;
	PFNGLPROGRAMUNIFORM1IVEXTPROC glProgramUniform1ivEXTProc;
	PFNGLPROGRAMUNIFORM2IVEXTPROC glProgramUniform2ivEXTProc;
	PFNGLPROGRAMUNIFORM3IVEXTPROC glProgramUniform3ivEXTProc;
	PFNGLPROGRAMUNIFORM4IVEXTPROC glProgramUniform4ivEXTProc;
	PFNGLPROGRAMUNIFORMMATRIX2FVEXTPROC glProgramUniformMatrix2fvEXTProc;
	PFNGLPROGRAMUNIFORMMATRIX3FVEXTPROC glProgramUniformMatrix3fvEXTProc;
	PFNGLPROGRAMUNIFORMMATRIX4FVEXTPROC glProgramUniformMatrix4fvEXTProc;
	PFNGLPROGRAMUNIFORMMATRIX2X3FVEXTPROC glProgramUniformMatrix2x3fvEXTProc;
	PFNGLPROGRAMUNIFORMMATRIX3X2FVEXTPROC glProgramUniformMatrix3x2fvEXTProc;
	PFNGLPROGRAMUNIFORMMATRIX2X4FVEXTPROC glProgramUniformMatrix2x4fvEXTProc;
	PFNGLPROGRAMUNIFORMMATRIX4X2FVEXTPROC glProgramUniformMatrix4x2fvEXTProc;
	PFNGLPROGRAMUNIFORMMATRIX3X4FVEXTPROC glProgramUniformMatrix3x4fvEXTProc;
	PFNGLPROGRAMUNIFORMMATRIX4X3FVEXTPROC glProgramUniformMatrix4x3fvEXTProc;
	PFNGLTEXTUREBUFFEREXTPROC glTextureBufferEXTProc;
	PFNGLMULTITEXBUFFEREXTPROC glMultiTexBufferEXTProc;
	PFNGLTEXTUREPARAMETERIIVEXTPROC glTextureParameterIivEXTProc;
	PFNGLTEXTUREPARAMETERIUIVEXTPROC glTextureParameterIuivEXTProc;
	PFNGLGETTEXTUREPARAMETERIIVEXTPROC glGetTextureParameterIivEXTProc;
	PFNGLGETTEXTUREPARAMETERIUIVEXTPROC glGetTextureParameterIuivEXTProc;
	PFNGLMULTITEXPARAMETERIIVEXTPROC glMultiTexParameterIivEXTProc;
	PFNGLMULTITEXPARAMETERIUIVEXTPROC glMultiTexParameterIuivEXTProc;
	PFNGLGETMULTITEXPARAMETERIIVEXTPROC glGetMultiTexParameterIivEXTProc;
	PFNGLGETMULTITEXPARAMETERIUIVEXTPROC glGetMultiTexParameterIuivEXTProc;
	PFNGLPROGRAMUNIFORM1UIEXTPROC glProgramUniform1uiEXTProc;
	PFNGLPROGRAMUNIFORM2UIEXTPROC glProgramUniform2uiEXTProc;
	PFNGLPROGRAMUNIFORM3UIEXTPROC glProgramUniform3uiEXTProc;
	PFNGLPROGRAMUNIFORM4UIEXTPROC glProgramUniform4uiEXTProc;
	PFNGLPROGRAMUNIFORM1UIVEXTPROC glProgramUniform1uivEXTProc;
	PFNGLPROGRAMUNIFORM2UIVEXTPROC glProgramUniform2uivEXTProc;
	PFNGLPROGRAMUNIFORM3UIVEXTPROC glProgramUniform3uivEXTProc;
	PFNGLPROGRAMUNIFORM4UIVEXTPROC glProgramUniform4uivEXTProc;
	PFNGLNAMEDPROGRAMLOCALPARAMETERS4FVEXTPROC glNamedProgramLocalParameters4fvEXTProc;
	PFNGLNAMEDPROGRAMLOCALPARAMETERI4IEXTPROC glNamedProgramLocalParameterI4iEXTProc;
	PFNGLNAMEDPROGRAMLOCALPARAMETERI4IVEXTPROC glNamedProgramLocalParameterI4ivEXTProc;
	PFNGLNAMEDPROGRAMLOCALPARAMETERSI4IVEXTPROC glNamedProgramLocalParametersI4ivEXTProc;
	PFNGLNAMEDPROGRAMLOCALPARAMETERI4UIEXTPROC glNamedProgramLocalParameterI4uiEXTProc;
	PFNGLNAMEDPROGRAMLOCALPARAMETERI4UIVEXTPROC glNamedProgramLocalParameterI4uivEXTProc;
	PFNGLNAMEDPROGRAMLOCALPARAMETERSI4UIVEXTPROC glNamedProgramLocalParametersI4uivEXTProc;
	PFNGLGETNAMEDPROGRAMLOCALPARAMETERIIVEXTPROC glGetNamedProgramLocalParameterIivEXTProc;
	PFNGLGETNAMEDPROGRAMLOCALPARAMETERIUIVEXTPROC glGetNamedProgramLocalParameterIuivEXTProc;
	PFNGLENABLECLIENTSTATEIEXTPROC glEnableClientStateiEXTProc;
	PFNGLDISABLECLIENTSTATEIEXTPROC glDisableClientStateiEXTProc;
	PFNGLGETFLOATI_VEXTPROC glGetFloati_vEXTProc;
	PFNGLGETDOUBLEI_VEXTPROC glGetDoublei_vEXTProc;
	PFNGLGETPOINTERI_VEXTPROC glGetPointeri_vEXTProc;
	PFNGLNAMEDPROGRAMSTRINGEXTPROC glNamedProgramStringEXTProc;
	PFNGLNAMEDPROGRAMLOCALPARAMETER4DEXTPROC glNamedProgramLocalParameter4dEXTProc;
	PFNGLNAMEDPROGRAMLOCALPARAMETER4DVEXTPROC glNamedProgramLocalParameter4dvEXTProc;
	PFNGLNAMEDPROGRAMLOCALPARAMETER4FEXTPROC glNamedProgramLocalParameter4fEXTProc;
	PFNGLNAMEDPROGRAMLOCALPARAMETER4FVEXTPROC glNamedProgramLocalParameter4fvEXTProc;
	PFNGLGETNAMEDPROGRAMLOCALPARAMETERDVEXTPROC glGetNamedProgramLocalParameterdvEXTProc;
	PFNGLGETNAMEDPROGRAMLOCALPARAMETERFVEXTPROC glGetNamedProgramLocalParameterfvEXTProc;
	PFNGLGETNAMEDPROGRAMIVEXTPROC glGetNamedProgramivEXTProc;
	PFNGLGETNAMEDPROGRAMSTRINGEXTPROC glGetNamedProgramStringEXTProc;
	PFNGLNAMEDRENDERBUFFERSTORAGEEXTPROC glNamedRenderbufferStorageEXTProc;
	PFNGLGETNAMEDRENDERBUFFERPARAMETERIVEXTPROC glGetNamedRenderbufferParameterivEXTProc;
	PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC glNamedRenderbufferStorageMultisampleEXTProc;
	PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLECOVERAGEEXTPROC glNamedRenderbufferStorageMultisampleCoverageEXTProc;
	PFNGLCHECKNAMEDFRAMEBUFFERSTATUSEXTPROC glCheckNamedFramebufferStatusEXTProc;
	PFNGLNAMEDFRAMEBUFFERTEXTURE1DEXTPROC glNamedFramebufferTexture1DEXTProc;
	PFNGLNAMEDFRAMEBUFFERTEXTURE2DEXTPROC glNamedFramebufferTexture2DEXTProc;
	PFNGLNAMEDFRAMEBUFFERTEXTURE3DEXTPROC glNamedFramebufferTexture3DEXTProc;
	PFNGLNAMEDFRAMEBUFFERRENDERBUFFEREXTPROC glNamedFramebufferRenderbufferEXTProc;
	PFNGLGETNAMEDFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glGetNamedFramebufferAttachmentParameterivEXTProc;
	PFNGLGENERATETEXTUREMIPMAPEXTPROC glGenerateTextureMipmapEXTProc;
	PFNGLGENERATEMULTITEXMIPMAPEXTPROC glGenerateMultiTexMipmapEXTProc;
	PFNGLFRAMEBUFFERDRAWBUFFEREXTPROC glFramebufferDrawBufferEXTProc;
	PFNGLFRAMEBUFFERDRAWBUFFERSEXTPROC glFramebufferDrawBuffersEXTProc;
	PFNGLFRAMEBUFFERREADBUFFEREXTPROC glFramebufferReadBufferEXTProc;
	PFNGLGETFRAMEBUFFERPARAMETERIVEXTPROC glGetFramebufferParameterivEXTProc;
	PFNGLNAMEDCOPYBUFFERSUBDATAEXTPROC glNamedCopyBufferSubDataEXTProc;
	PFNGLNAMEDFRAMEBUFFERTEXTUREEXTPROC glNamedFramebufferTextureEXTProc;
	PFNGLNAMEDFRAMEBUFFERTEXTURELAYEREXTPROC glNamedFramebufferTextureLayerEXTProc;
	PFNGLNAMEDFRAMEBUFFERTEXTUREFACEEXTPROC glNamedFramebufferTextureFaceEXTProc;
	PFNGLTEXTURERENDERBUFFEREXTPROC glTextureRenderbufferEXTProc;
	PFNGLMULTITEXRENDERBUFFEREXTPROC glMultiTexRenderbufferEXTProc;
	PFNGLVERTEXARRAYVERTEXOFFSETEXTPROC glVertexArrayVertexOffsetEXTProc;
	PFNGLVERTEXARRAYCOLOROFFSETEXTPROC glVertexArrayColorOffsetEXTProc;
	PFNGLVERTEXARRAYEDGEFLAGOFFSETEXTPROC glVertexArrayEdgeFlagOffsetEXTProc;
	PFNGLVERTEXARRAYINDEXOFFSETEXTPROC glVertexArrayIndexOffsetEXTProc;
	PFNGLVERTEXARRAYNORMALOFFSETEXTPROC glVertexArrayNormalOffsetEXTProc;
	PFNGLVERTEXARRAYTEXCOORDOFFSETEXTPROC glVertexArrayTexCoordOffsetEXTProc;
	PFNGLVERTEXARRAYMULTITEXCOORDOFFSETEXTPROC glVertexArrayMultiTexCoordOffsetEXTProc;
	PFNGLVERTEXARRAYFOGCOORDOFFSETEXTPROC glVertexArrayFogCoordOffsetEXTProc;
	PFNGLVERTEXARRAYSECONDARYCOLOROFFSETEXTPROC glVertexArraySecondaryColorOffsetEXTProc;
	PFNGLVERTEXARRAYVERTEXATTRIBOFFSETEXTPROC glVertexArrayVertexAttribOffsetEXTProc;
	PFNGLVERTEXARRAYVERTEXATTRIBIOFFSETEXTPROC glVertexArrayVertexAttribIOffsetEXTProc;
	PFNGLENABLEVERTEXARRAYEXTPROC glEnableVertexArrayEXTProc;
	PFNGLDISABLEVERTEXARRAYEXTPROC glDisableVertexArrayEXTProc;
	PFNGLENABLEVERTEXARRAYATTRIBEXTPROC glEnableVertexArrayAttribEXTProc;
	PFNGLDISABLEVERTEXARRAYATTRIBEXTPROC glDisableVertexArrayAttribEXTProc;
	PFNGLGETVERTEXARRAYINTEGERVEXTPROC glGetVertexArrayIntegervEXTProc;
	PFNGLGETVERTEXARRAYPOINTERVEXTPROC glGetVertexArrayPointervEXTProc;
	PFNGLGETVERTEXARRAYINTEGERI_VEXTPROC glGetVertexArrayIntegeri_vEXTProc;
	PFNGLGETVERTEXARRAYPOINTERI_VEXTPROC glGetVertexArrayPointeri_vEXTProc;
	PFNGLMAPNAMEDBUFFERRANGEEXTPROC glMapNamedBufferRangeEXTProc;
	PFNGLFLUSHMAPPEDNAMEDBUFFERRANGEEXTPROC glFlushMappedNamedBufferRangeEXTProc;
	PFNGLNAMEDBUFFERSTORAGEEXTPROC glNamedBufferStorageEXTProc;
	PFNGLCLEARNAMEDBUFFERDATAEXTPROC glClearNamedBufferDataEXTProc;
	PFNGLCLEARNAMEDBUFFERSUBDATAEXTPROC glClearNamedBufferSubDataEXTProc;
	PFNGLNAMEDFRAMEBUFFERPARAMETERIEXTPROC glNamedFramebufferParameteriEXTProc;
	PFNGLGETNAMEDFRAMEBUFFERPARAMETERIVEXTPROC glGetNamedFramebufferParameterivEXTProc;
	PFNGLPROGRAMUNIFORM1DEXTPROC glProgramUniform1dEXTProc;
	PFNGLPROGRAMUNIFORM2DEXTPROC glProgramUniform2dEXTProc;
	PFNGLPROGRAMUNIFORM3DEXTPROC glProgramUniform3dEXTProc;
	PFNGLPROGRAMUNIFORM4DEXTPROC glProgramUniform4dEXTProc;
	PFNGLPROGRAMUNIFORM1DVEXTPROC glProgramUniform1dvEXTProc;
	PFNGLPROGRAMUNIFORM2DVEXTPROC glProgramUniform2dvEXTProc;
	PFNGLPROGRAMUNIFORM3DVEXTPROC glProgramUniform3dvEXTProc;
	PFNGLPROGRAMUNIFORM4DVEXTPROC glProgramUniform4dvEXTProc;
	PFNGLPROGRAMUNIFORMMATRIX2DVEXTPROC glProgramUniformMatrix2dvEXTProc;
	PFNGLPROGRAMUNIFORMMATRIX3DVEXTPROC glProgramUniformMatrix3dvEXTProc;
	PFNGLPROGRAMUNIFORMMATRIX4DVEXTPROC glProgramUniformMatrix4dvEXTProc;
	PFNGLPROGRAMUNIFORMMATRIX2X3DVEXTPROC glProgramUniformMatrix2x3dvEXTProc;
	PFNGLPROGRAMUNIFORMMATRIX2X4DVEXTPROC glProgramUniformMatrix2x4dvEXTProc;
	PFNGLPROGRAMUNIFORMMATRIX3X2DVEXTPROC glProgramUniformMatrix3x2dvEXTProc;
	PFNGLPROGRAMUNIFORMMATRIX3X4DVEXTPROC glProgramUniformMatrix3x4dvEXTProc;
	PFNGLPROGRAMUNIFORMMATRIX4X2DVEXTPROC glProgramUniformMatrix4x2dvEXTProc;
	PFNGLPROGRAMUNIFORMMATRIX4X3DVEXTPROC glProgramUniformMatrix4x3dvEXTProc;
	PFNGLTEXTUREBUFFERRANGEEXTPROC glTextureBufferRangeEXTProc;
	PFNGLTEXTURESTORAGE1DEXTPROC glTextureStorage1DEXTProc;
	PFNGLTEXTURESTORAGE2DEXTPROC glTextureStorage2DEXTProc;
	PFNGLTEXTURESTORAGE3DEXTPROC glTextureStorage3DEXTProc;
	PFNGLTEXTURESTORAGE2DMULTISAMPLEEXTPROC glTextureStorage2DMultisampleEXTProc;
	PFNGLTEXTURESTORAGE3DMULTISAMPLEEXTPROC glTextureStorage3DMultisampleEXTProc;
	PFNGLVERTEXARRAYBINDVERTEXBUFFEREXTPROC glVertexArrayBindVertexBufferEXTProc;
	PFNGLVERTEXARRAYVERTEXATTRIBFORMATEXTPROC glVertexArrayVertexAttribFormatEXTProc;
	PFNGLVERTEXARRAYVERTEXATTRIBIFORMATEXTPROC glVertexArrayVertexAttribIFormatEXTProc;
	PFNGLVERTEXARRAYVERTEXATTRIBLFORMATEXTPROC glVertexArrayVertexAttribLFormatEXTProc;
	PFNGLVERTEXARRAYVERTEXATTRIBBINDINGEXTPROC glVertexArrayVertexAttribBindingEXTProc;
	PFNGLVERTEXARRAYVERTEXBINDINGDIVISOREXTPROC glVertexArrayVertexBindingDivisorEXTProc;
	PFNGLVERTEXARRAYVERTEXATTRIBLOFFSETEXTPROC glVertexArrayVertexAttribLOffsetEXTProc;
	PFNGLTEXTUREPAGECOMMITMENTEXTPROC glTexturePageCommitmentEXTProc;
	PFNGLVERTEXARRAYVERTEXATTRIBDIVISOREXTPROC glVertexArrayVertexAttribDivisorEXTProc;
	
	/* Constructors and destructors: */
	private:
	GLEXTDirectStateAccess(void);
	public:
	virtual ~GLEXTDirectStateAccess(void);
	
	/* Methods: */
	public:
	virtual const char* getExtensionName(void) const;
	virtual void activate(void);
	virtual void deactivate(void);
	static bool isSupported(void); // Returns true if the extension is supported in the current OpenGL context
	static void initExtension(void); // Initializes the extension in the current OpenGL context
	
	/* Extension entry points: */
	inline friend void glMatrixLoadfEXT(GLenum mode,const GLfloat* m)
		{
		GLEXTDirectStateAccess::current->glMatrixLoadfEXTProc(mode,m);
		}
	inline friend void glMatrixLoaddEXT(GLenum mode,const GLdouble* m)
		{
		GLEXTDirectStateAccess::current->glMatrixLoaddEXTProc(mode,m);
		}
	inline friend void glMatrixMultfEXT(GLenum mode,const GLfloat* m)
		{
		GLEXTDirectStateAccess::current->glMatrixMultfEXTProc(mode,m);
		}
	inline friend void glMatrixMultdEXT(GLenum mode,const GLdouble* m)
		{
		GLEXTDirectStateAccess::current->glMatrixMultdEXTProc(mode,m);
		}
	inline friend void glMatrixLoadIdentityEXT(GLenum mode)
		{
		GLEXTDirectStateAccess::current->glMatrixLoadIdentityEXTProc(mode);
		}
	inline friend void glMatrixRotatefEXT(GLenum mode,GLfloat angle,GLfloat x,GLfloat y,GLfloat z)
		{
		GLEXTDirectStateAccess::current->glMatrixRotatefEXTProc(mode,angle,x,y,z);
		}
	inline friend void glMatrixRotatedEXT(GLenum mode,GLdouble angle,GLdouble x,GLdouble y,GLdouble z)
		{
		GLEXTDirectStateAccess::current->glMatrixRotatedEXTProc(mode,angle,x,y,z);
		}
	inline friend void glMatrixScalefEXT(GLenum mode,GLfloat x,GLfloat y,GLfloat z)
		{
		GLEXTDirectStateAccess::current->glMatrixScalefEXTProc(mode,x,y,z);
		}
	inline friend void glMatrixScaledEXT(GLenum mode,GLdouble x,GLdouble y,GLdouble z)
		{
		GLEXTDirectStateAccess::current->glMatrixScaledEXTProc(mode,x,y,z);
		}
	inline friend void glMatrixTranslatefEXT(GLenum mode,GLfloat x,GLfloat y,GLfloat z)
		{
		GLEXTDirectStateAccess::current->glMatrixTranslatefEXTProc(mode,x,y,z);
		}
	inline friend void glMatrixTranslatedEXT(GLenum mode,GLdouble x,GLdouble y,GLdouble z)
		{
		GLEXTDirectStateAccess::current->glMatrixTranslatedEXTProc(mode,x,y,z);
		}
	inline friend void glMatrixFrustumEXT(GLenum mode,GLdouble left,GLdouble right,GLdouble bottom,GLdouble top,GLdouble zNear,GLdouble zFar)
		{
		GLEXTDirectStateAccess::current->glMatrixFrustumEXTProc(mode,left,right,bottom,top,zNear,zFar);
		}
	inline friend void glMatrixOrthoEXT(GLenum mode,GLdouble left,GLdouble right,GLdouble bottom,GLdouble top,GLdouble zNear,GLdouble zFar)
		{
		GLEXTDirectStateAccess::current->glMatrixOrthoEXTProc(mode,left,right,bottom,top,zNear,zFar);
		}
	inline friend void glMatrixPopEXT(GLenum mode)
		{
		GLEXTDirectStateAccess::current->glMatrixPopEXTProc(mode);
		}
	inline friend void glMatrixPushEXT(GLenum mode)
		{
		GLEXTDirectStateAccess::current->glMatrixPushEXTProc(mode);
		}
	inline friend void glClientAttribDefaultEXT(GLbitfield mask)
		{
		GLEXTDirectStateAccess::current->glClientAttribDefaultEXTProc(mask);
		}
	inline friend void glPushClientAttribDefaultEXT(GLbitfield mask)
		{
		GLEXTDirectStateAccess::current->glPushClientAttribDefaultEXTProc(mask);
		}
	inline friend void glTextureParameterfEXT(GLuint texture,GLenum target,GLenum pname,GLfloat param)
		{
		GLEXTDirectStateAccess::current->glTextureParameterfEXTProc(texture,target,pname,param);
		}
	inline friend void glTextureParameterfvEXT(GLuint texture,GLenum target,GLenum pname,const GLfloat* params)
		{
		GLEXTDirectStateAccess::current->glTextureParameterfvEXTProc(texture,target,pname,params);
		}
	inline friend void glTextureParameteriEXT(GLuint texture,GLenum target,GLenum pname,GLint param)
		{
		GLEXTDirectStateAccess::current->glTextureParameteriEXTProc(texture,target,pname,param);
		}
	inline friend void glTextureParameterivEXT(GLuint texture,GLenum target,GLenum pname,const GLint* params)
		{
		GLEXTDirectStateAccess::current->glTextureParameterivEXTProc(texture,target,pname,params);
		}
	inline friend void glTextureImage1DEXT(GLuint texture,GLenum target,GLint level,GLint internalformat,GLsizei width,GLint border,GLenum format,GLenum type,const void* pixels)
		{
		GLEXTDirectStateAccess::current->glTextureImage1DEXTProc(texture,target,level,internalformat,width,border,format,type,pixels);
		}
	inline friend void glTextureImage2DEXT(GLuint texture,GLenum target,GLint level,GLint internalformat,GLsizei width,GLsizei height,GLint border,GLenum format,GLenum type,const void* pixels)
		{
		GLEXTDirectStateAccess::current->glTextureImage2DEXTProc(texture,target,level,internalformat,width,height,border,format,type,pixels);
		}
	inline friend void glTextureSubImage1DEXT(GLuint texture,GLenum target,GLint level,GLint xoffset,GLsizei width,GLenum format,GLenum type,const void* pixels)
		{
		GLEXTDirectStateAccess::current->glTextureSubImage1DEXTProc(texture,target,level,xoffset,width,format,type,pixels);
		}
	inline friend void glTextureSubImage2DEXT(GLuint texture,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLsizei width,GLsizei height,GLenum format,GLenum type,const void* pixels)
		{
		GLEXTDirectStateAccess::current->glTextureSubImage2DEXTProc(texture,target,level,xoffset,yoffset,width,height,format,type,pixels);
		}
	inline friend void glCopyTextureImage1DEXT(GLuint texture,GLenum target,GLint level,GLenum internalformat,GLint x,GLint y,GLsizei width,GLint border)
		{
		GLEXTDirectStateAccess::current->glCopyTextureImage1DEXTProc(texture,target,level,internalformat,x,y,width,border);
		}
	inline friend void glCopyTextureImage2DEXT(GLuint texture,GLenum target,GLint level,GLenum internalformat,GLint x,GLint y,GLsizei width,GLsizei height,GLint border)
		{
		GLEXTDirectStateAccess::current->glCopyTextureImage2DEXTProc(texture,target,level,internalformat,x,y,width,height,border);
		}
	inline friend void glCopyTextureSubImage1DEXT(GLuint texture,GLenum target,GLint level,GLint xoffset,GLint x,GLint y,GLsizei width)
		{
		GLEXTDirectStateAccess::current->glCopyTextureSubImage1DEXTProc(texture,target,level,xoffset,x,y,width);
		}
	inline friend void glCopyTextureSubImage2DEXT(GLuint texture,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLint x,GLint y,GLsizei width,GLsizei height)
		{
		GLEXTDirectStateAccess::current->glCopyTextureSubImage2DEXTProc(texture,target,level,xoffset,yoffset,x,y,width,height);
		}
	inline friend void glGetTextureImageEXT(GLuint texture,GLenum target,GLint level,GLenum format,GLenum type,void* pixels)
		{
		GLEXTDirectStateAccess::current->glGetTextureImageEXTProc(texture,target,level,format,type,pixels);
		}
	inline friend void glGetTextureParameterfvEXT(GLuint texture,GLenum target,GLenum pname,GLfloat* params)
		{
		GLEXTDirectStateAccess::current->glGetTextureParameterfvEXTProc(texture,target,pname,params);
		}
	inline friend void glGetTextureParameterivEXT(GLuint texture,GLenum target,GLenum pname,GLint* params)
		{
		GLEXTDirectStateAccess::current->glGetTextureParameterivEXTProc(texture,target,pname,params);
		}
	inline friend void glGetTextureLevelParameterfvEXT(GLuint texture,GLenum target,GLint level,GLenum pname,GLfloat* params)
		{
		GLEXTDirectStateAccess::current->glGetTextureLevelParameterfvEXTProc(texture,target,level,pname,params);
		}
	inline friend void glGetTextureLevelParameterivEXT(GLuint texture,GLenum target,GLint level,GLenum pname,GLint* params)
		{
		GLEXTDirectStateAccess::current->glGetTextureLevelParameterivEXTProc(texture,target,level,pname,params);
		}
	inline friend void glTextureImage3DEXT(GLuint texture,GLenum target,GLint level,GLint internalformat,GLsizei width,GLsizei height,GLsizei depth,GLint border,GLenum format,GLenum type,const void* pixels)
		{
		GLEXTDirectStateAccess::current->glTextureImage3DEXTProc(texture,target,level,internalformat,width,height,depth,border,format,type,pixels);
		}
	inline friend void glTextureSubImage3DEXT(GLuint texture,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLint zoffset,GLsizei width,GLsizei height,GLsizei depth,GLenum format,GLenum type,const void* pixels)
		{
		GLEXTDirectStateAccess::current->glTextureSubImage3DEXTProc(texture,target,level,xoffset,yoffset,zoffset,width,height,depth,format,type,pixels);
		}
	inline friend void glCopyTextureSubImage3DEXT(GLuint texture,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLint zoffset,GLint x,GLint y,GLsizei width,GLsizei height)
		{
		GLEXTDirectStateAccess::current->glCopyTextureSubImage3DEXTProc(texture,target,level,xoffset,yoffset,zoffset,x,y,width,height);
		}
	inline friend void glBindMultiTextureEXT(GLenum texunit,GLenum target,GLuint texture)
		{
		GLEXTDirectStateAccess::current->glBindMultiTextureEXTProc(texunit,target,texture);
		}
	inline friend void glMultiTexCoordPointerEXT(GLenum texunit,GLint size,GLenum type,GLsizei stride,const void* pointer)
		{
		GLEXTDirectStateAccess::current->glMultiTexCoordPointerEXTProc(texunit,size,type,stride,pointer);
		}
	inline friend void glMultiTexEnvfEXT(GLenum texunit,GLenum target,GLenum pname,GLfloat param)
		{
		GLEXTDirectStateAccess::current->glMultiTexEnvfEXTProc(texunit,target,pname,param);
		}
	inline friend void glMultiTexEnvfvEXT(GLenum texunit,GLenum target,GLenum pname,const GLfloat* params)
		{
		GLEXTDirectStateAccess::current->glMultiTexEnvfvEXTProc(texunit,target,pname,params);
		}
	inline friend void glMultiTexEnviEXT(GLenum texunit,GLenum target,GLenum pname,GLint param)
		{
		GLEXTDirectStateAccess::current->glMultiTexEnviEXTProc(texunit,target,pname,param);
		}
	inline friend void glMultiTexEnvivEXT(GLenum texunit,GLenum target,GLenum pname,const GLint* params)
		{
		GLEXTDirectStateAccess::current->glMultiTexEnvivEXTProc(texunit,target,pname,params);
		}
	inline friend void glMultiTexGendEXT(GLenum texunit,GLenum coord,GLenum pname,GLdouble param)
		{
		GLEXTDirectStateAccess::current->glMultiTexGendEXTProc(texunit,coord,pname,param);
		}
	inline friend void glMultiTexGendvEXT(GLenum texunit,GLenum coord,GLenum pname,const GLdouble* params)
		{
		GLEXTDirectStateAccess::current->glMultiTexGendvEXTProc(texunit,coord,pname,params);
		}
	inline friend void glMultiTexGenfEXT(GLenum texunit,GLenum coord,GLenum pname,GLfloat param)
		{
		GLEXTDirectStateAccess::current->glMultiTexGenfEXTProc(texunit,coord,pname,param);
		}
	inline friend void glMultiTexGenfvEXT(GLenum texunit,GLenum coord,GLenum pname,const GLfloat* params)
		{
		GLEXTDirectStateAccess::current->glMultiTexGenfvEXTProc(texunit,coord,pname,params);
		}
	inline friend void glMultiTexGeniEXT(GLenum texunit,GLenum coord,GLenum pname,GLint param)
		{
		GLEXTDirectStateAccess::current->glMultiTexGeniEXTProc(texunit,coord,pname,param);
		}
	inline friend void glMultiTexGenivEXT(GLenum texunit,GLenum coord,GLenum pname,const GLint* params)
		{
		GLEXTDirectStateAccess::current->glMultiTexGenivEXTProc(texunit,coord,pname,params);
		}
	inline friend void glGetMultiTexEnvfvEXT(GLenum texunit,GLenum target,GLenum pname,GLfloat* params)
		{
		GLEXTDirectStateAccess::current->glGetMultiTexEnvfvEXTProc(texunit,target,pname,params);
		}
	inline friend void glGetMultiTexEnvivEXT(GLenum texunit,GLenum target,GLenum pname,GLint* params)
		{
		GLEXTDirectStateAccess::current->glGetMultiTexEnvivEXTProc(texunit,target,pname,params);
		}
	inline friend void glGetMultiTexGendvEXT(GLenum texunit,GLenum coord,GLenum pname,GLdouble* params)
		{
		GLEXTDirectStateAccess::current->glGetMultiTexGendvEXTProc(texunit,coord,pname,params);
		}
	inline friend void glGetMultiTexGenfvEXT(GLenum texunit,GLenum coord,GLenum pname,GLfloat* params)
		{
		GLEXTDirectStateAccess::current->glGetMultiTexGenfvEXTProc(texunit,coord,pname,params);
		}
	inline friend void glGetMultiTexGenivEXT(GLenum texunit,GLenum coord,GLenum pname,GLint* params)
		{
		GLEXTDirectStateAccess::current->glGetMultiTexGenivEXTProc(texunit,coord,pname,params);
		}
	inline friend void glMultiTexParameteriEXT(GLenum texunit,GLenum target,GLenum pname,GLint param)
		{
		GLEXTDirectStateAccess::current->glMultiTexParameteriEXTProc(texunit,target,pname,param);
		}
	inline friend void glMultiTexParameterivEXT(GLenum texunit,GLenum target,GLenum pname,const GLint* params)
		{
		GLEXTDirectStateAccess::current->glMultiTexParameterivEXTProc(texunit,target,pname,params);
		}
	inline friend void glMultiTexParameterfEXT(GLenum texunit,GLenum target,GLenum pname,GLfloat param)
		{
		GLEXTDirectStateAccess::current->glMultiTexParameterfEXTProc(texunit,target,pname,param);
		}
	inline friend void glMultiTexParameterfvEXT(GLenum texunit,GLenum target,GLenum pname,const GLfloat* params)
		{
		GLEXTDirectStateAccess::current->glMultiTexParameterfvEXTProc(texunit,target,pname,params);
		}
	inline friend void glMultiTexImage1DEXT(GLenum texunit,GLenum target,GLint level,GLint internalformat,GLsizei width,GLint border,GLenum format,GLenum type,const void* pixels)
		{
		GLEXTDirectStateAccess::current->glMultiTexImage1DEXTProc(texunit,target,level,internalformat,width,border,format,type,pixels);
		}
	inline friend void glMultiTexImage2DEXT(GLenum texunit,GLenum target,GLint level,GLint internalformat,GLsizei width,GLsizei height,GLint border,GLenum format,GLenum type,const void* pixels)
		{
		GLEXTDirectStateAccess::current->glMultiTexImage2DEXTProc(texunit,target,level,internalformat,width,height,border,format,type,pixels);
		}
	inline friend void glMultiTexSubImage1DEXT(GLenum texunit,GLenum target,GLint level,GLint xoffset,GLsizei width,GLenum format,GLenum type,const void* pixels)
		{
		GLEXTDirectStateAccess::current->glMultiTexSubImage1DEXTProc(texunit,target,level,xoffset,width,format,type,pixels);
		}
	inline friend void glMultiTexSubImage2DEXT(GLenum texunit,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLsizei width,GLsizei height,GLenum format,GLenum type,const void* pixels)
		{
		GLEXTDirectStateAccess::current->glMultiTexSubImage2DEXTProc(texunit,target,level,xoffset,yoffset,width,height,format,type,pixels);
		}
	inline friend void glCopyMultiTexImage1DEXT(GLenum texunit,GLenum target,GLint level,GLenum internalformat,GLint x,GLint y,GLsizei width,GLint border)
		{
		GLEXTDirectStateAccess::current->glCopyMultiTexImage1DEXTProc(texunit,target,level,internalformat,x,y,width,border);
		}
	inline friend void glCopyMultiTexImage2DEXT(GLenum texunit,GLenum target,GLint level,GLenum internalformat,GLint x,GLint y,GLsizei width,GLsizei height,GLint border)
		{
		GLEXTDirectStateAccess::current->glCopyMultiTexImage2DEXTProc(texunit,target,level,internalformat,x,y,width,height,border);
		}
	inline friend void glCopyMultiTexSubImage1DEXT(GLenum texunit,GLenum target,GLint level,GLint xoffset,GLint x,GLint y,GLsizei width)
		{
		GLEXTDirectStateAccess::current->glCopyMultiTexSubImage1DEXTProc(texunit,target,level,xoffset,x,y,width);
		}
	inline friend void glCopyMultiTexSubImage2DEXT(GLenum texunit,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLint x,GLint y,GLsizei width,GLsizei height)
		{
		GLEXTDirectStateAccess::current->glCopyMultiTexSubImage2DEXTProc(texunit,target,level,xoffset,yoffset,x,y,width,height);
		}
	inline friend void glGetMultiTexImageEXT(GLenum texunit,GLenum target,GLint level,GLenum format,GLenum type,void* pixels)
		{
		GLEXTDirectStateAccess::current->glGetMultiTexImageEXTProc(texunit,target,level,format,type,pixels);
		}
	inline friend void glGetMultiTexParameterfvEXT(GLenum texunit,GLenum target,GLenum pname,GLfloat* params)
		{
		GLEXTDirectStateAccess::current->glGetMultiTexParameterfvEXTProc(texunit,target,pname,params);
		}
	inline friend void glGetMultiTexParameterivEXT(GLenum texunit,GLenum target,GLenum pname,GLint* params)
		{
		GLEXTDirectStateAccess::current->glGetMultiTexParameterivEXTProc(texunit,target,pname,params);
		}
	inline friend void glGetMultiTexLevelParameterfvEXT(GLenum texunit,GLenum target,GLint level,GLenum pname,GLfloat* params)
		{
		GLEXTDirectStateAccess::current->glGetMultiTexLevelParameterfvEXTProc(texunit,target,level,pname,params);
		}
	inline friend void glGetMultiTexLevelParameterivEXT(GLenum texunit,GLenum target,GLint level,GLenum pname,GLint* params)
		{
		GLEXTDirectStateAccess::current->glGetMultiTexLevelParameterivEXTProc(texunit,target,level,pname,params);
		}
	inline friend void glMultiTexImage3DEXT(GLenum texunit,GLenum target,GLint level,GLint internalformat,GLsizei width,GLsizei height,GLsizei depth,GLint border,GLenum format,GLenum type,const void* pixels)
		{
		GLEXTDirectStateAccess::current->glMultiTexImage3DEXTProc(texunit,target,level,internalformat,width,height,depth,border,format,type,pixels);
		}
	inline friend void glMultiTexSubImage3DEXT(GLenum texunit,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLint zoffset,GLsizei width,GLsizei height,GLsizei depth,GLenum format,GLenum type,const void* pixels)
		{
		GLEXTDirectStateAccess::current->glMultiTexSubImage3DEXTProc(texunit,target,level,xoffset,yoffset,zoffset,width,height,depth,format,type,pixels);
		}
	inline friend void glCopyMultiTexSubImage3DEXT(GLenum texunit,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLint zoffset,GLint x,GLint y,GLsizei width,GLsizei height)
		{
		GLEXTDirectStateAccess::current->glCopyMultiTexSubImage3DEXTProc(texunit,target,level,xoffset,yoffset,zoffset,x,y,width,height);
		}
	inline friend void glEnableClientStateIndexedEXT(GLenum array,GLuint index)
		{
		GLEXTDirectStateAccess::current->glEnableClientStateIndexedEXTProc(array,index);
		}
	inline friend void glDisableClientStateIndexedEXT(GLenum array,GLuint index)
		{
		GLEXTDirectStateAccess::current->glDisableClientStateIndexedEXTProc(array,index);
		}
	inline friend void glGetFloatIndexedvEXT(GLenum target,GLuint index,GLfloat* data)
		{
		GLEXTDirectStateAccess::current->glGetFloatIndexedvEXTProc(target,index,data);
		}
	inline friend void glGetDoubleIndexedvEXT(GLenum target,GLuint index,GLdouble* data)
		{
		GLEXTDirectStateAccess::current->glGetDoubleIndexedvEXTProc(target,index,data);
		}
	inline friend void glGetPointerIndexedvEXT(GLenum target,GLuint index,void** data)
		{
		GLEXTDirectStateAccess::current->glGetPointerIndexedvEXTProc(target,index,data);
		}
	inline friend void glEnableIndexedEXT(GLenum target,GLuint index)
		{
		GLEXTDirectStateAccess::current->glEnableIndexedEXTProc(target,index);
		}
	inline friend void glDisableIndexedEXT(GLenum target,GLuint index)
		{
		GLEXTDirectStateAccess::current->glDisableIndexedEXTProc(target,index);
		}
	inline friend GLboolean glIsEnabledIndexedEXT(GLenum target,GLuint index)
		{
		return GLEXTDirectStateAccess::current->glIsEnabledIndexedEXTProc(target,index);
		}
	inline friend void glGetIntegerIndexedvEXT(GLenum target,GLuint index,GLint* data)
		{
		GLEXTDirectStateAccess::current->glGetIntegerIndexedvEXTProc(target,index,data);
		}
	inline friend void glGetBooleanIndexedvEXT(GLenum target,GLuint index,GLboolean* data)
		{
		GLEXTDirectStateAccess::current->glGetBooleanIndexedvEXTProc(target,index,data);
		}
	inline friend void glCompressedTextureImage3DEXT(GLuint texture,GLenum target,GLint level,GLenum internalformat,GLsizei width,GLsizei height,GLsizei depth,GLint border,GLsizei imageSize,const void* bits)
		{
		GLEXTDirectStateAccess::current->glCompressedTextureImage3DEXTProc(texture,target,level,internalformat,width,height,depth,border,imageSize,bits);
		}
	inline friend void glCompressedTextureImage2DEXT(GLuint texture,GLenum target,GLint level,GLenum internalformat,GLsizei width,GLsizei height,GLint border,GLsizei imageSize,const void* bits)
		{
		GLEXTDirectStateAccess::current->glCompressedTextureImage2DEXTProc(texture,target,level,internalformat,width,height,border,imageSize,bits);
		}
	inline friend void glCompressedTextureImage1DEXT(GLuint texture,GLenum target,GLint level,GLenum internalformat,GLsizei width,GLint border,GLsizei imageSize,const void* bits)
		{
		GLEXTDirectStateAccess::current->glCompressedTextureImage1DEXTProc(texture,target,level,internalformat,width,border,imageSize,bits);
		}
	inline friend void glCompressedTextureSubImage3DEXT(GLuint texture,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLint zoffset,GLsizei width,GLsizei height,GLsizei depth,GLenum format,GLsizei imageSize,const void* bits)
		{
		GLEXTDirectStateAccess::current->glCompressedTextureSubImage3DEXTProc(texture,target,level,xoffset,yoffset,zoffset,width,height,depth,format,imageSize,bits);
		}
	inline friend void glCompressedTextureSubImage2DEXT(GLuint texture,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLsizei width,GLsizei height,GLenum format,GLsizei imageSize,const void* bits)
		{
		GLEXTDirectStateAccess::current->glCompressedTextureSubImage2DEXTProc(texture,target,level,xoffset,yoffset,width,height,format,imageSize,bits);
		}
	inline friend void glCompressedTextureSubImage1DEXT(GLuint texture,GLenum target,GLint level,GLint xoffset,GLsizei width,GLenum format,GLsizei imageSize,const void* bits)
		{
		GLEXTDirectStateAccess::current->glCompressedTextureSubImage1DEXTProc(texture,target,level,xoffset,width,format,imageSize,bits);
		}
	inline friend void glGetCompressedTextureImageEXT(GLuint texture,GLenum target,GLint lod,void* img)
		{
		GLEXTDirectStateAccess::current->glGetCompressedTextureImageEXTProc(texture,target,lod,img);
		}
	inline friend void glCompressedMultiTexImage3DEXT(GLenum texunit,GLenum target,GLint level,GLenum internalformat,GLsizei width,GLsizei height,GLsizei depth,GLint border,GLsizei imageSize,const void* bits)
		{
		GLEXTDirectStateAccess::current->glCompressedMultiTexImage3DEXTProc(texunit,target,level,internalformat,width,height,depth,border,imageSize,bits);
		}
	inline friend void glCompressedMultiTexImage2DEXT(GLenum texunit,GLenum target,GLint level,GLenum internalformat,GLsizei width,GLsizei height,GLint border,GLsizei imageSize,const void* bits)
		{
		GLEXTDirectStateAccess::current->glCompressedMultiTexImage2DEXTProc(texunit,target,level,internalformat,width,height,border,imageSize,bits);
		}
	inline friend void glCompressedMultiTexImage1DEXT(GLenum texunit,GLenum target,GLint level,GLenum internalformat,GLsizei width,GLint border,GLsizei imageSize,const void* bits)
		{
		GLEXTDirectStateAccess::current->glCompressedMultiTexImage1DEXTProc(texunit,target,level,internalformat,width,border,imageSize,bits);
		}
	inline friend void glCompressedMultiTexSubImage3DEXT(GLenum texunit,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLint zoffset,GLsizei width,GLsizei height,GLsizei depth,GLenum format,GLsizei imageSize,const void* bits)
		{
		GLEXTDirectStateAccess::current->glCompressedMultiTexSubImage3DEXTProc(texunit,target,level,xoffset,yoffset,zoffset,width,height,depth,format,imageSize,bits);
		}
	inline friend void glCompressedMultiTexSubImage2DEXT(GLenum texunit,GLenum target,GLint level,GLint xoffset,GLint yoffset,GLsizei width,GLsizei height,GLenum format,GLsizei imageSize,const void* bits)
		{
		GLEXTDirectStateAccess::current->glCompressedMultiTexSubImage2DEXTProc(texunit,target,level,xoffset,yoffset,width,height,format,imageSize,bits);
		}
	inline friend void glCompressedMultiTexSubImage1DEXT(GLenum texunit,GLenum target,GLint level,GLint xoffset,GLsizei width,GLenum format,GLsizei imageSize,const void* bits)
		{
		GLEXTDirectStateAccess::current->glCompressedMultiTexSubImage1DEXTProc(texunit,target,level,xoffset,width,format,imageSize,bits);
		}
	inline friend void glGetCompressedMultiTexImageEXT(GLenum texunit,GLenum target,GLint lod,void* img)
		{
		GLEXTDirectStateAccess::current->glGetCompressedMultiTexImageEXTProc(texunit,target,lod,img);
		}
	inline friend void glMatrixLoadTransposefEXT(GLenum mode,const GLfloat* m)
		{
		GLEXTDirectStateAccess::current->glMatrixLoadTransposefEXTProc(mode,m);
		}
	inline friend void glMatrixLoadTransposedEXT(GLenum mode,const GLdouble* m)
		{
		GLEXTDirectStateAccess::current->glMatrixLoadTransposedEXTProc(mode,m);
		}
	inline friend void glMatrixMultTransposefEXT(GLenum mode,const GLfloat* m)
		{
		GLEXTDirectStateAccess::current->glMatrixMultTransposefEXTProc(mode,m);
		}
	inline friend void glMatrixMultTransposedEXT(GLenum mode,const GLdouble* m)
		{
		GLEXTDirectStateAccess::current->glMatrixMultTransposedEXTProc(mode,m);
		}
	inline friend void glNamedBufferDataEXT(GLuint buffer,GLsizeiptr size,const void* data,GLenum usage)
		{
		GLEXTDirectStateAccess::current->glNamedBufferDataEXTProc(buffer,size,data,usage);
		}
	inline friend void glNamedBufferSubDataEXT(GLuint buffer,GLintptr offset,GLsizeiptr size,const void* data)
		{
		GLEXTDirectStateAccess::current->glNamedBufferSubDataEXTProc(buffer,offset,size,data);
		}
	inline friend void* glMapNamedBufferEXT(GLuint buffer,GLenum access)
		{
		return GLEXTDirectStateAccess::current->glMapNamedBufferEXTProc(buffer,access);
		}
	inline friend GLboolean glUnmapNamedBufferEXT(GLuint buffer)
		{
		return GLEXTDirectStateAccess::current->glUnmapNamedBufferEXTProc(buffer);
		}
	inline friend void glGetNamedBufferParameterivEXT(GLuint buffer,GLenum pname,GLint* params)
		{
		GLEXTDirectStateAccess::current->glGetNamedBufferParameterivEXTProc(buffer,pname,params);
		}
	inline friend void glGetNamedBufferPointervEXT(GLuint buffer,GLenum pname,void** params)
		{
		GLEXTDirectStateAccess::current->glGetNamedBufferPointervEXTProc(buffer,pname,params);
		}
	inline friend void glGetNamedBufferSubDataEXT(GLuint buffer,GLintptr offset,GLsizeiptr size,void* data)
		{
		GLEXTDirectStateAccess::current->glGetNamedBufferSubDataEXTProc(buffer,offset,size,data);
		}
	inline friend void glProgramUniform1fEXT(GLuint program,GLint location,GLfloat v0)
		{
		GLEXTDirectStateAccess::current->glProgramUniform1fEXTProc(program,location,v0);
		}
	inline friend void glProgramUniform2fEXT(GLuint program,GLint location,GLfloat v0,GLfloat v1)
		{
		GLEXTDirectStateAccess::current->glProgramUniform2fEXTProc(program,location,v0,v1);
		}
	inline friend void glProgramUniform3fEXT(GLuint program,GLint location,GLfloat v0,GLfloat v1,GLfloat v2)
		{
		GLEXTDirectStateAccess::current->glProgramUniform3fEXTProc(program,location,v0,v1,v2);
		}
	inline friend void glProgramUniform4fEXT(GLuint program,GLint location,GLfloat v0,GLfloat v1,GLfloat v2,GLfloat v3)
		{
		GLEXTDirectStateAccess::current->glProgramUniform4fEXTProc(program,location,v0,v1,v2,v3);
		}
	inline friend void glProgramUniform1iEXT(GLuint program,GLint location,GLint v0)
		{
		GLEXTDirectStateAccess::current->glProgramUniform1iEXTProc(program,location,v0);
		}
	inline friend void glProgramUniform2iEXT(GLuint program,GLint location,GLint v0,GLint v1)
		{
		GLEXTDirectStateAccess::current->glProgramUniform2iEXTProc(program,location,v0,v1);
		}
	inline friend void glProgramUniform3iEXT(GLuint program,GLint location,GLint v0,GLint v1,GLint v2)
		{
		GLEXTDirectStateAccess::current->glProgramUniform3iEXTProc(program,location,v0,v1,v2);
		}
	inline friend void glProgramUniform4iEXT(GLuint program,GLint location,GLint v0,GLint v1,GLint v2,GLint v3)
		{
		GLEXTDirectStateAccess::current->glProgramUniform4iEXTProc(program,location,v0,v1,v2,v3);
		}
	inline friend void glProgramUniform1fvEXT(GLuint program,GLint location,GLsizei count,const GLfloat* value)
		{
		GLEXTDirectStateAccess::current->glProgramUniform1fvEXTProc(program,location,count,value);
		}
	inline friend void glProgramUniform2fvEXT(GLuint program,GLint location,GLsizei count,const GLfloat* value)
		{
		GLEXTDirectStateAccess::current->glProgramUniform2fvEXTProc(program,location,count,value);
		}
	inline friend void glProgramUniform3fvEXT(GLuint program,GLint location,GLsizei count,const GLfloat* value)
		{
		GLEXTDirectStateAccess::current->glProgramUniform3fvEXTProc(program,location,count,value);
		}
	inline friend void glProgramUniform4fvEXT(GLuint program,GLint location,GLsizei count,const GLfloat* value)
		{
		GLEXTDirectStateAccess::current->glProgramUniform4fvEXTProc(program,location,count,value);
		}
	inline friend void glProgramUniform1ivEXT(GLuint program,GLint location,GLsizei count,const GLint* value)
		{
		GLEXTDirectStateAccess::current->glProgramUniform1ivEXTProc(program,location,count,value);
		}
	inline friend void glProgramUniform2ivEXT(GLuint program,GLint location,GLsizei count,const GLint* value)
		{
		GLEXTDirectStateAccess::current->glProgramUniform2ivEXTProc(program,location,count,value);
		}
	inline friend void glProgramUniform3ivEXT(GLuint program,GLint location,GLsizei count,const GLint* value)
		{
		GLEXTDirectStateAccess::current->glProgramUniform3ivEXTProc(program,location,count,value);
		}
	inline friend void glProgramUniform4ivEXT(GLuint program,GLint location,GLsizei count,const GLint* value)
		{
		GLEXTDirectStateAccess::current->glProgramUniform4ivEXTProc(program,location,count,value);
		}
	inline friend void glProgramUniformMatrix2fvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLfloat* value)
		{
		GLEXTDirectStateAccess::current->glProgramUniformMatrix2fvEXTProc(program,location,count,transpose,value);
		}
	inline friend void glProgramUniformMatrix3fvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLfloat* value)
		{
		GLEXTDirectStateAccess::current->glProgramUniformMatrix3fvEXTProc(program,location,count,transpose,value);
		}
	inline friend void glProgramUniformMatrix4fvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLfloat* value)
		{
		GLEXTDirectStateAccess::current->glProgramUniformMatrix4fvEXTProc(program,location,count,transpose,value);
		}
	inline friend void glProgramUniformMatrix2x3fvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLfloat* value)
		{
		GLEXTDirectStateAccess::current->glProgramUniformMatrix2x3fvEXTProc(program,location,count,transpose,value);
		}
	inline friend void glProgramUniformMatrix3x2fvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLfloat* value)
		{
		GLEXTDirectStateAccess::current->glProgramUniformMatrix3x2fvEXTProc(program,location,count,transpose,value);
		}
	inline friend void glProgramUniformMatrix2x4fvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLfloat* value)
		{
		GLEXTDirectStateAccess::current->glProgramUniformMatrix2x4fvEXTProc(program,location,count,transpose,value);
		}
	inline friend void glProgramUniformMatrix4x2fvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLfloat* value)
		{
		GLEXTDirectStateAccess::current->glProgramUniformMatrix4x2fvEXTProc(program,location,count,transpose,value);
		}
	inline friend void glProgramUniformMatrix3x4fvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLfloat* value)
		{
		GLEXTDirectStateAccess::current->glProgramUniformMatrix3x4fvEXTProc(program,location,count,transpose,value);
		}
	inline friend void glProgramUniformMatrix4x3fvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLfloat* value)
		{
		GLEXTDirectStateAccess::current->glProgramUniformMatrix4x3fvEXTProc(program,location,count,transpose,value);
		}
	inline friend void glTextureBufferEXT(GLuint texture,GLenum target,GLenum internalformat,GLuint buffer)
		{
		GLEXTDirectStateAccess::current->glTextureBufferEXTProc(texture,target,internalformat,buffer);
		}
	inline friend void glMultiTexBufferEXT(GLenum texunit,GLenum target,GLenum internalformat,GLuint buffer)
		{
		GLEXTDirectStateAccess::current->glMultiTexBufferEXTProc(texunit,target,internalformat,buffer);
		}
	inline friend void glTextureParameterIivEXT(GLuint texture,GLenum target,GLenum pname,const GLint* params)
		{
		GLEXTDirectStateAccess::current->glTextureParameterIivEXTProc(texture,target,pname,params);
		}
	inline friend void glTextureParameterIuivEXT(GLuint texture,GLenum target,GLenum pname,const GLuint* params)
		{
		GLEXTDirectStateAccess::current->glTextureParameterIuivEXTProc(texture,target,pname,params);
		}
	inline friend void glGetTextureParameterIivEXT(GLuint texture,GLenum target,GLenum pname,GLint* params)
		{
		GLEXTDirectStateAccess::current->glGetTextureParameterIivEXTProc(texture,target,pname,params);
		}
	inline friend void glGetTextureParameterIuivEXT(GLuint texture,GLenum target,GLenum pname,GLuint* params)
		{
		GLEXTDirectStateAccess::current->glGetTextureParameterIuivEXTProc(texture,target,pname,params);
		}
	inline friend void glMultiTexParameterIivEXT(GLenum texunit,GLenum target,GLenum pname,const GLint* params)
		{
		GLEXTDirectStateAccess::current->glMultiTexParameterIivEXTProc(texunit,target,pname,params);
		}
	inline friend void glMultiTexParameterIuivEXT(GLenum texunit,GLenum target,GLenum pname,const GLuint* params)
		{
		GLEXTDirectStateAccess::current->glMultiTexParameterIuivEXTProc(texunit,target,pname,params);
		}
	inline friend void glGetMultiTexParameterIivEXT(GLenum texunit,GLenum target,GLenum pname,GLint* params)
		{
		GLEXTDirectStateAccess::current->glGetMultiTexParameterIivEXTProc(texunit,target,pname,params);
		}
	inline friend void glGetMultiTexParameterIuivEXT(GLenum texunit,GLenum target,GLenum pname,GLuint* params)
		{
		GLEXTDirectStateAccess::current->glGetMultiTexParameterIuivEXTProc(texunit,target,pname,params);
		}
	inline friend void glProgramUniform1uiEXT(GLuint program,GLint location,GLuint v0)
		{
		GLEXTDirectStateAccess::current->glProgramUniform1uiEXTProc(program,location,v0);
		}
	inline friend void glProgramUniform2uiEXT(GLuint program,GLint location,GLuint v0,GLuint v1)
		{
		GLEXTDirectStateAccess::current->glProgramUniform2uiEXTProc(program,location,v0,v1);
		}
	inline friend void glProgramUniform3uiEXT(GLuint program,GLint location,GLuint v0,GLuint v1,GLuint v2)
		{
		GLEXTDirectStateAccess::current->glProgramUniform3uiEXTProc(program,location,v0,v1,v2);
		}
	inline friend void glProgramUniform4uiEXT(GLuint program,GLint location,GLuint v0,GLuint v1,GLuint v2,GLuint v3)
		{
		GLEXTDirectStateAccess::current->glProgramUniform4uiEXTProc(program,location,v0,v1,v2,v3);
		}
	inline friend void glProgramUniform1uivEXT(GLuint program,GLint location,GLsizei count,const GLuint* value)
		{
		GLEXTDirectStateAccess::current->glProgramUniform1uivEXTProc(program,location,count,value);
		}
	inline friend void glProgramUniform2uivEXT(GLuint program,GLint location,GLsizei count,const GLuint* value)
		{
		GLEXTDirectStateAccess::current->glProgramUniform2uivEXTProc(program,location,count,value);
		}
	inline friend void glProgramUniform3uivEXT(GLuint program,GLint location,GLsizei count,const GLuint* value)
		{
		GLEXTDirectStateAccess::current->glProgramUniform3uivEXTProc(program,location,count,value);
		}
	inline friend void glProgramUniform4uivEXT(GLuint program,GLint location,GLsizei count,const GLuint* value)
		{
		GLEXTDirectStateAccess::current->glProgramUniform4uivEXTProc(program,location,count,value);
		}
	inline friend void glNamedProgramLocalParameters4fvEXT(GLuint program,GLenum target,GLuint index,GLsizei count,const GLfloat* params)
		{
		GLEXTDirectStateAccess::current->glNamedProgramLocalParameters4fvEXTProc(program,target,index,count,params);
		}
	inline friend void glNamedProgramLocalParameterI4iEXT(GLuint program,GLenum target,GLuint index,GLint x,GLint y,GLint z,GLint w)
		{
		GLEXTDirectStateAccess::current->glNamedProgramLocalParameterI4iEXTProc(program,target,index,x,y,z,w);
		}
	inline friend void glNamedProgramLocalParameterI4ivEXT(GLuint program,GLenum target,GLuint index,const GLint* params)
		{
		GLEXTDirectStateAccess::current->glNamedProgramLocalParameterI4ivEXTProc(program,target,index,params);
		}
	inline friend void glNamedProgramLocalParametersI4ivEXT(GLuint program,GLenum target,GLuint index,GLsizei count,const GLint* params)
		{
		GLEXTDirectStateAccess::current->glNamedProgramLocalParametersI4ivEXTProc(program,target,index,count,params);
		}
	inline friend void glNamedProgramLocalParameterI4uiEXT(GLuint program,GLenum target,GLuint index,GLuint x,GLuint y,GLuint z,GLuint w)
		{
		GLEXTDirectStateAccess::current->glNamedProgramLocalParameterI4uiEXTProc(program,target,index,x,y,z,w);
		}
	inline friend void glNamedProgramLocalParameterI4uivEXT(GLuint program,GLenum target,GLuint index,const GLuint* params)
		{
		GLEXTDirectStateAccess::current->glNamedProgramLocalParameterI4uivEXTProc(program,target,index,params);
		}
	inline friend void glNamedProgramLocalParametersI4uivEXT(GLuint program,GLenum target,GLuint index,GLsizei count,const GLuint* params)
		{
		GLEXTDirectStateAccess::current->glNamedProgramLocalParametersI4uivEXTProc(program,target,index,count,params);
		}
	inline friend void glGetNamedProgramLocalParameterIivEXT(GLuint program,GLenum target,GLuint index,GLint* params)
		{
		GLEXTDirectStateAccess::current->glGetNamedProgramLocalParameterIivEXTProc(program,target,index,params);
		}
	inline friend void glGetNamedProgramLocalParameterIuivEXT(GLuint program,GLenum target,GLuint index,GLuint* params)
		{
		GLEXTDirectStateAccess::current->glGetNamedProgramLocalParameterIuivEXTProc(program,target,index,params);
		}
	inline friend void glEnableClientStateiEXT(GLenum array,GLuint index)
		{
		GLEXTDirectStateAccess::current->glEnableClientStateiEXTProc(array,index);
		}
	inline friend void glDisableClientStateiEXT(GLenum array,GLuint index)
		{
		GLEXTDirectStateAccess::current->glDisableClientStateiEXTProc(array,index);
		}
	inline friend void glGetFloati_vEXT(GLenum pname,GLuint index,GLfloat* params)
		{
		GLEXTDirectStateAccess::current->glGetFloati_vEXTProc(pname,index,params);
		}
	inline friend void glGetDoublei_vEXT(GLenum pname,GLuint index,GLdouble* params)
		{
		GLEXTDirectStateAccess::current->glGetDoublei_vEXTProc(pname,index,params);
		}
	inline friend void glGetPointeri_vEXT(GLenum pname,GLuint index,void** params)
		{
		GLEXTDirectStateAccess::current->glGetPointeri_vEXTProc(pname,index,params);
		}
	inline friend void glNamedProgramStringEXT(GLuint program,GLenum target,GLenum format,GLsizei len,const void* string)
		{
		GLEXTDirectStateAccess::current->glNamedProgramStringEXTProc(program,target,format,len,string);
		}
	inline friend void glNamedProgramLocalParameter4dEXT(GLuint program,GLenum target,GLuint index,GLdouble x,GLdouble y,GLdouble z,GLdouble w)
		{
		GLEXTDirectStateAccess::current->glNamedProgramLocalParameter4dEXTProc(program,target,index,x,y,z,w);
		}
	inline friend void glNamedProgramLocalParameter4dvEXT(GLuint program,GLenum target,GLuint index,const GLdouble* params)
		{
		GLEXTDirectStateAccess::current->glNamedProgramLocalParameter4dvEXTProc(program,target,index,params);
		}
	inline friend void glNamedProgramLocalParameter4fEXT(GLuint program,GLenum target,GLuint index,GLfloat x,GLfloat y,GLfloat z,GLfloat w)
		{
		GLEXTDirectStateAccess::current->glNamedProgramLocalParameter4fEXTProc(program,target,index,x,y,z,w);
		}
	inline friend void glNamedProgramLocalParameter4fvEXT(GLuint program,GLenum target,GLuint index,const GLfloat* params)
		{
		GLEXTDirectStateAccess::current->glNamedProgramLocalParameter4fvEXTProc(program,target,index,params);
		}
	inline friend void glGetNamedProgramLocalParameterdvEXT(GLuint program,GLenum target,GLuint index,GLdouble* params)
		{
		GLEXTDirectStateAccess::current->glGetNamedProgramLocalParameterdvEXTProc(program,target,index,params);
		}
	inline friend void glGetNamedProgramLocalParameterfvEXT(GLuint program,GLenum target,GLuint index,GLfloat* params)
		{
		GLEXTDirectStateAccess::current->glGetNamedProgramLocalParameterfvEXTProc(program,target,index,params);
		}
	inline friend void glGetNamedProgramivEXT(GLuint program,GLenum target,GLenum pname,GLint* params)
		{
		GLEXTDirectStateAccess::current->glGetNamedProgramivEXTProc(program,target,pname,params);
		}
	inline friend void glGetNamedProgramStringEXT(GLuint program,GLenum target,GLenum pname,void* string)
		{
		GLEXTDirectStateAccess::current->glGetNamedProgramStringEXTProc(program,target,pname,string);
		}
	inline friend void glNamedRenderbufferStorageEXT(GLuint renderbuffer,GLenum internalformat,GLsizei width,GLsizei height)
		{
		GLEXTDirectStateAccess::current->glNamedRenderbufferStorageEXTProc(renderbuffer,internalformat,width,height);
		}
	inline friend void glGetNamedRenderbufferParameterivEXT(GLuint renderbuffer,GLenum pname,GLint* params)
		{
		GLEXTDirectStateAccess::current->glGetNamedRenderbufferParameterivEXTProc(renderbuffer,pname,params);
		}
	inline friend void glNamedRenderbufferStorageMultisampleEXT(GLuint renderbuffer,GLsizei samples,GLenum internalformat,GLsizei width,GLsizei height)
		{
		GLEXTDirectStateAccess::current->glNamedRenderbufferStorageMultisampleEXTProc(renderbuffer,samples,internalformat,width,height);
		}
	inline friend void glNamedRenderbufferStorageMultisampleCoverageEXT(GLuint renderbuffer,GLsizei coverageSamples,GLsizei colorSamples,GLenum internalformat,GLsizei width,GLsizei height)
		{
		GLEXTDirectStateAccess::current->glNamedRenderbufferStorageMultisampleCoverageEXTProc(renderbuffer,coverageSamples,colorSamples,internalformat,width,height);
		}
	inline friend GLenum glCheckNamedFramebufferStatusEXT(GLuint framebuffer,GLenum target)
		{
		return GLEXTDirectStateAccess::current->glCheckNamedFramebufferStatusEXTProc(framebuffer,target);
		}
	inline friend void glNamedFramebufferTexture1DEXT(GLuint framebuffer,GLenum attachment,GLenum textarget,GLuint texture,GLint level)
		{
		GLEXTDirectStateAccess::current->glNamedFramebufferTexture1DEXTProc(framebuffer,attachment,textarget,texture,level);
		}
	inline friend void glNamedFramebufferTexture2DEXT(GLuint framebuffer,GLenum attachment,GLenum textarget,GLuint texture,GLint level)
		{
		GLEXTDirectStateAccess::current->glNamedFramebufferTexture2DEXTProc(framebuffer,attachment,textarget,texture,level);
		}
	inline friend void glNamedFramebufferTexture3DEXT(GLuint framebuffer,GLenum attachment,GLenum textarget,GLuint texture,GLint level,GLint zoffset)
		{
		GLEXTDirectStateAccess::current->glNamedFramebufferTexture3DEXTProc(framebuffer,attachment,textarget,texture,level,zoffset);
		}
	inline friend void glNamedFramebufferRenderbufferEXT(GLuint framebuffer,GLenum attachment,GLenum renderbuffertarget,GLuint renderbuffer)
		{
		GLEXTDirectStateAccess::current->glNamedFramebufferRenderbufferEXTProc(framebuffer,attachment,renderbuffertarget,renderbuffer);
		}
	inline friend void glGetNamedFramebufferAttachmentParameterivEXT(GLuint framebuffer,GLenum attachment,GLenum pname,GLint* params)
		{
		GLEXTDirectStateAccess::current->glGetNamedFramebufferAttachmentParameterivEXTProc(framebuffer,attachment,pname,params);
		}
	inline friend void glGenerateTextureMipmapEXT(GLuint texture,GLenum target)
		{
		GLEXTDirectStateAccess::current->glGenerateTextureMipmapEXTProc(texture,target);
		}
	inline friend void glGenerateMultiTexMipmapEXT(GLenum texunit,GLenum target)
		{
		GLEXTDirectStateAccess::current->glGenerateMultiTexMipmapEXTProc(texunit,target);
		}
	inline friend void glFramebufferDrawBufferEXT(GLuint framebuffer,GLenum mode)
		{
		GLEXTDirectStateAccess::current->glFramebufferDrawBufferEXTProc(framebuffer,mode);
		}
	inline friend void glFramebufferDrawBuffersEXT(GLuint framebuffer,GLsizei n,const GLenum* bufs)
		{
		GLEXTDirectStateAccess::current->glFramebufferDrawBuffersEXTProc(framebuffer,n,bufs);
		}
	inline friend void glFramebufferReadBufferEXT(GLuint framebuffer,GLenum mode)
		{
		GLEXTDirectStateAccess::current->glFramebufferReadBufferEXTProc(framebuffer,mode);
		}
	inline friend void glGetFramebufferParameterivEXT(GLuint framebuffer,GLenum pname,GLint* params)
		{
		GLEXTDirectStateAccess::current->glGetFramebufferParameterivEXTProc(framebuffer,pname,params);
		}
	inline friend void glNamedCopyBufferSubDataEXT(GLuint readBuffer,GLuint writeBuffer,GLintptr readOffset,GLintptr writeOffset,GLsizeiptr size)
		{
		GLEXTDirectStateAccess::current->glNamedCopyBufferSubDataEXTProc(readBuffer,writeBuffer,readOffset,writeOffset,size);
		}
	inline friend void glNamedFramebufferTextureEXT(GLuint framebuffer,GLenum attachment,GLuint texture,GLint level)
		{
		GLEXTDirectStateAccess::current->glNamedFramebufferTextureEXTProc(framebuffer,attachment,texture,level);
		}
	inline friend void glNamedFramebufferTextureLayerEXT(GLuint framebuffer,GLenum attachment,GLuint texture,GLint level,GLint layer)
		{
		GLEXTDirectStateAccess::current->glNamedFramebufferTextureLayerEXTProc(framebuffer,attachment,texture,level,layer);
		}
	inline friend void glNamedFramebufferTextureFaceEXT(GLuint framebuffer,GLenum attachment,GLuint texture,GLint level,GLenum face)
		{
		GLEXTDirectStateAccess::current->glNamedFramebufferTextureFaceEXTProc(framebuffer,attachment,texture,level,face);
		}
	inline friend void glTextureRenderbufferEXT(GLuint texture,GLenum target,GLuint renderbuffer)
		{
		GLEXTDirectStateAccess::current->glTextureRenderbufferEXTProc(texture,target,renderbuffer);
		}
	inline friend void glMultiTexRenderbufferEXT(GLenum texunit,GLenum target,GLuint renderbuffer)
		{
		GLEXTDirectStateAccess::current->glMultiTexRenderbufferEXTProc(texunit,target,renderbuffer);
		}
	inline friend void glVertexArrayVertexOffsetEXT(GLuint vaobj,GLuint buffer,GLint size,GLenum type,GLsizei stride,GLintptr offset)
		{
		GLEXTDirectStateAccess::current->glVertexArrayVertexOffsetEXTProc(vaobj,buffer,size,type,stride,offset);
		}
	inline friend void glVertexArrayColorOffsetEXT(GLuint vaobj,GLuint buffer,GLint size,GLenum type,GLsizei stride,GLintptr offset)
		{
		GLEXTDirectStateAccess::current->glVertexArrayColorOffsetEXTProc(vaobj,buffer,size,type,stride,offset);
		}
	inline friend void glVertexArrayEdgeFlagOffsetEXT(GLuint vaobj,GLuint buffer,GLsizei stride,GLintptr offset)
		{
		GLEXTDirectStateAccess::current->glVertexArrayEdgeFlagOffsetEXTProc(vaobj,buffer,stride,offset);
		}
	inline friend void glVertexArrayIndexOffsetEXT(GLuint vaobj,GLuint buffer,GLenum type,GLsizei stride,GLintptr offset)
		{
		GLEXTDirectStateAccess::current->glVertexArrayIndexOffsetEXTProc(vaobj,buffer,type,stride,offset);
		}
	inline friend void glVertexArrayNormalOffsetEXT(GLuint vaobj,GLuint buffer,GLenum type,GLsizei stride,GLintptr offset)
		{
		GLEXTDirectStateAccess::current->glVertexArrayNormalOffsetEXTProc(vaobj,buffer,type,stride,offset);
		}
	inline friend void glVertexArrayTexCoordOffsetEXT(GLuint vaobj,GLuint buffer,GLint size,GLenum type,GLsizei stride,GLintptr offset)
		{
		GLEXTDirectStateAccess::current->glVertexArrayTexCoordOffsetEXTProc(vaobj,buffer,size,type,stride,offset);
		}
	inline friend void glVertexArrayMultiTexCoordOffsetEXT(GLuint vaobj,GLuint buffer,GLenum texunit,GLint size,GLenum type,GLsizei stride,GLintptr offset)
		{
		GLEXTDirectStateAccess::current->glVertexArrayMultiTexCoordOffsetEXTProc(vaobj,buffer,texunit,size,type,stride,offset);
		}
	inline friend void glVertexArrayFogCoordOffsetEXT(GLuint vaobj,GLuint buffer,GLenum type,GLsizei stride,GLintptr offset)
		{
		GLEXTDirectStateAccess::current->glVertexArrayFogCoordOffsetEXTProc(vaobj,buffer,type,stride,offset);
		}
	inline friend void glVertexArraySecondaryColorOffsetEXT(GLuint vaobj,GLuint buffer,GLint size,GLenum type,GLsizei stride,GLintptr offset)
		{
		GLEXTDirectStateAccess::current->glVertexArraySecondaryColorOffsetEXTProc(vaobj,buffer,size,type,stride,offset);
		}
	inline friend void glVertexArrayVertexAttribOffsetEXT(GLuint vaobj,GLuint buffer,GLuint index,GLint size,GLenum type,GLboolean normalized,GLsizei stride,GLintptr offset)
		{
		GLEXTDirectStateAccess::current->glVertexArrayVertexAttribOffsetEXTProc(vaobj,buffer,index,size,type,normalized,stride,offset);
		}
	inline friend void glVertexArrayVertexAttribIOffsetEXT(GLuint vaobj,GLuint buffer,GLuint index,GLint size,GLenum type,GLsizei stride,GLintptr offset)
		{
		GLEXTDirectStateAccess::current->glVertexArrayVertexAttribIOffsetEXTProc(vaobj,buffer,index,size,type,stride,offset);
		}
	inline friend void glEnableVertexArrayEXT(GLuint vaobj,GLenum array)
		{
		GLEXTDirectStateAccess::current->glEnableVertexArrayEXTProc(vaobj,array);
		}
	inline friend void glDisableVertexArrayEXT(GLuint vaobj,GLenum array)
		{
		GLEXTDirectStateAccess::current->glDisableVertexArrayEXTProc(vaobj,array);
		}
	inline friend void glEnableVertexArrayAttribEXT(GLuint vaobj,GLuint index)
		{
		GLEXTDirectStateAccess::current->glEnableVertexArrayAttribEXTProc(vaobj,index);
		}
	inline friend void glDisableVertexArrayAttribEXT(GLuint vaobj,GLuint index)
		{
		GLEXTDirectStateAccess::current->glDisableVertexArrayAttribEXTProc(vaobj,index);
		}
	inline friend void glGetVertexArrayIntegervEXT(GLuint vaobj,GLenum pname,GLint* param)
		{
		GLEXTDirectStateAccess::current->glGetVertexArrayIntegervEXTProc(vaobj,pname,param);
		}
	inline friend void glGetVertexArrayPointervEXT(GLuint vaobj,GLenum pname,void** param)
		{
		GLEXTDirectStateAccess::current->glGetVertexArrayPointervEXTProc(vaobj,pname,param);
		}
	inline friend void glGetVertexArrayIntegeri_vEXT(GLuint vaobj,GLuint index,GLenum pname,GLint* param)
		{
		GLEXTDirectStateAccess::current->glGetVertexArrayIntegeri_vEXTProc(vaobj,index,pname,param);
		}
	inline friend void glGetVertexArrayPointeri_vEXT(GLuint vaobj,GLuint index,GLenum pname,void** param)
		{
		GLEXTDirectStateAccess::current->glGetVertexArrayPointeri_vEXTProc(vaobj,index,pname,param);
		}
	inline friend void* glMapNamedBufferRangeEXT(GLuint buffer,GLintptr offset,GLsizeiptr length,GLbitfield access)
		{
		return GLEXTDirectStateAccess::current->glMapNamedBufferRangeEXTProc(buffer,offset,length,access);
		}
	inline friend void glFlushMappedNamedBufferRangeEXT(GLuint buffer,GLintptr offset,GLsizeiptr length)
		{
		GLEXTDirectStateAccess::current->glFlushMappedNamedBufferRangeEXTProc(buffer,offset,length);
		}
	inline friend void glNamedBufferStorageEXT(GLuint buffer,GLsizeiptr size,const void* data,GLbitfield flags)
		{
		GLEXTDirectStateAccess::current->glNamedBufferStorageEXTProc(buffer,size,data,flags);
		}
	inline friend void glClearNamedBufferDataEXT(GLuint buffer,GLenum internalformat,GLenum format,GLenum type,const void* data)
		{
		GLEXTDirectStateAccess::current->glClearNamedBufferDataEXTProc(buffer,internalformat,format,type,data);
		}
	inline friend void glClearNamedBufferSubDataEXT(GLuint buffer,GLenum internalformat,GLsizeiptr offset,GLsizeiptr size,GLenum format,GLenum type,const void* data)
		{
		GLEXTDirectStateAccess::current->glClearNamedBufferSubDataEXTProc(buffer,internalformat,offset,size,format,type,data);
		}
	inline friend void glNamedFramebufferParameteriEXT(GLuint framebuffer,GLenum pname,GLint param)
		{
		GLEXTDirectStateAccess::current->glNamedFramebufferParameteriEXTProc(framebuffer,pname,param);
		}
	inline friend void glGetNamedFramebufferParameterivEXT(GLuint framebuffer,GLenum pname,GLint* params)
		{
		GLEXTDirectStateAccess::current->glGetNamedFramebufferParameterivEXTProc(framebuffer,pname,params);
		}
	inline friend void glProgramUniform1dEXT(GLuint program,GLint location,GLdouble x)
		{
		GLEXTDirectStateAccess::current->glProgramUniform1dEXTProc(program,location,x);
		}
	inline friend void glProgramUniform2dEXT(GLuint program,GLint location,GLdouble x,GLdouble y)
		{
		GLEXTDirectStateAccess::current->glProgramUniform2dEXTProc(program,location,x,y);
		}
	inline friend void glProgramUniform3dEXT(GLuint program,GLint location,GLdouble x,GLdouble y,GLdouble z)
		{
		GLEXTDirectStateAccess::current->glProgramUniform3dEXTProc(program,location,x,y,z);
		}
	inline friend void glProgramUniform4dEXT(GLuint program,GLint location,GLdouble x,GLdouble y,GLdouble z,GLdouble w)
		{
		GLEXTDirectStateAccess::current->glProgramUniform4dEXTProc(program,location,x,y,z,w);
		}
	inline friend void glProgramUniform1dvEXT(GLuint program,GLint location,GLsizei count,const GLdouble* value)
		{
		GLEXTDirectStateAccess::current->glProgramUniform1dvEXTProc(program,location,count,value);
		}
	inline friend void glProgramUniform2dvEXT(GLuint program,GLint location,GLsizei count,const GLdouble* value)
		{
		GLEXTDirectStateAccess::current->glProgramUniform2dvEXTProc(program,location,count,value);
		}
	inline friend void glProgramUniform3dvEXT(GLuint program,GLint location,GLsizei count,const GLdouble* value)
		{
		GLEXTDirectStateAccess::current->glProgramUniform3dvEXTProc(program,location,count,value);
		}
	inline friend void glProgramUniform4dvEXT(GLuint program,GLint location,GLsizei count,const GLdouble* value)
		{
		GLEXTDirectStateAccess::current->glProgramUniform4dvEXTProc(program,location,count,value);
		}
	inline friend void glProgramUniformMatrix2dvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLdouble* value)
		{
		GLEXTDirectStateAccess::current->glProgramUniformMatrix2dvEXTProc(program,location,count,transpose,value);
		}
	inline friend void glProgramUniformMatrix3dvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLdouble* value)
		{
		GLEXTDirectStateAccess::current->glProgramUniformMatrix3dvEXTProc(program,location,count,transpose,value);
		}
	inline friend void glProgramUniformMatrix4dvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLdouble* value)
		{
		GLEXTDirectStateAccess::current->glProgramUniformMatrix4dvEXTProc(program,location,count,transpose,value);
		}
	inline friend void glProgramUniformMatrix2x3dvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLdouble* value)
		{
		GLEXTDirectStateAccess::current->glProgramUniformMatrix2x3dvEXTProc(program,location,count,transpose,value);
		}
	inline friend void glProgramUniformMatrix2x4dvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLdouble* value)
		{
		GLEXTDirectStateAccess::current->glProgramUniformMatrix2x4dvEXTProc(program,location,count,transpose,value);
		}
	inline friend void glProgramUniformMatrix3x2dvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLdouble* value)
		{
		GLEXTDirectStateAccess::current->glProgramUniformMatrix3x2dvEXTProc(program,location,count,transpose,value);
		}
	inline friend void glProgramUniformMatrix3x4dvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLdouble* value)
		{
		GLEXTDirectStateAccess::current->glProgramUniformMatrix3x4dvEXTProc(program,location,count,transpose,value);
		}
	inline friend void glProgramUniformMatrix4x2dvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLdouble* value)
		{
		GLEXTDirectStateAccess::current->glProgramUniformMatrix4x2dvEXTProc(program,location,count,transpose,value);
		}
	inline friend void glProgramUniformMatrix4x3dvEXT(GLuint program,GLint location,GLsizei count,GLboolean transpose,const GLdouble* value)
		{
		GLEXTDirectStateAccess::current->glProgramUniformMatrix4x3dvEXTProc(program,location,count,transpose,value);
		}
	inline friend void glTextureBufferRangeEXT(GLuint texture,GLenum target,GLenum internalformat,GLuint buffer,GLintptr offset,GLsizeiptr size)
		{
		GLEXTDirectStateAccess::current->glTextureBufferRangeEXTProc(texture,target,internalformat,buffer,offset,size);
		}
	inline friend void glTextureStorage1DEXT(GLuint texture,GLenum target,GLsizei levels,GLenum internalformat,GLsizei width)
		{
		GLEXTDirectStateAccess::current->glTextureStorage1DEXTProc(texture,target,levels,internalformat,width);
		}
	inline friend void glTextureStorage2DEXT(GLuint texture,GLenum target,GLsizei levels,GLenum internalformat,GLsizei width,GLsizei height)
		{
		GLEXTDirectStateAccess::current->glTextureStorage2DEXTProc(texture,target,levels,internalformat,width,height);
		}
	inline friend void glTextureStorage3DEXT(GLuint texture,GLenum target,GLsizei levels,GLenum internalformat,GLsizei width,GLsizei height,GLsizei depth)
		{
		GLEXTDirectStateAccess::current->glTextureStorage3DEXTProc(texture,target,levels,internalformat,width,height,depth);
		}
	inline friend void glTextureStorage2DMultisampleEXT(GLuint texture,GLenum target,GLsizei samples,GLenum internalformat,GLsizei width,GLsizei height,GLboolean fixedsamplelocations)
		{
		GLEXTDirectStateAccess::current->glTextureStorage2DMultisampleEXTProc(texture,target,samples,internalformat,width,height,fixedsamplelocations);
		}
	inline friend void glTextureStorage3DMultisampleEXT(GLuint texture,GLenum target,GLsizei samples,GLenum internalformat,GLsizei width,GLsizei height,GLsizei depth,GLboolean fixedsamplelocations)
		{
		GLEXTDirectStateAccess::current->glTextureStorage3DMultisampleEXTProc(texture,target,samples,internalformat,width,height,depth,fixedsamplelocations);
		}
	inline friend void glVertexArrayBindVertexBufferEXT(GLuint vaobj,GLuint bindingindex,GLuint buffer,GLintptr offset,GLsizei stride)
		{
		GLEXTDirectStateAccess::current->glVertexArrayBindVertexBufferEXTProc(vaobj,bindingindex,buffer,offset,stride);
		}
	inline friend void glVertexArrayVertexAttribFormatEXT(GLuint vaobj,GLuint attribindex,GLint size,GLenum type,GLboolean normalized,GLuint relativeoffset)
		{
		GLEXTDirectStateAccess::current->glVertexArrayVertexAttribFormatEXTProc(vaobj,attribindex,size,type,normalized,relativeoffset);
		}
	inline friend void glVertexArrayVertexAttribIFormatEXT(GLuint vaobj,GLuint attribindex,GLint size,GLenum type,GLuint relativeoffset)
		{
		GLEXTDirectStateAccess::current->glVertexArrayVertexAttribIFormatEXTProc(vaobj,attribindex,size,type,relativeoffset);
		}
	inline friend void glVertexArrayVertexAttribLFormatEXT(GLuint vaobj,GLuint attribindex,GLint size,GLenum type,GLuint relativeoffset)
		{
		GLEXTDirectStateAccess::current->glVertexArrayVertexAttribLFormatEXTProc(vaobj,attribindex,size,type,relativeoffset);
		}
	inline friend void glVertexArrayVertexAttribBindingEXT(GLuint vaobj,GLuint attribindex,GLuint bindingindex)
		{
		GLEXTDirectStateAccess::current->glVertexArrayVertexAttribBindingEXTProc(vaobj,attribindex,bindingindex);
		}
	inline friend void glVertexArrayVertexBindingDivisorEXT(GLuint vaobj,GLuint bindingindex,GLuint divisor)
		{
		GLEXTDirectStateAccess::current->glVertexArrayVertexBindingDivisorEXTProc(vaobj,bindingindex,divisor);
		}
	inline friend void glVertexArrayVertexAttribLOffsetEXT(GLuint vaobj,GLuint buffer,GLuint index,GLint size,GLenum type,GLsizei stride,GLintptr offset)
		{
		GLEXTDirectStateAccess::current->glVertexArrayVertexAttribLOffsetEXTProc(vaobj,buffer,index,size,type,stride,offset);
		}
	inline friend void glTexturePageCommitmentEXT(GLuint texture,GLint level,GLint xoffset,GLint yoffset,GLint zoffset,GLsizei width,GLsizei height,GLsizei depth,GLboolean commit)
		{
		GLEXTDirectStateAccess::current->glTexturePageCommitmentEXTProc(texture,level,xoffset,yoffset,zoffset,width,height,depth,commit);
		}
	inline friend void glVertexArrayVertexAttribDivisorEXT(GLuint vaobj,GLuint index,GLuint divisor)
		{
		GLEXTDirectStateAccess::current->glVertexArrayVertexAttribDivisorEXTProc(vaobj,index,divisor);
		}
	};

/*******************************
Extension-specific entry points:
*******************************/

#endif
