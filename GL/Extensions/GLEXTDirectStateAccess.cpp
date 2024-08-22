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

#include <GL/Extensions/GLEXTDirectStateAccess.h>

#include <GL/gl.h>
#include <GL/GLContextData.h>
#include <GL/GLExtensionManager.h>

/***********************************************
Static elements of class GLEXTDirectStateAccess:
***********************************************/

GL_THREAD_LOCAL(GLEXTDirectStateAccess*) GLEXTDirectStateAccess::current=0;
const char* GLEXTDirectStateAccess::name="GL_EXT_direct_state_access";

/***************************************
Methods of class GLEXTDirectStateAccess:
***************************************/

GLEXTDirectStateAccess::GLEXTDirectStateAccess(void)
	:glMatrixLoadfEXTProc(GLExtensionManager::getFunction<PFNGLMATRIXLOADFEXTPROC>("glMatrixLoadfEXT")),
	 glMatrixLoaddEXTProc(GLExtensionManager::getFunction<PFNGLMATRIXLOADDEXTPROC>("glMatrixLoaddEXT")),
	 glMatrixMultfEXTProc(GLExtensionManager::getFunction<PFNGLMATRIXMULTFEXTPROC>("glMatrixMultfEXT")),
	 glMatrixMultdEXTProc(GLExtensionManager::getFunction<PFNGLMATRIXMULTDEXTPROC>("glMatrixMultdEXT")),
	 glMatrixLoadIdentityEXTProc(GLExtensionManager::getFunction<PFNGLMATRIXLOADIDENTITYEXTPROC>("glMatrixLoadIdentityEXT")),
	 glMatrixRotatefEXTProc(GLExtensionManager::getFunction<PFNGLMATRIXROTATEFEXTPROC>("glMatrixRotatefEXT")),
	 glMatrixRotatedEXTProc(GLExtensionManager::getFunction<PFNGLMATRIXROTATEDEXTPROC>("glMatrixRotatedEXT")),
	 glMatrixScalefEXTProc(GLExtensionManager::getFunction<PFNGLMATRIXSCALEFEXTPROC>("glMatrixScalefEXT")),
	 glMatrixScaledEXTProc(GLExtensionManager::getFunction<PFNGLMATRIXSCALEDEXTPROC>("glMatrixScaledEXT")),
	 glMatrixTranslatefEXTProc(GLExtensionManager::getFunction<PFNGLMATRIXTRANSLATEFEXTPROC>("glMatrixTranslatefEXT")),
	 glMatrixTranslatedEXTProc(GLExtensionManager::getFunction<PFNGLMATRIXTRANSLATEDEXTPROC>("glMatrixTranslatedEXT")),
	 glMatrixFrustumEXTProc(GLExtensionManager::getFunction<PFNGLMATRIXFRUSTUMEXTPROC>("glMatrixFrustumEXT")),
	 glMatrixOrthoEXTProc(GLExtensionManager::getFunction<PFNGLMATRIXORTHOEXTPROC>("glMatrixOrthoEXT")),
	 glMatrixPopEXTProc(GLExtensionManager::getFunction<PFNGLMATRIXPOPEXTPROC>("glMatrixPopEXT")),
	 glMatrixPushEXTProc(GLExtensionManager::getFunction<PFNGLMATRIXPUSHEXTPROC>("glMatrixPushEXT")),
	 glClientAttribDefaultEXTProc(GLExtensionManager::getFunction<PFNGLCLIENTATTRIBDEFAULTEXTPROC>("glClientAttribDefaultEXT")),
	 glPushClientAttribDefaultEXTProc(GLExtensionManager::getFunction<PFNGLPUSHCLIENTATTRIBDEFAULTEXTPROC>("glPushClientAttribDefaultEXT")),
	 glTextureParameterfEXTProc(GLExtensionManager::getFunction<PFNGLTEXTUREPARAMETERFEXTPROC>("glTextureParameterfEXT")),
	 glTextureParameterfvEXTProc(GLExtensionManager::getFunction<PFNGLTEXTUREPARAMETERFVEXTPROC>("glTextureParameterfvEXT")),
	 glTextureParameteriEXTProc(GLExtensionManager::getFunction<PFNGLTEXTUREPARAMETERIEXTPROC>("glTextureParameteriEXT")),
	 glTextureParameterivEXTProc(GLExtensionManager::getFunction<PFNGLTEXTUREPARAMETERIVEXTPROC>("glTextureParameterivEXT")),
	 glTextureImage1DEXTProc(GLExtensionManager::getFunction<PFNGLTEXTUREIMAGE1DEXTPROC>("glTextureImage1DEXT")),
	 glTextureImage2DEXTProc(GLExtensionManager::getFunction<PFNGLTEXTUREIMAGE2DEXTPROC>("glTextureImage2DEXT")),
	 glTextureSubImage1DEXTProc(GLExtensionManager::getFunction<PFNGLTEXTURESUBIMAGE1DEXTPROC>("glTextureSubImage1DEXT")),
	 glTextureSubImage2DEXTProc(GLExtensionManager::getFunction<PFNGLTEXTURESUBIMAGE2DEXTPROC>("glTextureSubImage2DEXT")),
	 glCopyTextureImage1DEXTProc(GLExtensionManager::getFunction<PFNGLCOPYTEXTUREIMAGE1DEXTPROC>("glCopyTextureImage1DEXT")),
	 glCopyTextureImage2DEXTProc(GLExtensionManager::getFunction<PFNGLCOPYTEXTUREIMAGE2DEXTPROC>("glCopyTextureImage2DEXT")),
	 glCopyTextureSubImage1DEXTProc(GLExtensionManager::getFunction<PFNGLCOPYTEXTURESUBIMAGE1DEXTPROC>("glCopyTextureSubImage1DEXT")),
	 glCopyTextureSubImage2DEXTProc(GLExtensionManager::getFunction<PFNGLCOPYTEXTURESUBIMAGE2DEXTPROC>("glCopyTextureSubImage2DEXT")),
	 glGetTextureImageEXTProc(GLExtensionManager::getFunction<PFNGLGETTEXTUREIMAGEEXTPROC>("glGetTextureImageEXT")),
	 glGetTextureParameterfvEXTProc(GLExtensionManager::getFunction<PFNGLGETTEXTUREPARAMETERFVEXTPROC>("glGetTextureParameterfvEXT")),
	 glGetTextureParameterivEXTProc(GLExtensionManager::getFunction<PFNGLGETTEXTUREPARAMETERIVEXTPROC>("glGetTextureParameterivEXT")),
	 glGetTextureLevelParameterfvEXTProc(GLExtensionManager::getFunction<PFNGLGETTEXTURELEVELPARAMETERFVEXTPROC>("glGetTextureLevelParameterfvEXT")),
	 glGetTextureLevelParameterivEXTProc(GLExtensionManager::getFunction<PFNGLGETTEXTURELEVELPARAMETERIVEXTPROC>("glGetTextureLevelParameterivEXT")),
	 glTextureImage3DEXTProc(GLExtensionManager::getFunction<PFNGLTEXTUREIMAGE3DEXTPROC>("glTextureImage3DEXT")),
	 glTextureSubImage3DEXTProc(GLExtensionManager::getFunction<PFNGLTEXTURESUBIMAGE3DEXTPROC>("glTextureSubImage3DEXT")),
	 glCopyTextureSubImage3DEXTProc(GLExtensionManager::getFunction<PFNGLCOPYTEXTURESUBIMAGE3DEXTPROC>("glCopyTextureSubImage3DEXT")),
	 glBindMultiTextureEXTProc(GLExtensionManager::getFunction<PFNGLBINDMULTITEXTUREEXTPROC>("glBindMultiTextureEXT")),
	 glMultiTexCoordPointerEXTProc(GLExtensionManager::getFunction<PFNGLMULTITEXCOORDPOINTEREXTPROC>("glMultiTexCoordPointerEXT")),
	 glMultiTexEnvfEXTProc(GLExtensionManager::getFunction<PFNGLMULTITEXENVFEXTPROC>("glMultiTexEnvfEXT")),
	 glMultiTexEnvfvEXTProc(GLExtensionManager::getFunction<PFNGLMULTITEXENVFVEXTPROC>("glMultiTexEnvfvEXT")),
	 glMultiTexEnviEXTProc(GLExtensionManager::getFunction<PFNGLMULTITEXENVIEXTPROC>("glMultiTexEnviEXT")),
	 glMultiTexEnvivEXTProc(GLExtensionManager::getFunction<PFNGLMULTITEXENVIVEXTPROC>("glMultiTexEnvivEXT")),
	 glMultiTexGendEXTProc(GLExtensionManager::getFunction<PFNGLMULTITEXGENDEXTPROC>("glMultiTexGendEXT")),
	 glMultiTexGendvEXTProc(GLExtensionManager::getFunction<PFNGLMULTITEXGENDVEXTPROC>("glMultiTexGendvEXT")),
	 glMultiTexGenfEXTProc(GLExtensionManager::getFunction<PFNGLMULTITEXGENFEXTPROC>("glMultiTexGenfEXT")),
	 glMultiTexGenfvEXTProc(GLExtensionManager::getFunction<PFNGLMULTITEXGENFVEXTPROC>("glMultiTexGenfvEXT")),
	 glMultiTexGeniEXTProc(GLExtensionManager::getFunction<PFNGLMULTITEXGENIEXTPROC>("glMultiTexGeniEXT")),
	 glMultiTexGenivEXTProc(GLExtensionManager::getFunction<PFNGLMULTITEXGENIVEXTPROC>("glMultiTexGenivEXT")),
	 glGetMultiTexEnvfvEXTProc(GLExtensionManager::getFunction<PFNGLGETMULTITEXENVFVEXTPROC>("glGetMultiTexEnvfvEXT")),
	 glGetMultiTexEnvivEXTProc(GLExtensionManager::getFunction<PFNGLGETMULTITEXENVIVEXTPROC>("glGetMultiTexEnvivEXT")),
	 glGetMultiTexGendvEXTProc(GLExtensionManager::getFunction<PFNGLGETMULTITEXGENDVEXTPROC>("glGetMultiTexGendvEXT")),
	 glGetMultiTexGenfvEXTProc(GLExtensionManager::getFunction<PFNGLGETMULTITEXGENFVEXTPROC>("glGetMultiTexGenfvEXT")),
	 glGetMultiTexGenivEXTProc(GLExtensionManager::getFunction<PFNGLGETMULTITEXGENIVEXTPROC>("glGetMultiTexGenivEXT")),
	 glMultiTexParameteriEXTProc(GLExtensionManager::getFunction<PFNGLMULTITEXPARAMETERIEXTPROC>("glMultiTexParameteriEXT")),
	 glMultiTexParameterivEXTProc(GLExtensionManager::getFunction<PFNGLMULTITEXPARAMETERIVEXTPROC>("glMultiTexParameterivEXT")),
	 glMultiTexParameterfEXTProc(GLExtensionManager::getFunction<PFNGLMULTITEXPARAMETERFEXTPROC>("glMultiTexParameterfEXT")),
	 glMultiTexParameterfvEXTProc(GLExtensionManager::getFunction<PFNGLMULTITEXPARAMETERFVEXTPROC>("glMultiTexParameterfvEXT")),
	 glMultiTexImage1DEXTProc(GLExtensionManager::getFunction<PFNGLMULTITEXIMAGE1DEXTPROC>("glMultiTexImage1DEXT")),
	 glMultiTexImage2DEXTProc(GLExtensionManager::getFunction<PFNGLMULTITEXIMAGE2DEXTPROC>("glMultiTexImage2DEXT")),
	 glMultiTexSubImage1DEXTProc(GLExtensionManager::getFunction<PFNGLMULTITEXSUBIMAGE1DEXTPROC>("glMultiTexSubImage1DEXT")),
	 glMultiTexSubImage2DEXTProc(GLExtensionManager::getFunction<PFNGLMULTITEXSUBIMAGE2DEXTPROC>("glMultiTexSubImage2DEXT")),
	 glCopyMultiTexImage1DEXTProc(GLExtensionManager::getFunction<PFNGLCOPYMULTITEXIMAGE1DEXTPROC>("glCopyMultiTexImage1DEXT")),
	 glCopyMultiTexImage2DEXTProc(GLExtensionManager::getFunction<PFNGLCOPYMULTITEXIMAGE2DEXTPROC>("glCopyMultiTexImage2DEXT")),
	 glCopyMultiTexSubImage1DEXTProc(GLExtensionManager::getFunction<PFNGLCOPYMULTITEXSUBIMAGE1DEXTPROC>("glCopyMultiTexSubImage1DEXT")),
	 glCopyMultiTexSubImage2DEXTProc(GLExtensionManager::getFunction<PFNGLCOPYMULTITEXSUBIMAGE2DEXTPROC>("glCopyMultiTexSubImage2DEXT")),
	 glGetMultiTexImageEXTProc(GLExtensionManager::getFunction<PFNGLGETMULTITEXIMAGEEXTPROC>("glGetMultiTexImageEXT")),
	 glGetMultiTexParameterfvEXTProc(GLExtensionManager::getFunction<PFNGLGETMULTITEXPARAMETERFVEXTPROC>("glGetMultiTexParameterfvEXT")),
	 glGetMultiTexParameterivEXTProc(GLExtensionManager::getFunction<PFNGLGETMULTITEXPARAMETERIVEXTPROC>("glGetMultiTexParameterivEXT")),
	 glGetMultiTexLevelParameterfvEXTProc(GLExtensionManager::getFunction<PFNGLGETMULTITEXLEVELPARAMETERFVEXTPROC>("glGetMultiTexLevelParameterfvEXT")),
	 glGetMultiTexLevelParameterivEXTProc(GLExtensionManager::getFunction<PFNGLGETMULTITEXLEVELPARAMETERIVEXTPROC>("glGetMultiTexLevelParameterivEXT")),
	 glMultiTexImage3DEXTProc(GLExtensionManager::getFunction<PFNGLMULTITEXIMAGE3DEXTPROC>("glMultiTexImage3DEXT")),
	 glMultiTexSubImage3DEXTProc(GLExtensionManager::getFunction<PFNGLMULTITEXSUBIMAGE3DEXTPROC>("glMultiTexSubImage3DEXT")),
	 glCopyMultiTexSubImage3DEXTProc(GLExtensionManager::getFunction<PFNGLCOPYMULTITEXSUBIMAGE3DEXTPROC>("glCopyMultiTexSubImage3DEXT")),
	 glEnableClientStateIndexedEXTProc(GLExtensionManager::getFunction<PFNGLENABLECLIENTSTATEINDEXEDEXTPROC>("glEnableClientStateIndexedEXT")),
	 glDisableClientStateIndexedEXTProc(GLExtensionManager::getFunction<PFNGLDISABLECLIENTSTATEINDEXEDEXTPROC>("glDisableClientStateIndexedEXT")),
	 glGetFloatIndexedvEXTProc(GLExtensionManager::getFunction<PFNGLGETFLOATINDEXEDVEXTPROC>("glGetFloatIndexedvEXT")),
	 glGetDoubleIndexedvEXTProc(GLExtensionManager::getFunction<PFNGLGETDOUBLEINDEXEDVEXTPROC>("glGetDoubleIndexedvEXT")),
	 glGetPointerIndexedvEXTProc(GLExtensionManager::getFunction<PFNGLGETPOINTERINDEXEDVEXTPROC>("glGetPointerIndexedvEXT")),
	 glEnableIndexedEXTProc(GLExtensionManager::getFunction<PFNGLENABLEINDEXEDEXTPROC>("glEnableIndexedEXT")),
	 glDisableIndexedEXTProc(GLExtensionManager::getFunction<PFNGLDISABLEINDEXEDEXTPROC>("glDisableIndexedEXT")),
	 glIsEnabledIndexedEXTProc(GLExtensionManager::getFunction<PFNGLISENABLEDINDEXEDEXTPROC>("glIsEnabledIndexedEXT")),
	 glGetIntegerIndexedvEXTProc(GLExtensionManager::getFunction<PFNGLGETINTEGERINDEXEDVEXTPROC>("glGetIntegerIndexedvEXT")),
	 glGetBooleanIndexedvEXTProc(GLExtensionManager::getFunction<PFNGLGETBOOLEANINDEXEDVEXTPROC>("glGetBooleanIndexedvEXT")),
	 glCompressedTextureImage3DEXTProc(GLExtensionManager::getFunction<PFNGLCOMPRESSEDTEXTUREIMAGE3DEXTPROC>("glCompressedTextureImage3DEXT")),
	 glCompressedTextureImage2DEXTProc(GLExtensionManager::getFunction<PFNGLCOMPRESSEDTEXTUREIMAGE2DEXTPROC>("glCompressedTextureImage2DEXT")),
	 glCompressedTextureImage1DEXTProc(GLExtensionManager::getFunction<PFNGLCOMPRESSEDTEXTUREIMAGE1DEXTPROC>("glCompressedTextureImage1DEXT")),
	 glCompressedTextureSubImage3DEXTProc(GLExtensionManager::getFunction<PFNGLCOMPRESSEDTEXTURESUBIMAGE3DEXTPROC>("glCompressedTextureSubImage3DEXT")),
	 glCompressedTextureSubImage2DEXTProc(GLExtensionManager::getFunction<PFNGLCOMPRESSEDTEXTURESUBIMAGE2DEXTPROC>("glCompressedTextureSubImage2DEXT")),
	 glCompressedTextureSubImage1DEXTProc(GLExtensionManager::getFunction<PFNGLCOMPRESSEDTEXTURESUBIMAGE1DEXTPROC>("glCompressedTextureSubImage1DEXT")),
	 glGetCompressedTextureImageEXTProc(GLExtensionManager::getFunction<PFNGLGETCOMPRESSEDTEXTUREIMAGEEXTPROC>("glGetCompressedTextureImageEXT")),
	 glCompressedMultiTexImage3DEXTProc(GLExtensionManager::getFunction<PFNGLCOMPRESSEDMULTITEXIMAGE3DEXTPROC>("glCompressedMultiTexImage3DEXT")),
	 glCompressedMultiTexImage2DEXTProc(GLExtensionManager::getFunction<PFNGLCOMPRESSEDMULTITEXIMAGE2DEXTPROC>("glCompressedMultiTexImage2DEXT")),
	 glCompressedMultiTexImage1DEXTProc(GLExtensionManager::getFunction<PFNGLCOMPRESSEDMULTITEXIMAGE1DEXTPROC>("glCompressedMultiTexImage1DEXT")),
	 glCompressedMultiTexSubImage3DEXTProc(GLExtensionManager::getFunction<PFNGLCOMPRESSEDMULTITEXSUBIMAGE3DEXTPROC>("glCompressedMultiTexSubImage3DEXT")),
	 glCompressedMultiTexSubImage2DEXTProc(GLExtensionManager::getFunction<PFNGLCOMPRESSEDMULTITEXSUBIMAGE2DEXTPROC>("glCompressedMultiTexSubImage2DEXT")),
	 glCompressedMultiTexSubImage1DEXTProc(GLExtensionManager::getFunction<PFNGLCOMPRESSEDMULTITEXSUBIMAGE1DEXTPROC>("glCompressedMultiTexSubImage1DEXT")),
	 glGetCompressedMultiTexImageEXTProc(GLExtensionManager::getFunction<PFNGLGETCOMPRESSEDMULTITEXIMAGEEXTPROC>("glGetCompressedMultiTexImageEXT")),
	 glMatrixLoadTransposefEXTProc(GLExtensionManager::getFunction<PFNGLMATRIXLOADTRANSPOSEFEXTPROC>("glMatrixLoadTransposefEXT")),
	 glMatrixLoadTransposedEXTProc(GLExtensionManager::getFunction<PFNGLMATRIXLOADTRANSPOSEDEXTPROC>("glMatrixLoadTransposedEXT")),
	 glMatrixMultTransposefEXTProc(GLExtensionManager::getFunction<PFNGLMATRIXMULTTRANSPOSEFEXTPROC>("glMatrixMultTransposefEXT")),
	 glMatrixMultTransposedEXTProc(GLExtensionManager::getFunction<PFNGLMATRIXMULTTRANSPOSEDEXTPROC>("glMatrixMultTransposedEXT")),
	 glNamedBufferDataEXTProc(GLExtensionManager::getFunction<PFNGLNAMEDBUFFERDATAEXTPROC>("glNamedBufferDataEXT")),
	 glNamedBufferSubDataEXTProc(GLExtensionManager::getFunction<PFNGLNAMEDBUFFERSUBDATAEXTPROC>("glNamedBufferSubDataEXT")),
	 glMapNamedBufferEXTProc(GLExtensionManager::getFunction<PFNGLMAPNAMEDBUFFEREXTPROC>("glMapNamedBufferEXT")),
	 glUnmapNamedBufferEXTProc(GLExtensionManager::getFunction<PFNGLUNMAPNAMEDBUFFEREXTPROC>("glUnmapNamedBufferEXT")),
	 glGetNamedBufferParameterivEXTProc(GLExtensionManager::getFunction<PFNGLGETNAMEDBUFFERPARAMETERIVEXTPROC>("glGetNamedBufferParameterivEXT")),
	 glGetNamedBufferPointervEXTProc(GLExtensionManager::getFunction<PFNGLGETNAMEDBUFFERPOINTERVEXTPROC>("glGetNamedBufferPointervEXT")),
	 glGetNamedBufferSubDataEXTProc(GLExtensionManager::getFunction<PFNGLGETNAMEDBUFFERSUBDATAEXTPROC>("glGetNamedBufferSubDataEXT")),
	 glProgramUniform1fEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORM1FEXTPROC>("glProgramUniform1fEXT")),
	 glProgramUniform2fEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORM2FEXTPROC>("glProgramUniform2fEXT")),
	 glProgramUniform3fEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORM3FEXTPROC>("glProgramUniform3fEXT")),
	 glProgramUniform4fEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORM4FEXTPROC>("glProgramUniform4fEXT")),
	 glProgramUniform1iEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORM1IEXTPROC>("glProgramUniform1iEXT")),
	 glProgramUniform2iEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORM2IEXTPROC>("glProgramUniform2iEXT")),
	 glProgramUniform3iEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORM3IEXTPROC>("glProgramUniform3iEXT")),
	 glProgramUniform4iEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORM4IEXTPROC>("glProgramUniform4iEXT")),
	 glProgramUniform1fvEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORM1FVEXTPROC>("glProgramUniform1fvEXT")),
	 glProgramUniform2fvEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORM2FVEXTPROC>("glProgramUniform2fvEXT")),
	 glProgramUniform3fvEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORM3FVEXTPROC>("glProgramUniform3fvEXT")),
	 glProgramUniform4fvEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORM4FVEXTPROC>("glProgramUniform4fvEXT")),
	 glProgramUniform1ivEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORM1IVEXTPROC>("glProgramUniform1ivEXT")),
	 glProgramUniform2ivEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORM2IVEXTPROC>("glProgramUniform2ivEXT")),
	 glProgramUniform3ivEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORM3IVEXTPROC>("glProgramUniform3ivEXT")),
	 glProgramUniform4ivEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORM4IVEXTPROC>("glProgramUniform4ivEXT")),
	 glProgramUniformMatrix2fvEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORMMATRIX2FVEXTPROC>("glProgramUniformMatrix2fvEXT")),
	 glProgramUniformMatrix3fvEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORMMATRIX3FVEXTPROC>("glProgramUniformMatrix3fvEXT")),
	 glProgramUniformMatrix4fvEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORMMATRIX4FVEXTPROC>("glProgramUniformMatrix4fvEXT")),
	 glProgramUniformMatrix2x3fvEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORMMATRIX2X3FVEXTPROC>("glProgramUniformMatrix2x3fvEXT")),
	 glProgramUniformMatrix3x2fvEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORMMATRIX3X2FVEXTPROC>("glProgramUniformMatrix3x2fvEXT")),
	 glProgramUniformMatrix2x4fvEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORMMATRIX2X4FVEXTPROC>("glProgramUniformMatrix2x4fvEXT")),
	 glProgramUniformMatrix4x2fvEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORMMATRIX4X2FVEXTPROC>("glProgramUniformMatrix4x2fvEXT")),
	 glProgramUniformMatrix3x4fvEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORMMATRIX3X4FVEXTPROC>("glProgramUniformMatrix3x4fvEXT")),
	 glProgramUniformMatrix4x3fvEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORMMATRIX4X3FVEXTPROC>("glProgramUniformMatrix4x3fvEXT")),
	 glTextureBufferEXTProc(GLExtensionManager::getFunction<PFNGLTEXTUREBUFFEREXTPROC>("glTextureBufferEXT")),
	 glMultiTexBufferEXTProc(GLExtensionManager::getFunction<PFNGLMULTITEXBUFFEREXTPROC>("glMultiTexBufferEXT")),
	 glTextureParameterIivEXTProc(GLExtensionManager::getFunction<PFNGLTEXTUREPARAMETERIIVEXTPROC>("glTextureParameterIivEXT")),
	 glTextureParameterIuivEXTProc(GLExtensionManager::getFunction<PFNGLTEXTUREPARAMETERIUIVEXTPROC>("glTextureParameterIuivEXT")),
	 glGetTextureParameterIivEXTProc(GLExtensionManager::getFunction<PFNGLGETTEXTUREPARAMETERIIVEXTPROC>("glGetTextureParameterIivEXT")),
	 glGetTextureParameterIuivEXTProc(GLExtensionManager::getFunction<PFNGLGETTEXTUREPARAMETERIUIVEXTPROC>("glGetTextureParameterIuivEXT")),
	 glMultiTexParameterIivEXTProc(GLExtensionManager::getFunction<PFNGLMULTITEXPARAMETERIIVEXTPROC>("glMultiTexParameterIivEXT")),
	 glMultiTexParameterIuivEXTProc(GLExtensionManager::getFunction<PFNGLMULTITEXPARAMETERIUIVEXTPROC>("glMultiTexParameterIuivEXT")),
	 glGetMultiTexParameterIivEXTProc(GLExtensionManager::getFunction<PFNGLGETMULTITEXPARAMETERIIVEXTPROC>("glGetMultiTexParameterIivEXT")),
	 glGetMultiTexParameterIuivEXTProc(GLExtensionManager::getFunction<PFNGLGETMULTITEXPARAMETERIUIVEXTPROC>("glGetMultiTexParameterIuivEXT")),
	 glProgramUniform1uiEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORM1UIEXTPROC>("glProgramUniform1uiEXT")),
	 glProgramUniform2uiEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORM2UIEXTPROC>("glProgramUniform2uiEXT")),
	 glProgramUniform3uiEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORM3UIEXTPROC>("glProgramUniform3uiEXT")),
	 glProgramUniform4uiEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORM4UIEXTPROC>("glProgramUniform4uiEXT")),
	 glProgramUniform1uivEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORM1UIVEXTPROC>("glProgramUniform1uivEXT")),
	 glProgramUniform2uivEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORM2UIVEXTPROC>("glProgramUniform2uivEXT")),
	 glProgramUniform3uivEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORM3UIVEXTPROC>("glProgramUniform3uivEXT")),
	 glProgramUniform4uivEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORM4UIVEXTPROC>("glProgramUniform4uivEXT")),
	 glNamedProgramLocalParameters4fvEXTProc(GLExtensionManager::getFunction<PFNGLNAMEDPROGRAMLOCALPARAMETERS4FVEXTPROC>("glNamedProgramLocalParameters4fvEXT")),
	 glNamedProgramLocalParameterI4iEXTProc(GLExtensionManager::getFunction<PFNGLNAMEDPROGRAMLOCALPARAMETERI4IEXTPROC>("glNamedProgramLocalParameterI4iEXT")),
	 glNamedProgramLocalParameterI4ivEXTProc(GLExtensionManager::getFunction<PFNGLNAMEDPROGRAMLOCALPARAMETERI4IVEXTPROC>("glNamedProgramLocalParameterI4ivEXT")),
	 glNamedProgramLocalParametersI4ivEXTProc(GLExtensionManager::getFunction<PFNGLNAMEDPROGRAMLOCALPARAMETERSI4IVEXTPROC>("glNamedProgramLocalParametersI4ivEXT")),
	 glNamedProgramLocalParameterI4uiEXTProc(GLExtensionManager::getFunction<PFNGLNAMEDPROGRAMLOCALPARAMETERI4UIEXTPROC>("glNamedProgramLocalParameterI4uiEXT")),
	 glNamedProgramLocalParameterI4uivEXTProc(GLExtensionManager::getFunction<PFNGLNAMEDPROGRAMLOCALPARAMETERI4UIVEXTPROC>("glNamedProgramLocalParameterI4uivEXT")),
	 glNamedProgramLocalParametersI4uivEXTProc(GLExtensionManager::getFunction<PFNGLNAMEDPROGRAMLOCALPARAMETERSI4UIVEXTPROC>("glNamedProgramLocalParametersI4uivEXT")),
	 glGetNamedProgramLocalParameterIivEXTProc(GLExtensionManager::getFunction<PFNGLGETNAMEDPROGRAMLOCALPARAMETERIIVEXTPROC>("glGetNamedProgramLocalParameterIivEXT")),
	 glGetNamedProgramLocalParameterIuivEXTProc(GLExtensionManager::getFunction<PFNGLGETNAMEDPROGRAMLOCALPARAMETERIUIVEXTPROC>("glGetNamedProgramLocalParameterIuivEXT")),
	 glEnableClientStateiEXTProc(GLExtensionManager::getFunction<PFNGLENABLECLIENTSTATEIEXTPROC>("glEnableClientStateiEXT")),
	 glDisableClientStateiEXTProc(GLExtensionManager::getFunction<PFNGLDISABLECLIENTSTATEIEXTPROC>("glDisableClientStateiEXT")),
	 glGetFloati_vEXTProc(GLExtensionManager::getFunction<PFNGLGETFLOATI_VEXTPROC>("glGetFloati_vEXT")),
	 glGetDoublei_vEXTProc(GLExtensionManager::getFunction<PFNGLGETDOUBLEI_VEXTPROC>("glGetDoublei_vEXT")),
	 glGetPointeri_vEXTProc(GLExtensionManager::getFunction<PFNGLGETPOINTERI_VEXTPROC>("glGetPointeri_vEXT")),
	 glNamedProgramStringEXTProc(GLExtensionManager::getFunction<PFNGLNAMEDPROGRAMSTRINGEXTPROC>("glNamedProgramStringEXT")),
	 glNamedProgramLocalParameter4dEXTProc(GLExtensionManager::getFunction<PFNGLNAMEDPROGRAMLOCALPARAMETER4DEXTPROC>("glNamedProgramLocalParameter4dEXT")),
	 glNamedProgramLocalParameter4dvEXTProc(GLExtensionManager::getFunction<PFNGLNAMEDPROGRAMLOCALPARAMETER4DVEXTPROC>("glNamedProgramLocalParameter4dvEXT")),
	 glNamedProgramLocalParameter4fEXTProc(GLExtensionManager::getFunction<PFNGLNAMEDPROGRAMLOCALPARAMETER4FEXTPROC>("glNamedProgramLocalParameter4fEXT")),
	 glNamedProgramLocalParameter4fvEXTProc(GLExtensionManager::getFunction<PFNGLNAMEDPROGRAMLOCALPARAMETER4FVEXTPROC>("glNamedProgramLocalParameter4fvEXT")),
	 glGetNamedProgramLocalParameterdvEXTProc(GLExtensionManager::getFunction<PFNGLGETNAMEDPROGRAMLOCALPARAMETERDVEXTPROC>("glGetNamedProgramLocalParameterdvEXT")),
	 glGetNamedProgramLocalParameterfvEXTProc(GLExtensionManager::getFunction<PFNGLGETNAMEDPROGRAMLOCALPARAMETERFVEXTPROC>("glGetNamedProgramLocalParameterfvEXT")),
	 glGetNamedProgramivEXTProc(GLExtensionManager::getFunction<PFNGLGETNAMEDPROGRAMIVEXTPROC>("glGetNamedProgramivEXT")),
	 glGetNamedProgramStringEXTProc(GLExtensionManager::getFunction<PFNGLGETNAMEDPROGRAMSTRINGEXTPROC>("glGetNamedProgramStringEXT")),
	 glNamedRenderbufferStorageEXTProc(GLExtensionManager::getFunction<PFNGLNAMEDRENDERBUFFERSTORAGEEXTPROC>("glNamedRenderbufferStorageEXT")),
	 glGetNamedRenderbufferParameterivEXTProc(GLExtensionManager::getFunction<PFNGLGETNAMEDRENDERBUFFERPARAMETERIVEXTPROC>("glGetNamedRenderbufferParameterivEXT")),
	 glNamedRenderbufferStorageMultisampleEXTProc(GLExtensionManager::getFunction<PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC>("glNamedRenderbufferStorageMultisampleEXT")),
	 glNamedRenderbufferStorageMultisampleCoverageEXTProc(GLExtensionManager::getFunction<PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLECOVERAGEEXTPROC>("glNamedRenderbufferStorageMultisampleCoverageEXT")),
	 glCheckNamedFramebufferStatusEXTProc(GLExtensionManager::getFunction<PFNGLCHECKNAMEDFRAMEBUFFERSTATUSEXTPROC>("glCheckNamedFramebufferStatusEXT")),
	 glNamedFramebufferTexture1DEXTProc(GLExtensionManager::getFunction<PFNGLNAMEDFRAMEBUFFERTEXTURE1DEXTPROC>("glNamedFramebufferTexture1DEXT")),
	 glNamedFramebufferTexture2DEXTProc(GLExtensionManager::getFunction<PFNGLNAMEDFRAMEBUFFERTEXTURE2DEXTPROC>("glNamedFramebufferTexture2DEXT")),
	 glNamedFramebufferTexture3DEXTProc(GLExtensionManager::getFunction<PFNGLNAMEDFRAMEBUFFERTEXTURE3DEXTPROC>("glNamedFramebufferTexture3DEXT")),
	 glNamedFramebufferRenderbufferEXTProc(GLExtensionManager::getFunction<PFNGLNAMEDFRAMEBUFFERRENDERBUFFEREXTPROC>("glNamedFramebufferRenderbufferEXT")),
	 glGetNamedFramebufferAttachmentParameterivEXTProc(GLExtensionManager::getFunction<PFNGLGETNAMEDFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC>("glGetNamedFramebufferAttachmentParameterivEXT")),
	 glGenerateTextureMipmapEXTProc(GLExtensionManager::getFunction<PFNGLGENERATETEXTUREMIPMAPEXTPROC>("glGenerateTextureMipmapEXT")),
	 glGenerateMultiTexMipmapEXTProc(GLExtensionManager::getFunction<PFNGLGENERATEMULTITEXMIPMAPEXTPROC>("glGenerateMultiTexMipmapEXT")),
	 glFramebufferDrawBufferEXTProc(GLExtensionManager::getFunction<PFNGLFRAMEBUFFERDRAWBUFFEREXTPROC>("glFramebufferDrawBufferEXT")),
	 glFramebufferDrawBuffersEXTProc(GLExtensionManager::getFunction<PFNGLFRAMEBUFFERDRAWBUFFERSEXTPROC>("glFramebufferDrawBuffersEXT")),
	 glFramebufferReadBufferEXTProc(GLExtensionManager::getFunction<PFNGLFRAMEBUFFERREADBUFFEREXTPROC>("glFramebufferReadBufferEXT")),
	 glGetFramebufferParameterivEXTProc(GLExtensionManager::getFunction<PFNGLGETFRAMEBUFFERPARAMETERIVEXTPROC>("glGetFramebufferParameterivEXT")),
	 glNamedCopyBufferSubDataEXTProc(GLExtensionManager::getFunction<PFNGLNAMEDCOPYBUFFERSUBDATAEXTPROC>("glNamedCopyBufferSubDataEXT")),
	 glNamedFramebufferTextureEXTProc(GLExtensionManager::getFunction<PFNGLNAMEDFRAMEBUFFERTEXTUREEXTPROC>("glNamedFramebufferTextureEXT")),
	 glNamedFramebufferTextureLayerEXTProc(GLExtensionManager::getFunction<PFNGLNAMEDFRAMEBUFFERTEXTURELAYEREXTPROC>("glNamedFramebufferTextureLayerEXT")),
	 glNamedFramebufferTextureFaceEXTProc(GLExtensionManager::getFunction<PFNGLNAMEDFRAMEBUFFERTEXTUREFACEEXTPROC>("glNamedFramebufferTextureFaceEXT")),
	 glTextureRenderbufferEXTProc(GLExtensionManager::getFunction<PFNGLTEXTURERENDERBUFFEREXTPROC>("glTextureRenderbufferEXT")),
	 glMultiTexRenderbufferEXTProc(GLExtensionManager::getFunction<PFNGLMULTITEXRENDERBUFFEREXTPROC>("glMultiTexRenderbufferEXT")),
	 glVertexArrayVertexOffsetEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXARRAYVERTEXOFFSETEXTPROC>("glVertexArrayVertexOffsetEXT")),
	 glVertexArrayColorOffsetEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXARRAYCOLOROFFSETEXTPROC>("glVertexArrayColorOffsetEXT")),
	 glVertexArrayEdgeFlagOffsetEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXARRAYEDGEFLAGOFFSETEXTPROC>("glVertexArrayEdgeFlagOffsetEXT")),
	 glVertexArrayIndexOffsetEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXARRAYINDEXOFFSETEXTPROC>("glVertexArrayIndexOffsetEXT")),
	 glVertexArrayNormalOffsetEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXARRAYNORMALOFFSETEXTPROC>("glVertexArrayNormalOffsetEXT")),
	 glVertexArrayTexCoordOffsetEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXARRAYTEXCOORDOFFSETEXTPROC>("glVertexArrayTexCoordOffsetEXT")),
	 glVertexArrayMultiTexCoordOffsetEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXARRAYMULTITEXCOORDOFFSETEXTPROC>("glVertexArrayMultiTexCoordOffsetEXT")),
	 glVertexArrayFogCoordOffsetEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXARRAYFOGCOORDOFFSETEXTPROC>("glVertexArrayFogCoordOffsetEXT")),
	 glVertexArraySecondaryColorOffsetEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXARRAYSECONDARYCOLOROFFSETEXTPROC>("glVertexArraySecondaryColorOffsetEXT")),
	 glVertexArrayVertexAttribOffsetEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXARRAYVERTEXATTRIBOFFSETEXTPROC>("glVertexArrayVertexAttribOffsetEXT")),
	 glVertexArrayVertexAttribIOffsetEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXARRAYVERTEXATTRIBIOFFSETEXTPROC>("glVertexArrayVertexAttribIOffsetEXT")),
	 glEnableVertexArrayEXTProc(GLExtensionManager::getFunction<PFNGLENABLEVERTEXARRAYEXTPROC>("glEnableVertexArrayEXT")),
	 glDisableVertexArrayEXTProc(GLExtensionManager::getFunction<PFNGLDISABLEVERTEXARRAYEXTPROC>("glDisableVertexArrayEXT")),
	 glEnableVertexArrayAttribEXTProc(GLExtensionManager::getFunction<PFNGLENABLEVERTEXARRAYATTRIBEXTPROC>("glEnableVertexArrayAttribEXT")),
	 glDisableVertexArrayAttribEXTProc(GLExtensionManager::getFunction<PFNGLDISABLEVERTEXARRAYATTRIBEXTPROC>("glDisableVertexArrayAttribEXT")),
	 glGetVertexArrayIntegervEXTProc(GLExtensionManager::getFunction<PFNGLGETVERTEXARRAYINTEGERVEXTPROC>("glGetVertexArrayIntegervEXT")),
	 glGetVertexArrayPointervEXTProc(GLExtensionManager::getFunction<PFNGLGETVERTEXARRAYPOINTERVEXTPROC>("glGetVertexArrayPointervEXT")),
	 glGetVertexArrayIntegeri_vEXTProc(GLExtensionManager::getFunction<PFNGLGETVERTEXARRAYINTEGERI_VEXTPROC>("glGetVertexArrayIntegeri_vEXT")),
	 glGetVertexArrayPointeri_vEXTProc(GLExtensionManager::getFunction<PFNGLGETVERTEXARRAYPOINTERI_VEXTPROC>("glGetVertexArrayPointeri_vEXT")),
	 glMapNamedBufferRangeEXTProc(GLExtensionManager::getFunction<PFNGLMAPNAMEDBUFFERRANGEEXTPROC>("glMapNamedBufferRangeEXT")),
	 glFlushMappedNamedBufferRangeEXTProc(GLExtensionManager::getFunction<PFNGLFLUSHMAPPEDNAMEDBUFFERRANGEEXTPROC>("glFlushMappedNamedBufferRangeEXT")),
	 glNamedBufferStorageEXTProc(GLExtensionManager::getFunction<PFNGLNAMEDBUFFERSTORAGEEXTPROC>("glNamedBufferStorageEXT")),
	 glClearNamedBufferDataEXTProc(GLExtensionManager::getFunction<PFNGLCLEARNAMEDBUFFERDATAEXTPROC>("glClearNamedBufferDataEXT")),
	 glClearNamedBufferSubDataEXTProc(GLExtensionManager::getFunction<PFNGLCLEARNAMEDBUFFERSUBDATAEXTPROC>("glClearNamedBufferSubDataEXT")),
	 glNamedFramebufferParameteriEXTProc(GLExtensionManager::getFunction<PFNGLNAMEDFRAMEBUFFERPARAMETERIEXTPROC>("glNamedFramebufferParameteriEXT")),
	 glGetNamedFramebufferParameterivEXTProc(GLExtensionManager::getFunction<PFNGLGETNAMEDFRAMEBUFFERPARAMETERIVEXTPROC>("glGetNamedFramebufferParameterivEXT")),
	 glProgramUniform1dEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORM1DEXTPROC>("glProgramUniform1dEXT")),
	 glProgramUniform2dEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORM2DEXTPROC>("glProgramUniform2dEXT")),
	 glProgramUniform3dEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORM3DEXTPROC>("glProgramUniform3dEXT")),
	 glProgramUniform4dEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORM4DEXTPROC>("glProgramUniform4dEXT")),
	 glProgramUniform1dvEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORM1DVEXTPROC>("glProgramUniform1dvEXT")),
	 glProgramUniform2dvEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORM2DVEXTPROC>("glProgramUniform2dvEXT")),
	 glProgramUniform3dvEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORM3DVEXTPROC>("glProgramUniform3dvEXT")),
	 glProgramUniform4dvEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORM4DVEXTPROC>("glProgramUniform4dvEXT")),
	 glProgramUniformMatrix2dvEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORMMATRIX2DVEXTPROC>("glProgramUniformMatrix2dvEXT")),
	 glProgramUniformMatrix3dvEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORMMATRIX3DVEXTPROC>("glProgramUniformMatrix3dvEXT")),
	 glProgramUniformMatrix4dvEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORMMATRIX4DVEXTPROC>("glProgramUniformMatrix4dvEXT")),
	 glProgramUniformMatrix2x3dvEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORMMATRIX2X3DVEXTPROC>("glProgramUniformMatrix2x3dvEXT")),
	 glProgramUniformMatrix2x4dvEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORMMATRIX2X4DVEXTPROC>("glProgramUniformMatrix2x4dvEXT")),
	 glProgramUniformMatrix3x2dvEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORMMATRIX3X2DVEXTPROC>("glProgramUniformMatrix3x2dvEXT")),
	 glProgramUniformMatrix3x4dvEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORMMATRIX3X4DVEXTPROC>("glProgramUniformMatrix3x4dvEXT")),
	 glProgramUniformMatrix4x2dvEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORMMATRIX4X2DVEXTPROC>("glProgramUniformMatrix4x2dvEXT")),
	 glProgramUniformMatrix4x3dvEXTProc(GLExtensionManager::getFunction<PFNGLPROGRAMUNIFORMMATRIX4X3DVEXTPROC>("glProgramUniformMatrix4x3dvEXT")),
	 glTextureBufferRangeEXTProc(GLExtensionManager::getFunction<PFNGLTEXTUREBUFFERRANGEEXTPROC>("glTextureBufferRangeEXT")),
	 glTextureStorage1DEXTProc(GLExtensionManager::getFunction<PFNGLTEXTURESTORAGE1DEXTPROC>("glTextureStorage1DEXT")),
	 glTextureStorage2DEXTProc(GLExtensionManager::getFunction<PFNGLTEXTURESTORAGE2DEXTPROC>("glTextureStorage2DEXT")),
	 glTextureStorage3DEXTProc(GLExtensionManager::getFunction<PFNGLTEXTURESTORAGE3DEXTPROC>("glTextureStorage3DEXT")),
	 glTextureStorage2DMultisampleEXTProc(GLExtensionManager::getFunction<PFNGLTEXTURESTORAGE2DMULTISAMPLEEXTPROC>("glTextureStorage2DMultisampleEXT")),
	 glTextureStorage3DMultisampleEXTProc(GLExtensionManager::getFunction<PFNGLTEXTURESTORAGE3DMULTISAMPLEEXTPROC>("glTextureStorage3DMultisampleEXT")),
	 glVertexArrayBindVertexBufferEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXARRAYBINDVERTEXBUFFEREXTPROC>("glVertexArrayBindVertexBufferEXT")),
	 glVertexArrayVertexAttribFormatEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXARRAYVERTEXATTRIBFORMATEXTPROC>("glVertexArrayVertexAttribFormatEXT")),
	 glVertexArrayVertexAttribIFormatEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXARRAYVERTEXATTRIBIFORMATEXTPROC>("glVertexArrayVertexAttribIFormatEXT")),
	 glVertexArrayVertexAttribLFormatEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXARRAYVERTEXATTRIBLFORMATEXTPROC>("glVertexArrayVertexAttribLFormatEXT")),
	 glVertexArrayVertexAttribBindingEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXARRAYVERTEXATTRIBBINDINGEXTPROC>("glVertexArrayVertexAttribBindingEXT")),
	 glVertexArrayVertexBindingDivisorEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXARRAYVERTEXBINDINGDIVISOREXTPROC>("glVertexArrayVertexBindingDivisorEXT")),
	 glVertexArrayVertexAttribLOffsetEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXARRAYVERTEXATTRIBLOFFSETEXTPROC>("glVertexArrayVertexAttribLOffsetEXT")),
	 glTexturePageCommitmentEXTProc(GLExtensionManager::getFunction<PFNGLTEXTUREPAGECOMMITMENTEXTPROC>("glTexturePageCommitmentEXT")),
	 glVertexArrayVertexAttribDivisorEXTProc(GLExtensionManager::getFunction<PFNGLVERTEXARRAYVERTEXATTRIBDIVISOREXTPROC>("glVertexArrayVertexAttribDivisorEXT"))
	{
	}

GLEXTDirectStateAccess::~GLEXTDirectStateAccess(void)
	{
	}

const char* GLEXTDirectStateAccess::getExtensionName(void) const
	{
	return name;
	}

void GLEXTDirectStateAccess::activate(void)
	{
	current=this;
	}

void GLEXTDirectStateAccess::deactivate(void)
	{
	current=0;
	}

bool GLEXTDirectStateAccess::isSupported(void)
	{
	/* Ask the current extension manager whether the extension is supported in the current OpenGL context: */
	return GLExtensionManager::isExtensionSupported(name);
	}

void GLEXTDirectStateAccess::initExtension(void)
	{
	/* Check if the extension is already initialized: */
	if(!GLExtensionManager::isExtensionRegistered(name))
		{
		/* Create a new extension object: */
		GLEXTDirectStateAccess* newExtension=new GLEXTDirectStateAccess;
		
		/* Register the extension with the current extension manager: */
		GLExtensionManager::registerExtension(newExtension);
		}
	}
