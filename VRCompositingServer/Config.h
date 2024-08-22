/***********************************************************************
Config.h - Configuration header for Vrui VR Compositing Server.
Copyright (c) 2022-2024 Oliver Kreylos

This file is part of the Vrui VR Compositing Server (VRCompositor).

The Vrui VR Compositing Server is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Vrui VR Compositing Server is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Vrui VR Compositing Server; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef CONFIG_INCLUDED
#define CONFIG_INCLUDED

#define VRCOMPOSITOR_USE_LATENCYTESTER 0
#define VRCOMPOSITOR_SHAREDMEMORY_NAME "/VRCompositingServer.shmem"
#define VRCOMPOSITOR_SHADERDIR "/home/okreylos/Projects/Vulkan/spirv"

#define VRSERVER_APPNAME "VRCompositingServer"
#define VRSERVER_APPVERSION Vulkan::APIVersion(1,0)
#define VRSERVER_ENGINENAME "Vrui"
#define VRSERVER_ENGINEVERSION Vulkan::APIVersion(11,0,1)
#define VRSERVER_SOCKET_NAME "VRCompositingServer.socket"
#define VRSERVER_SOCKET_ABSTRACT true

#define VRDEVICEDAEMON_SOCKET_NAME "VRDeviceDaemon.socket"
#define VRDEVICEDAEMON_SOCKET_ABSTRACT true
#define VRSERVER_DEFAULT_HMD "Valve Index"
#define VRSERVER_DEFAULT_HZ 90.0

#endif
