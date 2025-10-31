########################################################################
# Makefile for Vrui toolkit and its underlying libraries.
# Copyright (c) 1998-2025 Oliver Kreylos
#
# This file is part of the WhyTools Build Environment.
# 
# The WhyTools Build Environment is free software; you can redistribute
# it and/or modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.
# 
# The WhyTools Build Environment is distributed in the hope that it will
# be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with the WhyTools Build Environment; if not, write to the Free
# Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
# 02111-1307 USA
########################################################################

########################################################################
# The root directory where Vrui will be installed when running
# "make install". This must be a different directory than the one that
# contains this makefile, i.e., Vrui cannot be built in-tree. If the
# installation directory is changed from the default, the makefiles of
# Vrui applications typically need to be changed to use the same
# installation directory.
########################################################################

INSTALLDIR := /usr/local

########################################################################
# Please do not change the following lines
########################################################################

# Define the root of the toolkit source tree and the build system
# directory
VRUI_PACKAGEROOT := $(shell pwd)
VRUI_MAKEDIR := $(VRUI_PACKAGEROOT)/BuildRoot
VRUI_ETCDIR := $(VRUI_PACKAGEROOT)/etc
VRUI_SHAREDIR := $(VRUI_PACKAGEROOT)/share
VRUI_SCRIPTDIR := $(VRUI_PACKAGEROOT)/scripts
VRUI_DOCDIR := $(VRUI_PACKAGEROOT)/Documentation

# Include definitions for the system environment and provided software
# packages
include $(VRUI_MAKEDIR)/SystemDefinitions
include $(VRUI_MAKEDIR)/Packages.System

########################################################################
# Some settings that might need adjustment. In general, do not bother
# with these unless something goes wrong during the build process, or
# you have very specific requirements. Proceed with caution!
# The settings below are conservative; see the README file for what they
# mean, and how they depend on capabilities of the host system.
########################################################################

# The build system attempts to auto-detect the presence of system
# libraries that provide optional features. If autodetection fails and
# the build process generates error messages related to missing include
# files during compiling or missing libraries during linking, the use
# of these optional libraries can be disabled manually by uncommenting
# any of the following lines.

# SYSTEM_HAVE_LIBUDEV = 0
# SYSTEM_HAVE_LIBDBUS = 0
# SYSTEM_HAVE_LIBUSB1 = 0
# SYSTEM_HAVE_OPENSSL = 0
# SYSTEM_HAVE_LIBPNG = 0
# SYSTEM_HAVE_LIBJPEG = 0
# SYSTEM_HAVE_LIBTIFF = 0
# SYSTEM_HAVE_ALSA = 0
# SYSTEM_HAVE_PULSEAUDIO = 0
# SYSTEM_HAVE_OPENAL = 0
# SYSTEM_HAVE_V4L2 = 0
# SYSTEM_HAVE_BLUETOOTH = 0
# SYSTEM_HAVE_DC1394 = 0
# SYSTEM_HAVE_SPEEX = 0
# SYSTEM_HAVE_THEORA = 0
# SYSTEM_HAVE_FREETYPE = 0
# SYSTEM_HAVE_XRANDR = 0
# SYSTEM_HAVE_XINPUT2 = 0
# SYSTEM_HAVE_VULKAN = 0

# The build system attempts to auto-detect optional features in system
# libraries. If autodetection fails and the build process generates
# error messages related to undeclared functions, the use of these
# optional features can be disabled manually by uncommenting any of the
# following lines:

# Presense of libusb_get_parent, libusb_strerror, and libusb_set_option in libusb.h:
# LIBUSB1_HAS_TOPOLOGY_CALLS = 0
# LIBUSB1_HAS_STRERROR = 0
# LIBUSB1_HAS_SET_OPTION = 0

# Location of GNU versions of Helvetica/Times New Roman/New Courier
# TrueType fonts:
SCENEGRAPH_USE_FANCYTEXT = 0
ifneq ($(SYSTEM_HAVE_FREETYPE),0)
  SYSTEM_COREFONTDIR = $(sort $(dir $(shell find /usr/share/fonts -name FreeSans.ttf -or -name FreeSerif.ttf -or -name FreeMono.ttf)))
  ifeq ($(words $(SYSTEM_COREFONTDIR)),1)
    SCENEGRAPH_USE_FANCYTEXT = 1
  endif
endif

########################################################################
# Select support for commodity VR headsets via the OpenVR API
########################################################################

# Root directory of the SteamVR run-time.
STEAMVRDIR = 

# If no SteamVR directory is defined above, or given on make's command
# line, search the file system for an installation:
ifeq ($(strip $(STEAMVRDIR)),)
  STEAMVRDIR = $(realpath $(firstword $(wildcard $(HOME)/.[Ss]team/[Ss]team/[Ss]team[Aa]pps/[Cc]ommon/[Ss]team[Vv][Rr] $(HOME)/.local/share/[Ss]team/[Ss]team[Aa]pps/[Cc]ommon/[Ss]team[Vv][Rr])))
  ifeq ($(strip $(STEAMVRDIR)),)
    STEAMVRDIR = $(firstword $(shell locate -l 1 -i "*/common/SteamVR"))
  endif
endif

ifneq ($(strip $(STEAMVRDIR)),)
  # Build OpenVRHost VRDeviceDaemon driver module:
  SYSTEM_HAVE_OPENVR = 1
  
  # Root directory of the OpenVR SDK
  # (The single required header file, openvr_driver.h, is now included in
  # Vrui package as contributed source.)
  OPENVR_BASEDIR = $(PWD)/Contributed/OpenVR
  
  # Try looking for the Steam run-time three levels up from the SteamVR
  # directory first:
  STEAMDIR = $(realpath $(STEAMVRDIR)/../../..)
  STEAMRUNTIMEDIR1 = $(firstword $(wildcard $(STEAMDIR)/ubuntu12_32/steam-runtime/amd64/lib/x86_64-linux-gnu))
  
  # If the run-time wasn't found there, look four levels up from the
  # SteamVR directory:
  ifeq ($(strip $(STEAMRUNTIMEDIR1)),)
    STEAMDIR = $(realpath $(STEAMVRDIR)/../../../..)
    STEAMRUNTIMEDIR1 = $(firstword $(wildcard $(STEAMDIR)/ubuntu12_32/steam-runtime/amd64/lib/x86_64-linux-gnu))
  endif
  
  # If the run-time still wasn't found, disable Vive support:
  ifeq ($(strip $(STEAMRUNTIMEDIR1)),)
    SYSTEM_HAVE_OPENVR = 0
  endif
  
  # Set the secondary Steam run-time directory:
  STEAMRUNTIMEDIR2 = $(realpath $(STEAMRUNTIMEDIR1)/../../usr/lib/x86_64-linux-gnu)
  
  # SteamVR Lighthouse driver directory:
  STEAMVRDRIVERDIR = $(STEAMVRDIR)/drivers/lighthouse/bin/linux64
  
  # SteamVR render model resource directory:
  STEAMVRRENDERMODELSDIR = $(STEAMVRDIR)/resources/rendermodels
  
  # Unfortunately Index controller render models are somewhere else:
  STEAMVRINDEXCONTROLLERRENDERMODELSDIR = $(STEAMVRDIR)/drivers/indexcontroller/resources/rendermodels
else
  SYSTEM_HAVE_OPENVR = 0
endif

########################################################################
# Please do not change the following line
########################################################################

# Include definitions for Vrui-provided software packages
include $(VRUI_MAKEDIR)/Packages.Vrui

########################################################################
# More settings that might need adjustment. In general, do not bother
# with these unless something goes wrong during the build process, or
# you have very specific requirements. Proceed with caution!
# The settings below are conservative; see the README file for what they
# mean, and how they depend on capabilities of the host system.
########################################################################

# Set this to 1 if Vrui executables and shared libraries shall contain
# links to any shared libraries they link to. This will relieve a user
# from having to set up a dynamic library search path.
USE_RPATH = 1

# Set the following flag to 1 if the GL support library shall contain
# support for multithreaded rendering to several windows. This is
# required for shared-memory multi-pipe rendering systems such as SGI
# Onyx or Prism, but somewhat reduces performance when accessing per-
# window data elements or OpenGL extensions. If the compiler and run-
# time environment do not support a direct mechanism for thread-local
# storage (such as gcc's __thread extension), this uses the even slower
# POSIX thread-local storage mechanism. If setting this to 1 causes
# compilation errors, the SYSTEM_HAVE_TLS variable in
# BuildRoot/SystemDefinitions needs to be set to 0.
GLSUPPORT_USE_TLS = 0

# Set this to 1 if the Linux input.h header file has the required
# structure definitions (usually on newer Linux versions). If this is
# set wrongly, Vrui/Internal/Linux/InputDeviceAdapterHID.cpp and
# VRDeviceDaemon/VRDevices/HIDDevice.cpp will generate compiler errors.
# This setting is ignored on MacOS X.
LINUX_INPUT_H_HAS_STRUCTS = 1

# Set this to 1 if the VRWindow class shall be compiled with support for
# swap locks and swap groups (NVidia extension). This is only necessary
# in very rare cases; if you don't already know you need it, leave this
# setting at 0.
# If this is set to 1 and the NVidia extension is not there,
# VRWindow.cpp will generate compiler errors.
VRUI_VRWINDOW_USE_SWAPGROUPS = 0

# Set this to 1 if the operating system supports the input abstraction
# layer. If this is set to 1 and the input abstraction is not supported,
# Joystick.cpp will generate compiler errors.
VRDEVICES_USE_INPUT_ABSTRACTION = 0

# Set this to 1 if the Vrui VR device driver shall support Nintendo Wii
# controllers using the bluez user-level Bluetooth library. This
# requires that bluez is installed on the host computer, and that the
# path to the bluez header files / libraries is set properly in
# $(VRUI_MAKEDIR)/Packages.
# For now, the following code tries to automatically determine whether
# bluez is supported. This might or might not work.
VRDEVICES_USE_BLUETOOTH = $(SYSTEM_HAVE_BLUETOOTH)

########################################################################
# Please do not change anything below this line
########################################################################

PROJECT_NAME = Vrui

# Specify version of created dynamic shared libraries
PROJECT_MAJOR = 14
PROJECT_MINOR = 0
PROJECT_BUILD = 1
PROJECT_NUMERICVERSION = 14000001

# Override the include file and library search directories:
VRUI_INCLUDEDIR = $(PROJECT_ROOT)
VRUI_LIBDIR = $(PROJECT_ROOT)/$(MYLIBEXT)
VRUI_RPATH = $(LIBINSTALLDIR)

########################################################################
# Create the full set of installation paths, with defaults for common
# setups
########################################################################

ifdef SYSTEMINSTALL # Configuration to install Vrui in a non-local system directory
  HEADERINSTALLDIR = $(INSTALLDIR)/usr/$(INCLUDEEXT)/$(PROJECT_FULLNAME)
  LIBINSTALLDIR_DEBUG = $(INSTALLDIR)/usr/$(LIBEXT)/$(PROJECT_FULLNAME)/debug
  LIBINSTALLDIR_RELEASE = $(INSTALLDIR)/usr/$(LIBEXT)/$(PROJECT_FULLNAME)
  EXECUTABLEINSTALLDIR_DEBUG = $(INSTALLDIR)/usr/$(BINEXT)/debug
  EXECUTABLEINSTALLDIR_RELEASE = $(INSTALLDIR)/usr/$(BINEXT)
  ETCINSTALLDIR = $(INSTALLDIR)/etc/$(PROJECT_FULLNAME)
  SHAREINSTALLDIR = $(INSTALLDIR)/usr/share/$(PROJECT_FULLNAME)
  PKGCONFIGINSTALLDIR = $(INSTALLDIR)/usr/$(LIBEXT)/pkgconfig
  DOCINSTALLDIR = $(INSTALLDIR)/usr/share/doc/$(PROJECT_FULLNAME)
  UDEVRULEDIR = /usr/lib/udev/rules.d
  INSTALLROOT = /usr
else # Configuration for regular installation
  ifeq ($(findstring $(PROJECT_FULLNAME),$(INSTALLDIR)),$(PROJECT_FULLNAME))
    INSTALLSHIM = 
  else
    INSTALLSHIM = /$(PROJECT_FULLNAME)
  endif
  HEADERINSTALLDIR = $(INSTALLDIR)/$(INCLUDEEXT)$(INSTALLSHIM)
  LIBINSTALLDIR_DEBUG = $(INSTALLDIR)/$(LIBEXT)$(INSTALLSHIM)/debug
  LIBINSTALLDIR_RELEASE = $(INSTALLDIR)/$(LIBEXT)$(INSTALLSHIM)
  EXECUTABLEINSTALLDIR_DEBUG = $(INSTALLDIR)/$(BINEXT)/debug
  EXECUTABLEINSTALLDIR_RELEASE = $(INSTALLDIR)/$(BINEXT)
  ETCINSTALLDIR = $(INSTALLDIR)/etc$(INSTALLSHIM)
  SHAREINSTALLDIR = $(INSTALLDIR)/share$(INSTALLSHIM)
  PKGCONFIGINSTALLDIR = $(INSTALLDIR)/$(LIBEXT)/pkgconfig
  DOCINSTALLDIR = $(INSTALLDIR)/share/doc$(INSTALLSHIM)
  UDEVRULEDIR = /etc/udev/rules.d
  INSTALLROOT = $(INSTALLDIR)
endif
MAKEINSTALLDIR = $(SHAREINSTALLDIR)/make
PLUGININSTALLDIR_DEBUG = $(LIBINSTALLDIR_DEBUG)
PLUGININSTALLDIR_RELEASE = $(LIBINSTALLDIR_RELEASE)

# Select debug or release versions:
ifdef DEBUG
  LIBINSTALLDIR = $(LIBINSTALLDIR_DEBUG)
  EXECUTABLEINSTALLDIR = $(EXECUTABLEINSTALLDIR_DEBUG)
  PLUGININSTALLDIR = $(PLUGININSTALLDIR_DEBUG)
else
  LIBINSTALLDIR = $(LIBINSTALLDIR_RELEASE)
  EXECUTABLEINSTALLDIR = $(EXECUTABLEINSTALLDIR_RELEASE)
  PLUGININSTALLDIR = $(PLUGININSTALLDIR_RELEASE)
endif

# Specify the location of the per-user global configuration file:
ifeq ($(findstring $(HOME),$(INSTALLDIR)),$(HOME))
# Vrui is installed in user's home directory, no need for per-user config file
  VRUI_READUSERCONFIGFILE = 0
else
# Vrui is installed outside user's home directory
  VRUI_READUSERCONFIGFILE = 1
endif
ifeq ($(HOST_OS),Darwin)
  VRUI_USERCONFIGDIR = Library/Preferences/$(PROJECT_FULLNAME)
else
  VRUI_USERCONFIGDIR = .config/$(PROJECT_FULLNAME)
endif

########################################################################
# Specify additional compiler and linker flags
########################################################################

########################################################################
# List packages used by this project
# (Supported packages can be found in ./BuildRoot/Packages)
########################################################################

PACKAGES = 

########################################################################
# Specify all final targets
########################################################################

BUILDUTILS = 
CONFIGFILES = 
LIBRARIES = 
PLUGINS = 
EXECUTABLES = 
RESOURCES = 

#
# The build utilities:
#

BUILDUTILS += $(VRUI_MAKEDIR)/StripPackages

#
# The basic libraries:
#

LIBRARY_NAMES    = libMisc \
                   libRealtime \
                   libThreads
ifneq ($(SYSTEM_HAVE_LIBUSB1),0)
  LIBRARY_NAMES += libUSB
endif
ifneq ($(SYSTEM_HAVE_LIBUDEV),0)
  LIBRARY_NAMES += libRawHID
endif
LIBRARY_NAMES   += libIO \
                   libPlugins \
                   libComm \
                   libCluster \
                   libMath \
                   libGeometry \
                   libGLWrappers \
                   libGLSupport \
                   libGLXSupport \
                   libGLGeometry
ifneq ($(SYSTEM_HAVE_VULKAN),0)
  LIBRARY_NAMES += libVulkan \
                   libVulkanXlib
endif
LIBRARY_NAMES   += libImages \
                   libGLMotif \
                   libSound \
                   libVideo \
                   libALSupport \
                   libSceneGraph \
                   libVrui \
                   libVRDeviceDaemon

LIBRARIES += $(LIBRARY_NAMES:%=$(call LIBRARYNAME,%))

#
# The Vrui VR tool plug-in hierarchy:
#

# Don't build the following tool modules because they are unfinished or experimental:
VRTOOLS_IGNORE_SOURCES = Vrui/Tools/FPSArmTool.cpp

VRTOOLS_SOURCES = $(filter-out $(VRTOOLS_IGNORE_SOURCES),$(wildcard Vrui/Tools/*.cpp))

VRTOOLSDIREXT = VRTools
VRTOOLSDIR= $(PROJECT_LIBDIR)/$(VRTOOLSDIREXT)
VRTOOLNAMES = $(1:%=$(VRTOOLSDIR)/lib%.$(PLUGINFILEEXT))
VRTOOLS = $(call VRTOOLNAMES,$(VRTOOLS_SOURCES:Vrui/Tools/%.cpp=%))
PLUGINS += $(VRTOOLS)

#
# The Vrui vislet plug-in hierarchy:
#

# Don't build the following vislet modules unless explicitly asked later:
VRVISLETS_IGNORE_SOURCES = Vrui/Vislets/LatencyTester.cpp

VRVISLETS_SOURCES = $(filter-out $(VRVISLETS_IGNORE_SOURCES),$(wildcard Vrui/Vislets/*.cpp))
ifneq ($(SYSTEM_HAVE_LIBUDEV),0)
  VRVISLETS_SOURCES += Vrui/Vislets/LatencyTester.cpp
endif

VRVISLETSDIREXT = VRVislets
VRVISLETSDIR = $(PROJECT_LIBDIR)/$(VRVISLETSDIREXT)
VRVISLETNAMES = $(1:%=$(VRVISLETSDIR)/lib%.$(PLUGINFILEEXT))
VRVISLETS = $(call VRVISLETNAMES,$(VRVISLETS_SOURCES:Vrui/Vislets/%.cpp=%))
PLUGINS += $(VRVISLETS)

#
# The VR device driver daemon:
#

EXECUTABLES += $(EXEDIR)/VRDeviceDaemon

#
# The VR device driver plug-ins:
#

# Don't build the following device modules unless explicitly asked later:
VRDEVICES_IGNORE_SOURCES = VRDeviceDaemon/VRDevices/Joystick.cpp \
                           VRDeviceDaemon/VRDevices/VRPNConnection.cpp \
                           VRDeviceDaemon/VRDevices/Wiimote.cpp \
                           VRDeviceDaemon/VRDevices/WiimoteTracker.cpp \
                           VRDeviceDaemon/VRDevices/RazerHydra.cpp \
                           VRDeviceDaemon/VRDevices/RazerHydraDevice.cpp \
                           VRDeviceDaemon/VRDevices/OculusRift.cpp \
                           VRDeviceDaemon/VRDevices/OpenVRHost.cpp

VRDEVICES_SOURCES = $(filter-out $(VRDEVICES_IGNORE_SOURCES),$(wildcard VRDeviceDaemon/VRDevices/*.cpp))
ifneq ($(VRDEVICES_USE_INPUT_ABSTRACTION),0)
  VRDEVICES_SOURCES += VRDeviceDaemon/VRDevices/Joystick.cpp
endif
ifneq ($(VRDEVICES_USE_BLUETOOTH),0)
  VRDEVICES_SOURCES += VRDeviceDaemon/VRDevices/WiimoteTracker.cpp
endif
ifneq ($(SYSTEM_HAVE_LIBUSB1),0)
  VRDEVICES_SOURCES += VRDeviceDaemon/VRDevices/RazerHydraDevice.cpp \
                       VRDeviceDaemon/VRDevices/OculusRift.cpp
endif
ifneq ($(SYSTEM_HAVE_OPENVR),0)
  VRDEVICES_SOURCES += VRDeviceDaemon/VRDevices/OpenVRHost.cpp
endif

VRDEVICESDIREXT = VRDevices
VRDEVICESDIR = $(PROJECT_LIBDIR)/$(VRDEVICESDIREXT)
VRDEVICENAMES = $(1:%=$(VRDEVICESDIR)/lib%.$(PLUGINFILEEXT))
VRDEVICES = $(call VRDEVICENAMES,$(VRDEVICES_SOURCES:VRDeviceDaemon/VRDevices/%.cpp=%))
PLUGINS += $(VRDEVICES)

#
# The VR tracker calibrator plug-ins:
#

VRCALIBRATORS_SOURCES = $(wildcard VRDeviceDaemon/VRCalibrators/*.cpp)

VRCALIBRATORSDIREXT = VRCalibrators
VRCALIBRATORSDIR = $(PROJECT_LIBDIR)/$(VRCALIBRATORSDIREXT)
VRCALIBRATORNAMES = $(1:%=$(VRCALIBRATORSDIR)/lib%.$(PLUGINFILEEXT))
VRCALIBRATORS = $(call VRCALIBRATORNAMES,$(VRCALIBRATORS_SOURCES:VRDeviceDaemon/VRCalibrators/%.cpp=%))
PLUGINS += $(VRCALIBRATORS)

#
# A helper script to run the OpenVRHost device driver module:
#

ifneq ($(SYSTEM_HAVE_OPENVR),0)
  EXECUTABLES += $(EXEDIR)/RunOpenVRTracker.sh
endif

#
# The Vrui VR Compositing Server:
#

ifneq ($(SYSTEM_HAVE_VULKAN),0)
  EXECUTABLES += $(EXEDIR)/VRCompositingServer \
                 $(EXEDIR)/RunVRCompositor.sh
  
  VRCOMPOSITINGSERVER_SHADERDIR = VRCompositingServer/Shaders
  
  VRCOMPOSITINGSERVER_SHADERS = BoundaryRenderer \
                                DistortionCorrection
  
  SPIRVDIR = $(VRUI_SHAREDIR)/spirv
  SPIRVVERTEXDIR = $(VRUI_SHAREDIR)/spirv/vertex
  SPIRVFRAGMENTDIR = $(VRUI_SHAREDIR)/spirv/fragment
  
  RESOURCES += $(VRCOMPOSITINGSERVER_SHADERS:%=$(SPIRVVERTEXDIR)/%.spv) \
               $(VRCOMPOSITINGSERVER_SHADERS:%=$(SPIRVFRAGMENTDIR)/%.spv)
endif

#
# The Vrui device driver test programs (textual and visual):
#

EXECUTABLES += $(EXEDIR)/DeviceTest \
               $(EXEDIR)/TrackingTest

#
# A utility to find connected HMDs:
#

ifneq ($(SYSTEM_HAVE_LIBUSB1),0)
  ifneq ($(SYSTEM_HAVE_XRANDR),0)
    EXECUTABLES += $(EXEDIR)/FindHMD
  endif
endif

#
# The Vrui environment setup program:
#

EXECUTABLES += $(EXEDIR)/RoomSetup

#
# The Vrui sound configuration and test programs:
#

EXECUTABLES += $(EXEDIR)/VruiSoundConfig \
               $(EXEDIR)/VruiSoundConfigTest

#
# A helper script to run Vrui applications on an OpenVR-supported HMD:
#

EXECUTABLES += $(EXEDIR)/OnHMD

#
# The Vrui eye calibration program:
#

EXECUTABLES += $(EXEDIR)/EyeCalibrator

#
# The input device data file dumping program:
#

EXECUTABLES += $(EXEDIR)/PrintInputDeviceDataFile

#
# The Vrui calibration utilities:
#

EXECUTABLES += $(EXEDIR)/TransformCalculator \
               $(EXEDIR)/XBackground \
               $(EXEDIR)/MeasureEnvironment \
               $(EXEDIR)/ScreenCalibrator \
               $(EXEDIR)/AlignTrackingMarkers \
               $(EXEDIR)/AlignPoints \
               $(EXEDIR)/MeasurePoints \
               $(EXEDIR)/SampleTrackerField \
               $(EXEDIR)/CalibrateTouchscreen
ifneq ($(SYSTEM_HAVE_LIBUSB1),0)
  EXECUTABLES += $(EXEDIR)/OculusCalibrator
endif

#
# Generated resources:
#

RESOURCES += $(VRUI_SHAREDIR)/Resources/ViveController.wrl \
             $(VRUI_SHAREDIR)/Resources/IndexControllerLeft.wrl \
             $(VRUI_SHAREDIR)/Resources/IndexControllerRight.wrl

# Set the name of the makefile fragment and make configuration file:
ifdef DEBUG
  MAKEFILEFRAGMENT = $(VRUI_SCRIPTDIR)/Vrui.debug.makeinclude
else
  MAKEFILEFRAGMENT = $(VRUI_SCRIPTDIR)/Vrui.makeinclude
endif
MAKECONFIGFILE = $(VRUI_MAKEDIR)/Configuration.Vrui

# Set the name of the pkg-config meta data file:
PKGCONFIGFILE = $(VRUI_SCRIPTDIR)/Vrui.pc

# Set the names of library configuration files:
CONFIGFILES += Realtime/Config.h \
               Threads/Config.h \
               USB/Config.h \
               Comm/Config.h \
               GL/Config.h \
               Images/Config.h \
               GLMotif/Config.h \
               Sound/Config.h \
               Video/Config.h \
               AL/Config.h \
               SceneGraph/Config.h SceneGraph/Internal/Config.h \
               Vrui/Internal/Config.h \
               VRDeviceDaemon/Config.h \
               VRDeviceDaemon/VRDevices/OpenVRHost-Config.h \
               VRCompositingServer/Config.h

# Set the name of the template makefile:
TEMPLATEMAKEFILE = $(VRUI_MAKEDIR)/makefile
CONFIGFILES += $(TEMPLATEMAKEFILE)

# Remember other files generated by "make config":
CONFIGFILES += ExamplePrograms/makefile \
               ExamplePrograms/MeshEditor/makefile \
               ExamplePrograms/VRMLViewer/makefile

# Remember the names of all generated files for "make clean":
ALL = $(BUILDUTILS) $(LIBRARIES) $(EXECUTABLES) $(PLUGINS) $(RESOURCES)

.PHONY: all
all: $(ALL)

# Build all libraries before any plug-ins or executables:
$(PLUGINS): | $(LIBRARIES)
$(EXECUTABLES): | $(LIBRARIES)

########################################################################
# Build build system utilities
########################################################################

$(VRUI_MAKEDIR)/StripPackages: $(VRUI_MAKEDIR)/StripPackages.c
	@echo Building build utilities...
	@$(PLAINCCOMP) -o $@ $^

########################################################################
# Pseudo-target to print configuration options and configure libraries
########################################################################

.PHONY: config config-invalidate
config: config-invalidate $(DEPDIR)/config

config-invalidate:
	@mkdir -p $(DEPDIR)
	@touch $(DEPDIR)/Configure-Begin

$(DEPDIR)/Configure-Begin: | $(VRUI_MAKEDIR)/StripPackages
	@mkdir -p $(DEPDIR)
	@touch $(DEPDIR)/Configure-Begin

$(DEPDIR)/Configure-Package: $(DEPDIR)/Configure-Begin
	@echo "---- Vrui configuration options: ----"
ifneq ($(USE_RPATH),0)
	@echo "Run-time library search paths enabled"
else
	@echo "Run-time library search paths disabled"
endif
	@touch $(DEPDIR)/Configure-Package

$(DEPDIR)/Configure-Install: $(DEPDIR)/Configure-Realtime \
                             $(DEPDIR)/Configure-Threads \
                             $(DEPDIR)/Configure-USB \
                             $(DEPDIR)/Configure-Comm \
                             $(DEPDIR)/Configure-GLSupport \
                             $(DEPDIR)/Configure-Vulkan \
                             $(DEPDIR)/Configure-Images \
                             $(DEPDIR)/Configure-GLMotif \
                             $(DEPDIR)/Configure-Sound \
                             $(DEPDIR)/Configure-ALSupport \
                             $(DEPDIR)/Configure-Video \
                             $(DEPDIR)/Configure-SceneGraph \
                             $(DEPDIR)/Configure-Vrui \
                             $(DEPDIR)/Configure-VRDeviceDaemon \
                             $(DEPDIR)/Configure-VRCompositingServer
	@echo "---- Vrui installation configuration ----"
ifdef SYSTEMINSTALL
	@echo "System-level installation requested"
endif
	@echo "Root installation directory               : $(INSTALLDIR)"
	@echo "Header installation root directory        : $(HEADERINSTALLDIR)"
	@echo "Library installation root directory       : $(LIBINSTALLDIR)"
	@echo "Executable installation directory         : $(EXECUTABLEINSTALLDIR)"
	@echo "Plug-in installation root directory       : $(PLUGININSTALLDIR)"
	@echo "Configuration file installation directory : $(ETCINSTALLDIR)"
	@echo "Shared file installation root directory   : $(SHAREINSTALLDIR)"
	@echo "Makefile fragment installation directory  : $(SHAREINSTALLDIR)"
	@echo "Build system installation directory       : $(MAKEINSTALLDIR)"
	@echo "pkg-config metafile installation directory: $(PKGCONFIGINSTALLDIR)"
	@echo "Documentation installation directory      : $(DOCINSTALLDIR)"
	@touch $(DEPDIR)/Configure-Install

$(DEPDIR)/Configure-End: $(DEPDIR)/Configure-Makefiles
	@echo "---- End of Vrui configuration options ----"
	@touch $(DEPDIR)/Configure-End

$(DEPDIR)/config: $(DEPDIR)/Configure-End
	@touch $(DEPDIR)/config

########################################################################
# Specify other actions to be performed on a `make clean'
########################################################################

.PHONY: extraclean
extraclean:
	-rm -f $(LIBRARY_NAMES:%=$(PROJECT_LIBDIR)/$(call DSONAME,%))
	-rm -f $(LIBRARY_NAMES:%=$(PROJECT_LIBDIR)/$(call LINKDSONAME,%))
	-rm -f $(MAKEFILEFRAGMENT) $(MAKECONFIGFILE) $(PKGCONFIGFILE)

.PHONY: extrasqueakyclean
extrasqueakyclean:
	-rm -f $(ALL)
	-rm -f $(CONFIGFILES)
	-rm -f $(MAKEFILEFRAGMENT) $(MAKECONFIGFILE) $(PKGCONFIGFILE)
	-rm -f $(VRUI_SCRIPTDIR)/Vrui.makeinclude $(VRUI_SCRIPTDIR)/Vrui.debug.makeinclude
	-rm -f $(VRUI_MAKEDIR)/Configuration.Vrui

# Include basic makefile
include $(VRUI_MAKEDIR)/BasicMakefile

########################################################################
# Specify build rules for dynamic shared objects
########################################################################

# Set up a link rpath to find package-created libraries while building
# dependent objects:
EXTRALINKDIRFLAGS = -Wl,-rpath-link=$(PROJECT_LIBDIR)

#
# The Miscellaneous Support Library (Misc)
#

MISC_HEADERS = $(wildcard Misc/*.h) \
               $(wildcard Misc/*.icpp)

MISC_SOURCES = $(wildcard Misc/*.cpp)

$(call LIBOBJNAMES,$(MISC_SOURCES)): | $(DEPDIR)/config

MISC_PACKAGES = MATH
$(call LIBRARYNAME,libMisc): PACKAGES = $(MISC_PACKAGES)
$(call LIBRARYNAME,libMisc): EXTRACINCLUDEFLAGS += $(MYMISC_INCLUDE)
$(call LIBRARYNAME,libMisc): | $(call DEPENDENCIES,$(MISC_PACKAGES))
$(call LIBRARYNAME,libMisc): $(call LIBOBJNAMES,$(MISC_SOURCES))
.PHONY: libMisc
libMisc: $(call LIBRARYNAME,libMisc)

#
# The Realtime Processing Library (Realtime):
#

$(DEPDIR)/Configure-Realtime: $(DEPDIR)/Configure-Package
ifneq ($(SYSTEM_HAVE_RT),0)
	@echo Realtime library uses POSIX clocks
	@echo Realtime library uses POSIX timers
else
	@echo Realtime library uses UNIX clocks
	@echo Realtime library uses wallclock time
endif
	@cp Realtime/Config.h.template Realtime/Config.h.temp
	@$(call CONFIG_SETVAR,Realtime/Config.h.temp,REALTIME_CONFIG_HAVE_POSIX_CLOCKS,$(SYSTEM_HAVE_RT))
	@$(call CONFIG_SETVAR,Realtime/Config.h.temp,REALTIME_CONFIG_HAVE_POSIX_TIMERS,$(SYSTEM_HAVE_RT))
	@if ! diff -qN Realtime/Config.h.temp Realtime/Config.h > /dev/null ; then cp Realtime/Config.h.temp Realtime/Config.h ; fi
	@rm Realtime/Config.h.temp
	@touch $(DEPDIR)/Configure-Realtime

REALTIME_HEADERS = $(wildcard Realtime/*.h) \
                   $(wildcard Realtime/*.icpp)

REALTIME_SOURCES = $(wildcard Realtime/*.cpp)

$(call LIBOBJNAMES,$(REALTIME_SOURCES)): | $(DEPDIR)/config

REALTIME_PACKAGES = MYMISC MATH
ifneq ($(SYSTEM_HAVE_RT),0)
  REALTIME_PACKAGES += RT
endif
$(call LIBRARYNAME,libRealtime): PACKAGES = $(REALTIME_PACKAGES)
$(call LIBRARYNAME,libRealtime): EXTRACINCLUDEFLAGS += $(MYREALTIME_INCLUDE)
$(call LIBRARYNAME,libRealtime): | $(call DEPENDENCIES,$(REALTIME_PACKAGES))
$(call LIBRARYNAME,libRealtime): $(call LIBOBJNAMES,$(REALTIME_SOURCES))
.PHONY: libRealtime
libRealtime: $(call LIBRARYNAME,libRealtime)

#
# The Portable Threading Library (Threads):
#

$(DEPDIR)/Configure-Threads: $(DEPDIR)/Configure-Realtime
ifneq ($(SYSTEM_HAVE_TLS),0)
	@echo Threads library has built-in thread-local storage
else
	@echo Threads library uses POSIX thread-local storage
endif
ifneq ($(SYSTEM_HAVE_ATOMICS),0)
	@echo Threads library uses built-in atomics
else
	@echo Threads library simulates atomics using POSIX spinlocks or mutexes
endif
ifneq ($(SYSTEM_HAVE_SPINLOCKS),0)
	@echo Threads library uses POSIX spinlocks
else
	@echo Threads library simulates spinlocks using POSIX mutexes
endif
ifneq ($(SYSTEM_CAN_CANCEL_THREADS),0)
	@echo Local pthread implements pthread_cancel
else
	@echo Local pthread does not implement pthread_cancel
endif
	@cp Threads/Config.h.template Threads/Config.h.temp
	@$(call CONFIG_SETVAR,Threads/Config.h.temp,THREADS_CONFIG_HAVE_BUILTIN_TLS,$(SYSTEM_HAVE_TLS))
	@$(call CONFIG_SETVAR,Threads/Config.h.temp,THREADS_CONFIG_HAVE_BUILTIN_ATOMICS,$(SYSTEM_HAVE_ATOMICS))
	@$(call CONFIG_SETVAR,Threads/Config.h.temp,THREADS_CONFIG_HAVE_SPINLOCKS,$(SYSTEM_HAVE_SPINLOCKS))
	@$(call CONFIG_SETVAR,Threads/Config.h.temp,THREADS_CONFIG_CAN_CANCEL,$(SYSTEM_CAN_CANCEL_THREADS))
	@if ! diff -qN Threads/Config.h.temp Threads/Config.h > /dev/null ; then cp Threads/Config.h.temp Threads/Config.h ; fi
	@rm Threads/Config.h.temp
	@touch $(DEPDIR)/Configure-Threads

THREADS_HEADERS = $(wildcard Threads/*.h) \
                  $(wildcard Threads/*.icpp)

THREADS_SOURCES = $(wildcard Threads/*.cpp)

$(call LIBOBJNAMES,$(THREADS_SOURCES)): | $(DEPDIR)/config

THREADS_PACKAGES = MYMISC PTHREADS MATH
$(call LIBRARYNAME,libThreads): PACKAGES = $(THREADS_PACKAGES)
$(call LIBRARYNAME,libThreads): EXTRACINCLUDEFLAGS += $(MYTHREADS_INCLUDE)
$(call LIBRARYNAME,libThreads): | $(call DEPENDENCIES,$(THREADS_PACKAGES))
$(call LIBRARYNAME,libThreads): $(call LIBOBJNAMES,$(THREADS_SOURCES))
.PHONY: libThreads
libThreads: $(call LIBRARYNAME,libThreads)

#
# The USB Support Library (USB):
#

$(DEPDIR)/Configure-USB: $(DEPDIR)/Configure-Threads
ifneq ($(SYSTEM_HAVE_LIBUSB1),0)
	@echo Libusb library version 1.0 exists on host system
  ifneq ($(LIBUSB1_HAS_TOPOLOGY_CALLS),0)
	@echo Libusb library supports bus topology queries
  else
	@echo Libusb library does not support bus topology queries
  endif
  ifneq ($(LIBUSB1_HAS_STRERROR),0)
	@echo Libusb library has libusb_strerror function
  else
	@echo Libusb library does not have libusb_strerror function
  endif
else
	@echo Libusb library version 1.0 does not exist on host system
endif
	@cp USB/Config.h.template USB/Config.h.temp
	@$(call CONFIG_SETVAR,USB/Config.h.temp,USB_CONFIG_HAVE_LIBUSB1,$(SYSTEM_HAVE_LIBUSB1))
	@$(call CONFIG_SETVAR,USB/Config.h.temp,USB_CONFIG_HAVE_TOPOLOGY_CALLS,$(LIBUSB1_HAS_TOPOLOGY_CALLS))
	@$(call CONFIG_SETVAR,USB/Config.h.temp,USB_CONFIG_HAVE_STRERROR,$(LIBUSB1_HAS_STRERROR))
	@$(call CONFIG_SETVAR,USB/Config.h.temp,USB_CONFIG_HAVE_SET_OPTION,$(LIBUSB1_HAS_SET_OPTION))
	@if ! diff -qN USB/Config.h.temp USB/Config.h > /dev/null ; then cp USB/Config.h.temp USB/Config.h ; fi
	@rm USB/Config.h.temp
	@touch $(DEPDIR)/Configure-USB

USB_HEADERS = $(wildcard USB/*.h) \
              $(wildcard USB/*.icpp)

USB_SOURCES = $(wildcard USB/*.cpp)

$(call LIBOBJNAMES,$(USB_SOURCES)): | $(DEPDIR)/config

USB_PACKAGES = MYTHREADS MYMISC LIBUSB1
$(call LIBRARYNAME,libUSB): PACKAGES = $(USB_PACKAGES)
$(call LIBRARYNAME,libUSB): EXTRACINCLUDEFLAGS += $(MYUSB_INCLUDE)
$(call LIBRARYNAME,libUSB): | $(call DEPENDENCIES,$(USB_PACKAGES))
$(call LIBRARYNAME,libUSB): $(call LIBOBJNAMES,$(USB_SOURCES))
.PHONY: libUSB
libUSB: $(call LIBRARYNAME,libUSB)

#
# The Raw HID Support Library (RawHID):
#

$(DEPDIR)/Configure-RawHID: $(DEPDIR)/Configure-USB
ifneq ($(SYSTEM_HAVE_LIBUDEV),0)
	@echo Libudev library exists on host system
	@echo RawHID library enabled
else
	@echo Libudev library does not exist on host system
	@echo RawHID library disabled
endif

RAWHID_HEADERS = $(wildcard RawHID/*.h) \
                 $(wildcard RawHID/*.icpp)

RAWHID_SOURCES = $(wildcard RawHID/*.cpp) \
                 $(wildcard RawHID/Internal/*.cpp)

$(call LIBOBJNAMES,$(RAWHID_SOURCES)): | $(DEPDIR)/config

RAWHID_PACKAGES = MYTHREADS MYMISC LIBUDEV
$(call LIBRARYNAME,libRawHID): PACKAGES = $(RAWHID_PACKAGES)
$(call LIBRARYNAME,libRawHID): EXTRACINCLUDEFLAGS += $(MYRAWHID_INCLUDE)
$(call LIBRARYNAME,libRawHID): | $(call DEPENDENCIES,$(RAWHID_PACKAGES))
$(call LIBRARYNAME,libRawHID): $(call LIBOBJNAMES,$(RAWHID_SOURCES))
.PHONY: libRawHID
libRawHID: $(call LIBRARYNAME,libRawHID)

#
# The I/O Support Library (IO):
#

IO_HEADERS = $(wildcard IO/*.h) \
             $(wildcard IO/*.icpp)

IO_SOURCES = $(wildcard IO/*.cpp)

$(call LIBOBJNAMES,$(IO_SOURCES)): | $(DEPDIR)/config

IO_PACKAGES = MYTHREADS MYMISC ZLIB MATH
$(call LIBRARYNAME,libIO): PACKAGES = $(IO_PACKAGES)
$(call LIBRARYNAME,libIO): EXTRACINCLUDEFLAGS += $(MYIO_INCLUDE)
$(call LIBRARYNAME,libIO): | $(call DEPENDENCIES,$(IO_PACKAGES))
$(call LIBRARYNAME,libIO): $(call LIBOBJNAMES,$(IO_SOURCES))
.PHONY: libIO
libIO: $(call LIBRARYNAME,libIO)

#
# The Plugin Handling Library (Plugins):
#

PLUGINS_HEADERS = $(wildcard Plugins/*.h) \
                  $(wildcard Plugins/*.icpp)

PLUGINS_SOURCES = $(wildcard Plugins/*.cpp)

$(call LIBOBJNAMES,$(PLUGINS_SOURCES)): | $(DEPDIR)/config

PLUGINS_PACKAGES = MYMISC DL
$(call LIBRARYNAME,libPlugins): PACKAGES = $(PLUGINS_PACKAGES)
$(call LIBRARYNAME,libPlugins): EXTRACINCLUDEFLAGS += $(MYPLUGINS_INCLUDE)
$(call LIBRARYNAME,libPlugins): | $(call DEPENDENCIES,$(PLUGINS_PACKAGES))
$(call LIBRARYNAME,libPlugins): $(call LIBOBJNAMES,$(PLUGINS_SOURCES))
.PHONY: libPlugins
libPlugins: $(call LIBRARYNAME,libPlugins)

#
# The Portable Communications Library (Comm):
#

$(DEPDIR)/Configure-Comm: $(DEPDIR)/Configure-USB
ifneq ($(SYSTEM_HAVE_OPENSSL),0)
	@echo "TLS-secured TCP connections enabled"
else
	@echo "TLS-secured TCP connections disabled"
endif
	@cp Comm/Config.h.template Comm/Config.h.temp
	@$(call CONFIG_SETVAR,Comm/Config.h.temp,COMM_CONFIG_HAVE_OPENSSL,$(SYSTEM_HAVE_OPENSSL))
	@if ! diff -qN Comm/Config.h.temp Comm/Config.h > /dev/null ; then cp Comm/Config.h.temp Comm/Config.h ; fi
	@rm Comm/Config.h.temp
	@touch $(DEPDIR)/Configure-Comm

COMM_HEADERS = $(wildcard Comm/*.h) \
               $(wildcard Comm/*.icpp)

# Don't build the following modules unless specifically asked:
COMM_IGNORE_SOURCES = Comm/TLSPipe.cpp

COMM_SOURCES = $(filter-out $(COMM_IGNORE_SOURCES),$(wildcard Comm/*.cpp))
ifneq ($(SYSTEM_HAVE_OPENSSL),0)
  COMM_SOURCES += Comm/TLSPipe.cpp
endif

$(call LIBOBJNAMES,$(COMM_SOURCES)): | $(DEPDIR)/config

COMM_PACKAGES = MYIO MYTHREADS MYMISC
ifneq ($(SYSTEM_HAVE_OPENSSL),0)
  COMM_PACKAGES += OPENSSL
endif
$(call LIBRARYNAME,libComm): PACKAGES = $(COMM_PACKAGES)
$(call LIBRARYNAME,libComm): EXTRACINCLUDEFLAGS += $(MYCOMM_INCLUDE)
$(call LIBRARYNAME,libComm): | $(call DEPENDENCIES,$(COMM_PACKAGES))
$(call LIBRARYNAME,libComm): $(call LIBOBJNAMES,$(COMM_SOURCES))
.PHONY: libComm
libComm: $(call LIBRARYNAME,libComm)

#
# The Cluster Abstraction Library (Cluster)
#

CLUSTER_HEADERS = $(wildcard Cluster/*.h) \
                  $(wildcard Cluster/*.icpp)

CLUSTER_SOURCES = $(wildcard Cluster/*.cpp)

$(call LIBOBJNAMES,$(CLUSTER_SOURCES)): | $(DEPDIR)/config

CLUSTER_PACKAGES = MYCOMM MYIO MYTHREADS MYMISC
$(call LIBRARYNAME,libCluster): PACKAGES = $(CLUSTER_PACKAGES)
$(call LIBRARYNAME,libCluster): EXTRACINCLUDEFLAGS += $(MYCLUSTER_INCLUDE)
$(call LIBRARYNAME,libCluster): | $(call DEPENDENCIES,$(CLUSTER_PACKAGES))
$(call LIBRARYNAME,libCluster): $(call LIBOBJNAMES,$(CLUSTER_SOURCES))
.PHONY: libCluster
libCluster: $(call LIBRARYNAME,libCluster)

#
# The Templatized Math Library (Math)
#

MATH_HEADERS = $(wildcard Math/*.h) \
               $(wildcard Math/*.icpp)

MATH_SOURCES = $(wildcard Math/*.cpp)

$(call LIBOBJNAMES,$(MATH_SOURCES)): | $(DEPDIR)/config

MATH_PACKAGES = MYMISC MATH
$(call LIBRARYNAME,libMath): PACKAGES = $(MATH_PACKAGES)
$(call LIBRARYNAME,libMath): EXTRACINCLUDEFLAGS += $(MYMATH_INCLUDE)
$(call LIBRARYNAME,libMath): | $(call DEPENDENCIES,$(MATH_PACKAGES))
$(call LIBRARYNAME,libMath): $(call LIBOBJNAMES,$(MATH_SOURCES))
.PHONY: libMath
libMath: $(call LIBRARYNAME,libMath)

#
# The Templatized Geometry Library (Geometry)
#

GEOMETRY_HEADERS = $(wildcard Geometry/*.h) \
                   $(wildcard Geometry/*.icpp)

GEOMETRY_SOURCES = $(wildcard Geometry/*.cpp)

$(call LIBOBJNAMES,$(GEOMETRY_SOURCES)): | $(DEPDIR)/config

GEOMETRY_PACKAGES = MYMATH MYIO MYTHREADS MYMISC
$(call LIBRARYNAME,libGeometry): PACKAGES = $(GEOMETRY_PACKAGES)
$(call LIBRARYNAME,libGeometry): EXTRACINCLUDEFLAGS += $(MYGEOMETRY_INCLUDE)
$(call LIBRARYNAME,libGeometry): | $(call DEPENDENCIES,$(GEOMETRY_PACKAGES))
$(call LIBRARYNAME,libGeometry): $(call LIBOBJNAMES,$(GEOMETRY_SOURCES))
.PHONY: libGeometry
libGeometry: $(call LIBRARYNAME,libGeometry)

#
# The Mac OSX Support Library (MacOSX)
#

MACOSX_HEADERS = $(wildcard MacOSX/*.h) \
                 $(wildcard MacOSX/*.icpp)

#
# The OpenGL C++ Wrapper Library (GLWrappers)
#

GLWRAPPERS_HEADERS = GL/GLTexCoordTemplates.h \
                     GL/GLMultiTexCoordTemplates.h \
                     GL/GLColorTemplates.h \
                     GL/GLSecondaryColorTemplates.h \
                     GL/GLNormalTemplates.h \
                     GL/GLVertexTemplates.h \
                     GL/GLIndexTemplates.h \
                     GL/GLPrimitiveTemplates.h \
                     GL/GLScalarLimits.h \
                     GL/GLScalarConverter.h \
                     GL/GLColor.h \
                     GL/GLColorOperations.h \
                     GL/GLVector.h \
                     GL/GLBox.h \
                     GL/GLVertexArrayTemplates.h \
                     GL/GLVertexArrayParts.h \
                     GL/GLVertex.h GL/GLVertex.icpp \
                     GL/GLFogEnums.h \
                     GL/GLFogTemplates.h \
                     GL/GLFog.h \
                     GL/GLLightEnums.h \
                     GL/GLLightTemplates.h \
                     GL/GLLight.h \
                     GL/GLLightModelEnums.h \
                     GL/GLLightModelTemplates.h \
                     GL/GLMaterialEnums.h \
                     GL/GLMaterialTemplates.h \
                     GL/GLMaterial.h \
                     GL/GLTexEnvEnums.h \
                     GL/GLTexEnvTemplates.h \
                     GL/GLMatrixEnums.h \
                     GL/GLMatrixTemplates.h \
                     GL/GLMiscTemplates.h \
                     GL/GLGetTemplates.h \
                     GL/GLGetPrimitiveTemplates.h \
                     GL/GLGetFogTemplates.h \
                     GL/GLGetLightTemplates.h \
                     GL/GLGetMaterialTemplates.h \
                     GL/GLGetTexEnvTemplates.h \
                     GL/GLGetMatrixTemplates.h \
                     GL/GLGetMiscTemplates.h

GLWRAPPERS_SOURCES = GL/GLScalarLimits.cpp \
                     GL/GLColor.cpp \
                     GL/GLVertex.cpp \
                     GL/GLFog.cpp \
                     GL/GLLight.cpp \
                     GL/GLMaterial.cpp

$(call LIBOBJNAMES,$(GLWRAPPERS_SOURCES)): | $(DEPDIR)/config

GLWRAPPERS_PACKAGES = MYMATH GL
$(call LIBRARYNAME,libGLWrappers): PACKAGES = $(GLWRAPPERS_PACKAGES)
$(call LIBRARYNAME,libGLWrappers): EXTRACINCLUDEFLAGS += $(MYGLWRAPPERS_INCLUDE)
$(call LIBRARYNAME,libGLWrappers): CFLAGS += $(MYGLWRAPPERS_CFLAGS)
$(call LIBRARYNAME,libGLWrappers): | $(call DEPENDENCIES,$(GLWRAPPERS_PACKAGES))
$(call LIBRARYNAME,libGLWrappers): $(call LIBOBJNAMES,$(GLWRAPPERS_SOURCES))
.PHONY: libGLWrappers
libGLWrappers: $(call LIBRARYNAME,libGLWrappers)

#
# The OpenGL Support Library (GLSupport)
#

$(DEPDIR)/Configure-GLSupport: $(DEPDIR)/Configure-Comm
ifneq ($(GLSUPPORT_USE_TLS),0)
  ifneq ($(SYSTEM_HAVE_TLS),0)
	@echo "Multithreaded rendering enabled via TLS"
  else
	@echo "Multithreaded rendering enabled via pthreads"
  endif
else
	@echo "Multithreaded rendering disabled"
endif
	@echo "Default GL font directory:" $(SHAREINSTALLDIR)/GLFonts
	@echo "Default GL shader directory:" $(SHAREINSTALLDIR)/Shaders/GLSupport
	@cp GL/Config.h.template GL/Config.h.temp
	@$(call CONFIG_SETVAR,GL/Config.h.temp,GLSUPPORT_CONFIG_HAVE_GLXGETPROCADDRESS,$(SYSTEM_HAVE_GLXGETPROCADDRESS))
	@$(call CONFIG_SETVAR,GL/Config.h.temp,GLSUPPORT_CONFIG_USE_TLS,$(GLSUPPORT_USE_TLS))
	@$(call CONFIG_SETVAR,GL/Config.h.temp,GLSUPPORT_CONFIG_HAVE_BUILTIN_TLS,$(SYSTEM_HAVE_TLS))
	@$(call CONFIG_SETSTRINGVAR,GL/Config.h.temp,GLSUPPORT_CONFIG_GL_FONT_DIR,$(SHAREINSTALLDIR)/GLFonts)
	@$(call CONFIG_SETSTRINGVAR,GL/Config.h.temp,GLSUPPORT_CONFIG_SHADERDIR,$(SHAREINSTALLDIR)/Shaders/GLSupport)
	@if ! diff -qN GL/Config.h.temp GL/Config.h > /dev/null ; then cp GL/Config.h.temp GL/Config.h ; fi
	@rm GL/Config.h.temp
	@touch $(DEPDIR)/Configure-GLSupport

GLSUPPORT_HEADERS = GL/Config.h \
                    GL/GLPrintError.h \
                    GL/GLMarshallers.h GL/GLMarshallers.icpp \
                    GL/GLValueCoders.h \
                    GL/GLLightTracker.h \
                    GL/GLClipPlaneTracker.h \
                    GL/GLShaderManager.h \
                    GL/TLSHelper.h \
                    GL/GLObject.h \
                    GL/GLContextData.h \
                    GL/GLExtensions.h \
                    GL/GLExtensionManager.h \
                    GL/GLTextureObject.h \
                    GL/GLBuffer.h \
                    GL/GLVertexBuffer.h GL/GLVertexBuffer.icpp \
                    GL/GLIndexBuffer.h GL/GLIndexBuffer.icpp \
                    GL/GLShaderSupport.h \
                    GL/GLShader.h \
                    GL/GLGeometryShader.h \
                    GL/GLAutomaticShader.h \
                    GL/GLLineLightingShader.h \
                    GL/GLSphereRenderer.h \
                    GL/GLCylinderRenderer.h \
                    GL/GLFrameBuffer.h \
                    GL/GLColorMap.h \
                    GL/GLNumberRenderer.h \
                    GL/GLFont.h \
                    GL/GLString.h \
                    GL/GLLabel.h \
                    GL/GLLineIlluminator.h \
                    GL/GLModels.h

GLSUPPORTEXTENSION_HEADERS = $(wildcard GL/Extensions/*.h) \
                             $(wildcard GL/Extensions/*.icpp)

GLSUPPORT_SOURCES = GL/GLPrintError.cpp \
                    GL/GLMarshallers.cpp \
                    GL/GLValueCoders.cpp \
                    GL/GLLightTracker.cpp \
                    GL/GLClipPlaneTracker.cpp \
                    GL/GLShaderManager.cpp \
                    GL/GLObject.cpp \
                    GL/Internal/GLThingManager.cpp \
                    GL/GLContextData.cpp \
                    GL/GLExtensions.cpp \
                    GL/GLExtensionManager.cpp \
                    $(wildcard GL/Extensions/*.cpp) \
                    GL/GLTextureObject.cpp \
                    GL/GLBuffer.cpp \
                    GL/GLShaderSupport.cpp \
                    GL/GLShader.cpp \
                    GL/GLGeometryShader.cpp \
                    GL/GLAutomaticShader.cpp \
                    GL/GLLineLightingShader.cpp \
                    GL/GLSphereRenderer.cpp \
                    GL/GLCylinderRenderer.cpp \
                    GL/GLFrameBuffer.cpp \
                    GL/GLColorMap.cpp \
                    GL/GLNumberRenderer.cpp \
                    GL/GLFont.cpp \
                    GL/GLString.cpp \
                    GL/GLLabel.cpp \
                    GL/GLLineIlluminator.cpp \
                    GL/GLModels.cpp

$(call LIBOBJNAMES,$(GLSUPPORT_SOURCES)): | $(DEPDIR)/config

GLSUPPORT_PACKAGES = MYGLWRAPPERS MYGEOMETRY MYMATH MYIO MYTHREADS MYMISC GL MATH
ifeq ($(SYSTEM_HAVE_GLXGETPROCADDRESS),0)
  GLSUPPORT_PACKAGES += DL
endif
$(call LIBRARYNAME,libGLSupport): PACKAGES = $(GLSUPPORT_PACKAGES)
$(call LIBRARYNAME,libGLSupport): EXTRACINCLUDEFLAGS += $(MYGLSUPPORT_INCLUDE)
$(call LIBRARYNAME,libGLSupport): | $(call DEPENDENCIES,$(GLSUPPORT_PACKAGES))
$(call LIBRARYNAME,libGLSupport): $(call LIBOBJNAMES,$(GLSUPPORT_SOURCES))
.PHONY: libGLSupport
libGLSupport: $(call LIBRARYNAME,libGLSupport)

#
# The OpenGL/GLX Support Library (GLXSupport)
#

GLXSUPPORT_HEADERS = GL/GLContext.h \
                     GL/GLWindow.h

GLXSUPPORT_SOURCES = GL/GLContext.cpp \
                     GL/GLWindow.cpp

$(call LIBOBJNAMES,$(GLXSUPPORT_SOURCES)): | $(DEPDIR)/config

GLXSUPPORT_PACKAGES = MYGLSUPPORT MYTHREADS MYMISC GL X11
$(call LIBRARYNAME,libGLXSupport): PACKAGES = $(GLXSUPPORT_PACKAGES)
$(call LIBRARYNAME,libGLXSupport): EXTRACINCLUDEFLAGS += $(MYGLXSUPPORT_INCLUDE)
$(call LIBRARYNAME,libGLXSupport): | $(call DEPENDENCIES,$(GLXSUPPORT_PACKAGES))
$(call LIBRARYNAME,libGLXSupport): $(call LIBOBJNAMES,$(GLXSUPPORT_SOURCES))
.PHONY: libGLXSupport
libGLXSupport: $(call LIBRARYNAME,libGLXSupport)

#
# The OpenGL Wrapper Library for the Templatized Geometry Library (GLGeometry)
#

GLGEOMETRY_HEADERS = GL/GLGeometryWrappers.h \
                     GL/GLGeometryVertex.h GL/GLGeometryVertex.icpp \
                     GL/GLTransformationWrappers.h GL/GLTransformationWrappers.icpp \
                     GL/GLFrustum.h GL/GLFrustum.icpp \
                     GL/GLPolylineTube.h

GLGEOMETRY_SOURCES = GL/GLGeometryVertex.cpp \
                     GL/GLTransformationWrappers.cpp \
                     GL/GLFrustum.cpp \
                     GL/GLPolylineTube.cpp

$(call LIBOBJNAMES,$(GLGEOMETRY_SOURCES)): | $(DEPDIR)/config

GLGEOMETRY_PACKAGES = MYGLSUPPORT MYGLWRAPPERS MYGEOMETRY MYMATH GL
$(call LIBRARYNAME,libGLGeometry): PACKAGES = $(GLGEOMETRY_PACKAGES)
$(call LIBRARYNAME,libGLGeometry): EXTRACINCLUDEFLAGS += $(MYGLGEOMETRY_INCLUDE)
$(call LIBRARYNAME,libGLGeometry): | $(call DEPENDENCIES,$(GLGEOMETRY_PACKAGES))
$(call LIBRARYNAME,libGLGeometry): $(call LIBOBJNAMES,$(GLGEOMETRY_SOURCES))
.PHONY: libGLGeometry
libGLGeometry: $(call LIBRARYNAME,libGLGeometry)

#
# The Vulkan C++ Wrapper Library (Vulkan)
#

$(DEPDIR)/Configure-Vulkan: $(DEPDIR)/Configure-GLSupport
ifneq ($(SYSTEM_HAVE_VULKAN),0)
	@echo "Vulkan low-level 3D graphics API enabled"
else
	@echo "Vulkan low-level 3D graphics API disabled"
endif
	@touch $(DEPDIR)/Configure-Vulkan

VULKAN_HEADERS = $(wildcard Vulkan/*.h) \
                 $(wildcard Vulkan/*.icpp)

VULKAN_SOURCES = $(wildcard Vulkan/*.cpp)

$(call LIBOBJNAMES,$(VULKAN_SOURCES)): | $(DEPDIR)/config

VULKAN_PACKAGES = MYIO MYMISC VULKAN
$(call LIBRARYNAME,libVulkan): PACKAGES = $(VULKAN_PACKAGES)
$(call LIBRARYNAME,libVulkan): EXTRACINCLUDEFLAGS += $(MYVULKAN_INCLUDE)
$(call LIBRARYNAME,libVulkan): | $(call DEPENDENCIES,$(VULKAN_PACKAGES))
$(call LIBRARYNAME,libVulkan): $(call LIBOBJNAMES,$(VULKAN_SOURCES))
.PHONY: libVulkan
libVulkan: $(call LIBRARYNAME,libVulkan)

#
# The Vulkan Xlib Binding Library (VulkanXlib)
#

VULKANXLIB_HEADERS = $(wildcard VulkanXlib/*.h) \
                     $(wildcard VulkanXlib/*.icpp)

VULKANXLIB_SOURCES = $(wildcard VulkanXlib/*.cpp)

$(call LIBOBJNAMES,$(VULKANXLIB_SOURCES)): | $(DEPDIR)/config

VULKANXLIB_PACKAGES = MYVULKAN MYMATH MYMISC VULKAN X11
$(call LIBRARYNAME,libVulkanXlib): PACKAGES = $(VULKANXLIB_PACKAGES)
$(call LIBRARYNAME,libVulkanXlib): EXTRACINCLUDEFLAGS += $(MYVULKANXLIB_INCLUDE)
$(call LIBRARYNAME,libVulkanXlib): | $(call DEPENDENCIES,$(VULKANXLIB_PACKAGES))
$(call LIBRARYNAME,libVulkanXlib): $(call LIBOBJNAMES,$(VULKANXLIB_SOURCES))
.PHONY: libVulkanXlib
libVulkanXlib: $(call LIBRARYNAME,libVulkanXlib)

#
# The Image Handling Library (Images)
#

$(DEPDIR)/Configure-Images: $(DEPDIR)/Configure-Vulkan
ifneq ($(SYSTEM_HAVE_LIBPNG),0)
	@echo "PNG image file format enabled"
else
	@echo "PNG image file format disabled"
endif
ifneq ($(SYSTEM_HAVE_LIBJPEG),0)
	@echo "JPG image file format enabled"
else
	@echo "JPG image file format disabled"
endif
ifneq ($(SYSTEM_HAVE_LIBTIFF),0)
	@echo "TIFF image file format enabled"
else
	@echo "TIFF image file format disabled"
endif
	@cp Images/Config.h.template Images/Config.h.temp
	@$(call CONFIG_SETVAR,Images/Config.h.temp,IMAGES_CONFIG_HAVE_PNG,$(SYSTEM_HAVE_LIBPNG))
	@$(call CONFIG_SETVAR,Images/Config.h.temp,IMAGES_CONFIG_HAVE_JPEG,$(SYSTEM_HAVE_LIBJPEG))
	@$(call CONFIG_SETVAR,Images/Config.h.temp,IMAGES_CONFIG_HAVE_TIFF,$(SYSTEM_HAVE_LIBTIFF))
	@if ! diff -qN Images/Config.h.temp Images/Config.h > /dev/null ; then cp Images/Config.h.temp Images/Config.h ; fi
	@rm Images/Config.h.temp
	@touch $(DEPDIR)/Configure-Images

IMAGES_HEADERS = $(wildcard Images/*.h) \
                 $(wildcard Images/*.icpp)

IMAGES_SOURCES = $(wildcard Images/*.cpp)

$(call LIBOBJNAMES,$(IMAGES_SOURCES)): | $(DEPDIR)/config

IMAGES_PACKAGES = MYGLSUPPORT MYGLWRAPPERS MYMATH MYIO MYTHREADS MYMISC GL
ifneq ($(SYSTEM_HAVE_LIBPNG),0)
  IMAGES_PACKAGES += PNG
endif
ifneq ($(SYSTEM_HAVE_LIBJPEG),0)
  IMAGES_PACKAGES += JPEG
endif
ifneq ($(SYSTEM_HAVE_LIBTIFF),0)
  IMAGES_PACKAGES += TIFF
endif
$(call LIBRARYNAME,libImages): PACKAGES = $(IMAGES_PACKAGES)
$(call LIBRARYNAME,libImages): EXTRACINCLUDEFLAGS += $(MYIMAGES_INCLUDE)
$(call LIBRARYNAME,libImages): | $(call DEPENDENCIES,$(IMAGES_PACKAGES))
$(call LIBRARYNAME,libImages): $(call LIBOBJNAMES,$(IMAGES_SOURCES))
.PHONY: libImages
libImages: $(call LIBRARYNAME,libImages)

#
# The GLMotif 3D User Interface Component Library (GLMotif)
#

$(DEPDIR)/Configure-GLMotif: $(DEPDIR)/Configure-Images
	@cp GLMotif/Config.h.template GLMotif/Config.h.temp
	@$(call CONFIG_SETSTRINGVAR,GLMotif/Config.h.temp,GLMOTIF_CONFIG_SHAREDIR,$(SHAREINSTALLDIR))
	@if ! diff -qN GLMotif/Config.h.temp GLMotif/Config.h > /dev/null ; then cp GLMotif/Config.h.temp GLMotif/Config.h ; fi
	@rm GLMotif/Config.h.temp
	@touch $(DEPDIR)/Configure-GLMotif

GLMOTIF_HEADERS = $(wildcard GLMotif/*.h) \
                  $(wildcard GLMotif/*.icpp)

GLMOTIF_SOURCES = $(wildcard GLMotif/*.cpp)

$(call LIBOBJNAMES,$(GLMOTIF_SOURCES)): | $(DEPDIR)/config

GLMOTIF_PACKAGES = MYIMAGES MYGLGEOMETRY MYGLSUPPORT MYGLWRAPPERS MYGEOMETRY MYMATH MYIO MYMISC GL
$(call LIBRARYNAME,libGLMotif): PACKAGES = $(GLMOTIF_PACKAGES)
$(call LIBRARYNAME,libGLMotif): EXTRACINCLUDEFLAGS += $(MYGLMOTIF_INCLUDE)
$(call LIBRARYNAME,libGLMotif): | $(call DEPENDENCIES,$(GLMOTIF_PACKAGES))
$(call LIBRARYNAME,libGLMotif): $(call LIBOBJNAMES,$(GLMOTIF_SOURCES))
.PHONY: libGLMotif
libGLMotif: $(call LIBRARYNAME,libGLMotif)

#
# The basic sound library (Sound)
#

$(DEPDIR)/Configure-Sound: $(DEPDIR)/Configure-GLMotif
ifneq ($(SYSTEM_HAVE_ALSA),0)
	@echo "ALSA sound device support enabled"
else
	@echo "ALSA sound device support disabled"
endif
ifneq ($(SYSTEM_HAVE_SPEEX),0)
	@echo "SPEEX speech compression support enabled"
else
	@echo "SPEEX speech compression support disabled"
endif
	@cp Sound/Config.h.template Sound/Config.h.temp
	@$(call CONFIG_SETVAR,Sound/Config.h.temp,SOUND_CONFIG_HAVE_ALSA,$(SYSTEM_HAVE_ALSA))
	@$(call CONFIG_SETVAR,Sound/Config.h.temp,SOUND_CONFIG_HAVE_PULSEAUDIO,$(SYSTEM_HAVE_PULSEAUDIO))
	@$(call CONFIG_SETVAR,Sound/Config.h.temp,SOUND_CONFIG_HAVE_SPEEX,$(SYSTEM_HAVE_SPEEX))
	@if ! diff -qN Sound/Config.h.temp Sound/Config.h > /dev/null ; then cp Sound/Config.h.temp Sound/Config.h ; fi
	@rm Sound/Config.h.temp
	@touch $(DEPDIR)/Configure-Sound

SOUND_HEADERS = $(wildcard Sound/*.h) \
                $(wildcard Sound/*.icpp)
ifeq ($(SYSTEM),LINUX)
  SOUND_LINUX_HEADERS = 
  ifneq ($(SYSTEM_HAVE_ALSA),0)
    SOUND_LINUX_HEADERS += Sound/Linux/ALSAPCMDevice.h \
                           Sound/Linux/ALSAAudioCaptureDevice.h
  endif
  ifneq ($(SYSTEM_HAVE_PULSEAUDIO),0)
    SOUND_LINUX_HEADERS += Sound/Linux/PulseAudio.h
  endif
  ifneq ($(SYSTEM_HAVE_SPEEX),0)
    SOUND_LINUX_HEADERS += Sound/Linux/SpeexEncoder.h \
                           Sound/Linux/SpeexDecoder.h
  endif
endif

SOUND_SOURCES = $(wildcard Sound/*.cpp)
ifeq ($(SYSTEM),LINUX)
  ifneq ($(SYSTEM_HAVE_ALSA),0)
    SOUND_SOURCES += Sound/Linux/ALSAPCMDevice.cpp \
                     Sound/Linux/ALSAAudioCaptureDevice.cpp
  endif
  ifneq ($(SYSTEM_HAVE_PULSEAUDIO),0)
    SOUND_SOURCES += Sound/Linux/PulseAudio.cpp
  endif
  ifneq ($(SYSTEM_HAVE_SPEEX),0)
    SOUND_SOURCES += Sound/Linux/SpeexEncoder.cpp \
                     Sound/Linux/SpeexDecoder.cpp
  endif
endif

$(call LIBOBJNAMES,$(SOUND_SOURCES)): | $(DEPDIR)/config

SOUND_PACKAGES = MYIO MYTHREADS MYMISC MATH
ifeq ($(SYSTEM),DARWIN)
  SOUND_PACKAGES += AUDIOTOOLBOX COREAUDIO COREFOUNDATION
else
  ifneq ($(SYSTEM_HAVE_ALSA),0)
    SOUND_PACKAGES += ALSA
  endif
  ifneq ($(SYSTEM_HAVE_PULSEAUDIO),0)
    SOUND_PACKAGES += PULSEAUDIO
  endif
endif
ifneq ($(SYSTEM_HAVE_SPEEX),0)
  SOUND_PACKAGES += SPEEX
endif
$(call LIBRARYNAME,libSound): PACKAGES = $(SOUND_PACKAGES)
$(call LIBRARYNAME,libSound): EXTRACINCLUDEFLAGS += $(MYSOUND_INCLUDE)
$(call LIBRARYNAME,libSound): | $(call DEPENDENCIES,$(SOUND_PACKAGES))
$(call LIBRARYNAME,libSound): $(call LIBOBJNAMES,$(SOUND_SOURCES))
.PHONY: libSound
libSound: $(call LIBRARYNAME,libSound)

#
# The basic video library (Video)
#

$(DEPDIR)/Configure-Video: $(DEPDIR)/Configure-Sound
ifneq ($(SYSTEM_HAVE_V4L2),0)
	@echo "Video4Linux2 video device support enabled"
else
	@echo "Video4Linux2 video device support disabled"
endif
ifneq ($(SYSTEM_HAVE_DC1394),0)
	@echo "FireWire DC1394 video device support enabled"
else
	@echo "FireWire DC1394 video device support disabled"
endif
ifneq ($(SYSTEM_HAVE_THEORA),0)
	@echo "Theora video codec support enabled"
else
	@echo "Theora video codec support disabled"
endif
	@cp Video/Config.h.template Video/Config.h.temp
	@$(call CONFIG_SETVAR,Video/Config.h.temp,VIDEO_CONFIG_HAVE_V4L2,$(SYSTEM_HAVE_V4L2))
	@$(call CONFIG_SETVAR,Video/Config.h.temp,VIDEO_CONFIG_HAVE_DC1394,$(SYSTEM_HAVE_DC1394))
	@$(call CONFIG_SETVAR,Video/Config.h.temp,VIDEO_CONFIG_HAVE_THEORA,$(SYSTEM_HAVE_THEORA))
	@if ! diff -qN Video/Config.h.temp Video/Config.h > /dev/null ; then cp Video/Config.h.temp Video/Config.h ; fi
	@rm Video/Config.h.temp
	@touch $(DEPDIR)/Configure-Video

VIDEO_HEADERS = Video/Config.h \
                Video/Types.h \
                Video/VideoDataFormat.h \
                Video/FrameBuffer.h \
                Video/ImageExtractor.h \
                Video/VideoDevice.h \
                Video/ImageSequenceVideoDevice.h \
                Video/Colorspaces.h \
                Video/LensDistortion.h \
                Video/IntrinsicParameters.h \
                Video/YpCbCr420Texture.h \
                Video/VideoPane.h
ifneq ($(SYSTEM_HAVE_THEORA),0)
  VIDEO_HEADERS += Video/OggPage.h \
                   Video/OggSync.h \
                   Video/OggStream.h \
                   Video/TheoraInfo.h \
                   Video/TheoraComment.h \
                   Video/TheoraPacket.h \
                   Video/TheoraFrame.h \
                   Video/TheoraEncoder.h \
                   Video/TheoraDecoder.h
endif
VIDEO_HEADERS += Video/ViewerComponent.h

VIDEO_SOURCES = Video/VideoDataFormat.cpp \
                Video/VideoDevice.cpp \
                Video/ImageSequenceVideoDevice.cpp \
                Video/ImageExtractor.cpp \
                Video/Internal/ImageExtractorRGB8.cpp \
                Video/Internal/ImageExtractorY8.cpp \
                Video/Internal/ImageExtractorY10B.cpp \
                Video/Internal/ImageExtractorYUYV.cpp \
                Video/Internal/ImageExtractorUYVY.cpp \
                Video/Internal/ImageExtractorYV12.cpp \
                Video/Internal/ImageExtractorYpCbCr.cpp \
                Video/Internal/ImageExtractorBA81.cpp \
                Video/LensDistortion.cpp \
                Video/IntrinsicParameters.cpp \
                Video/YpCbCr420Texture.cpp \
                Video/VideoPane.cpp
ifneq ($(SYSTEM_HAVE_LIBJPEG),0)
  VIDEO_SOURCES += Video/Internal/ImageExtractorMJPG.cpp
endif
ifeq ($(SYSTEM),LINUX)
  ifneq ($(SYSTEM_HAVE_V4L2),0)
    VIDEO_SOURCES += Video/Linux/V4L2VideoDevice.cpp \
                     Video/Linux/OculusRiftDK2VideoDevice.cpp
  endif
  ifneq ($(SYSTEM_HAVE_DC1394),0)
    VIDEO_SOURCES += Video/Linux/DC1394VideoDevice.cpp
  endif
endif
ifneq ($(SYSTEM_HAVE_THEORA),0)
  VIDEO_SOURCES += Video/OggSync.cpp \
                   Video/OggStream.cpp \
                   Video/TheoraInfo.cpp \
                   Video/TheoraComment.cpp \
                   Video/TheoraPacket.cpp \
                   Video/TheoraFrame.cpp \
                   Video/TheoraEncoder.cpp \
                   Video/TheoraDecoder.cpp
endif
VIDEO_SOURCES += Video/ViewerComponent.cpp

$(call LIBOBJNAMES,$(VIDEO_SOURCES)): | $(DEPDIR)/config

VIDEO_PACKAGES = MYGLMOTIF MYIMAGES MYGLSUPPORT MYGLWRAPPERS MYGEOMETRY MYMATH MYIO MYTHREADS MYMISC GL
ifneq ($(SYSTEM_HAVE_LIBJPEG),0)
  VIDEO_PACKAGES += JPEG
endif
ifneq ($(SYSTEM_HAVE_V4L2),0)
  VIDEO_PACKAGES += V4L2
endif
ifneq ($(SYSTEM_HAVE_DC1394),0)
  VIDEO_PACKAGES += DC1394
endif
ifneq ($(SYSTEM_HAVE_THEORA),0)
  VIDEO_PACKAGES += THEORA OGG
endif
$(call LIBRARYNAME,libVideo): PACKAGES = $(VIDEO_PACKAGES)
$(call LIBRARYNAME,libVideo): EXTRACINCLUDEFLAGS += $(MYVIDEO_INCLUDE)
$(call LIBRARYNAME,libVideo): | $(call DEPENDENCIES,$(VIDEO_PACKAGES))
$(call LIBRARYNAME,libVideo): $(call LIBOBJNAMES,$(VIDEO_SOURCES))
.PHONY: libVideo
libVideo: $(call LIBRARYNAME,libVideo)

#
# The OpenAL Support Library (ALSupport)
#

$(DEPDIR)/Configure-ALSupport: $(DEPDIR)/Configure-Video
ifneq ($(SYSTEM_HAVE_OPENAL),0)
	@echo "Spatial sound using OpenAL enabled"
else
	@echo "Spatial sound using OpenAL disabled"
endif
	@cp AL/Config.h.template AL/Config.h.temp
	@$(call CONFIG_SETVAR,AL/Config.h.temp,ALSUPPORT_CONFIG_HAVE_OPENAL,$(SYSTEM_HAVE_OPENAL))
	@if ! diff -qN AL/Config.h.temp AL/Config.h > /dev/null ; then cp AL/Config.h.temp AL/Config.h ; fi
	@rm AL/Config.h.temp
	@touch $(DEPDIR)/Configure-ALSupport

ALSUPPORT_HEADERS = $(wildcard AL/*.h) \
                    $(wildcard AL/*.icpp)

ALSUPPORT_SOURCES = $(wildcard AL/*.cpp) \
                    $(wildcard AL/Internal/*.cpp)

$(call LIBOBJNAMES,$(ALSUPPORT_SOURCES)): | $(DEPDIR)/config

ALSUPPORT_PACKAGES = MYGEOMETRY MYTHREADS MYMISC
ifneq ($(SYSTEM_HAVE_OPENAL),0)
  ALSUPPORT_PACKAGES += OPENAL
endif
$(call LIBRARYNAME,libALSupport): PACKAGES = $(ALSUPPORT_PACKAGES)
$(call LIBRARYNAME,libALSupport): EXTRACINCLUDEFLAGS += $(MYALSUPPORT_INCLUDE)
$(call LIBRARYNAME,libALSupport): | $(call DEPENDENCIES,$(ALSUPPORT_PACKAGES))
$(call LIBRARYNAME,libALSupport): $(call LIBOBJNAMES,$(ALSUPPORT_SOURCES))
.PHONY: libALSupport
libALSupport: $(call LIBRARYNAME,libALSupport)

#
# The Scene Graph Rendering Library (SceneGraph)
#

$(DEPDIR)/Configure-SceneGraph: $(DEPDIR)/Configure-ALSupport
	@echo "Default SceneGraph shader directory:" $(SHAREINSTALLDIR)/Shaders/SceneGraph
ifneq ($(SYSTEM_HAVE_FREETYPE),0)
	@echo FreeType library exists on system
ifneq ($(SCENEGRAPH_USE_FANCYTEXT),0)
	@echo Core TrueType font directory: $(SYSTEM_COREFONTDIR)
else
	@echo Core TrueType Sans/Serif/Mono font families not found
endif
else
	@echo FreeType library does not exist on system
endif
ifneq ($(SCENEGRAPH_USE_FANCYTEXT),0)
	@echo Fancy font rendering in simple scene graph renderer enabled
else
	@echo Fancy font rendering in simple scene graph renderer disabled
endif
	@cp SceneGraph/Config.h.template SceneGraph/Config.h.temp
	@$(call CONFIG_SETSTRINGVAR,SceneGraph/Config.h.temp,SCENEGRAPH_CONFIG_SHADERDIR,$(SHAREINSTALLDIR)/Shaders/SceneGraph)
	@$(call CONFIG_SETVAR,SceneGraph/Config.h.temp,SCENEGRAPH_CONFIG_HAVE_FANCYTEXT,$(SCENEGRAPH_USE_FANCYTEXT))
ifneq ($(SCENEGRAPH_USE_FANCYTEXT),0)
	@$(call CONFIG_SETSTRINGVAR,SceneGraph/Config.h.temp,SCENEGRAPH_CONFIG_FONTDIR,$(SYSTEM_COREFONTDIR))
endif
	@if ! diff -qN SceneGraph/Config.h.temp SceneGraph/Config.h > /dev/null ; then cp SceneGraph/Config.h.temp SceneGraph/Config.h ; fi
	@rm SceneGraph/Config.h.temp
	@cp SceneGraph/Internal/Config.h.template SceneGraph/Internal/Config.h.temp
	@$(call CONFIG_SETSTRINGVAR,SceneGraph/Internal/Config.h.temp,SCENEGRAPH_CONFIG_DOOM3MATERIALMANAGER_SHADERDIR,$(SHAREINSTALLDIR)/Shaders/SceneGraph)
	@if ! diff -qN SceneGraph/Internal/Config.h.temp SceneGraph/Internal/Config.h > /dev/null ; then cp SceneGraph/Internal/Config.h.temp SceneGraph/Internal/Config.h ; fi
	@rm SceneGraph/Internal/Config.h.temp
	@touch $(DEPDIR)/Configure-SceneGraph

SCENEGRAPH_IGNORE_HEADERS = SceneGraph/FancyFontStyleNode.h \
                            SceneGraph/FancyTextNode.h

SCENEGRAPH_HEADERS = $(filter-out $(SCENEGRAPH_IGNORE_HEADERS),$(wildcard SceneGraph/*.h)) \
                     $(wildcard SceneGraph/*.icpp)
ifneq ($(SCENEGRAPH_USE_FANCYTEXT),0)
  SCENEGRAPH_HEADERS += SceneGraph/FancyFontStyleNode.h \
                        SceneGraph/FancyTextNode.h
endif

SCENEGRAPH_IGNORE_SOURCES = SceneGraph/FancyFontStyleNode.cpp \
                            SceneGraph/FancyTextNode.cpp

SCENEGRAPH_SOURCES = $(filter-out $(SCENEGRAPH_IGNORE_SOURCES),$(wildcard SceneGraph/*.cpp)) \
                     $(wildcard SceneGraph/Internal/*.cpp)
ifneq ($(SCENEGRAPH_USE_FANCYTEXT),0)
  SCENEGRAPH_SOURCES += SceneGraph/FancyFontStyleNode.cpp \
                        SceneGraph/FancyTextNode.cpp
endif

$(call LIBOBJNAMES,$(SCENEGRAPH_SOURCES)): | $(DEPDIR)/config

SCENEGRAPH_PACKAGES = MYALSUPPORT MYSOUND MYIMAGES MYGLMOTIF MYGLGEOMETRY MYGLSUPPORT MYGLWRAPPERS MYGEOMETRY MYMATH MYIO MYTHREADS MYMISC GL
ifneq ($(SCENEGRAPH_USE_FANCYTEXT),0)
  SCENEGRAPH_PACKAGES += FREETYPE
endif
ifneq ($(SYSTEM_HAVE_OPENAL),0)
  SCENEGRAPH_PACKAGES += OPENAL
endif
$(call LIBRARYNAME,libSceneGraph): PACKAGES = $(SCENEGRAPH_PACKAGES)
$(call LIBRARYNAME,libSceneGraph): EXTRACINCLUDEFLAGS += $(MYSCENEGRAPH_INCLUDE)
$(call LIBRARYNAME,libSceneGraph): | $(call DEPENDENCIES,$(SCENEGRAPH_PACKAGES))
$(call LIBRARYNAME,libSceneGraph): $(call LIBOBJNAMES,$(SCENEGRAPH_SOURCES))
.PHONY: libSceneGraph
libSceneGraph: $(call LIBRARYNAME,libSceneGraph)

#
# The Vrui Virtual Reality User Interface Library (Vrui)
#

$(DEPDIR)/Configure-Vrui: $(DEPDIR)/Configure-SceneGraph
ifneq ($(VRUI_VRWINDOW_USE_SWAPGROUPS),0)
	@echo "Swapgroup support for Vrui windows enabled"
else
	@echo "Swapgroup support for Vrui windows disabled"
endif
ifneq ($(SYSTEM_HAVE_LIBDBUS),0)
	@echo "Support for screen saver inhibition via DBus enabled"
else
	@echo "Support for screen saver inhibition via DBus disabled"
endif
ifneq ($(SYSTEM_HAVE_LIBPNG),0)
	@echo "Vrui will save screenshots in PNG format"
else
	@echo "Vrui will save screenshots in PPM format"
endif
ifneq ($(SYSTEM_HAVE_OPENAL),0)
	@echo "Support for spatial audio enabled"
else
	@echo "Support for spatial audio disabled"
endif
ifneq ($(SYSTEM_HAVE_THEORA),0)
	@echo "Support to save screen captures in Ogg/Theora format enabled"
else
	@echo "Support to save screen captures in Ogg/Theora format disabled"
endif
ifneq ($(SYSTEM_HAVE_XINPUT2),0)
	@echo "Support for multitouch screens enabled"
else
	@echo "Support for multitouch screens disabled (missing XINPUT2 library)"
endif
ifneq ($(SYSTEM_HAVE_XRANDR),0)
	@echo "Support for named video outputs enabled"
else
	@echo "Support for named video outputs disabled (missing XRANDR library)"
endif
ifneq ($(SYSTEM_HAVE_LIBUDEV),0)
	@echo "LatencyTester vislet for Oculus DK1 latency tester enabled"
else
	@echo "LatencyTester vislet disabled (missing udev library)"
endif
ifneq ($(VRUI_READUSERCONFIGFILE),0)
	@echo "Using global per-user configuration file" $(VRUI_USERCONFIGDIR)/Vrui.cfg
else
	@echo "Global per-user configuration disabled"
endif
# Adjust file paths in Vrui/Internal/Config.h:
	@cp Vrui/Internal/Config.h.template Vrui/Internal/Config.h.temp
	@$(call CONFIG_SETVAR,Vrui/Internal/Config.h.temp,VRUI_INTERNAL_CONFIG_HAVE_XRANDR,$(SYSTEM_HAVE_XRANDR))
	@$(call CONFIG_SETVAR,Vrui/Internal/Config.h.temp,VRUI_INTERNAL_CONFIG_HAVE_XINPUT2,$(SYSTEM_HAVE_XINPUT2))
	@$(call CONFIG_SETVAR,Vrui/Internal/Config.h.temp,VRUI_INTERNAL_CONFIG_HAVE_LIBDBUS,$(SYSTEM_HAVE_LIBDBUS))
	@$(call CONFIG_SETVAR,Vrui/Internal/Config.h.temp,VRUI_INTERNAL_CONFIG_VRWINDOW_USE_SWAPGROUPS,$(VRUI_VRWINDOW_USE_SWAPGROUPS))
	@$(call CONFIG_SETVAR,Vrui/Internal/Config.h.temp,VRUI_INTERNAL_CONFIG_INPUT_H_HAS_STRUCTS,$(LINUX_INPUT_H_HAS_STRUCTS))
	@$(call CONFIG_SETSTRINGVAR,Vrui/Internal/Config.h.temp,VRUI_INTERNAL_CONFIG_LIBDIR_DEBUG,$(LIBINSTALLDIR_DEBUG))
	@$(call CONFIG_SETSTRINGVAR,Vrui/Internal/Config.h.temp,VRUI_INTERNAL_CONFIG_LIBDIR_RELEASE,$(LIBINSTALLDIR_RELEASE))
	@$(call CONFIG_SETSTRINGVAR,Vrui/Internal/Config.h.temp,VRUI_INTERNAL_CONFIG_EXECUTABLEDIR_DEBUG,$(EXECUTABLEINSTALLDIR_DEBUG))
	@$(call CONFIG_SETSTRINGVAR,Vrui/Internal/Config.h.temp,VRUI_INTERNAL_CONFIG_EXECUTABLEDIR_RELEASE,$(EXECUTABLEINSTALLDIR_RELEASE))
	@$(call CONFIG_SETSTRINGVAR,Vrui/Internal/Config.h.temp,VRUI_INTERNAL_CONFIG_PLUGINDIR_DEBUG,$(PLUGININSTALLDIR_DEBUG))
	@$(call CONFIG_SETSTRINGVAR,Vrui/Internal/Config.h.temp,VRUI_INTERNAL_CONFIG_PLUGINDIR_RELEASE,$(PLUGININSTALLDIR_RELEASE))
	@$(call CONFIG_SETSTRINGVAR,Vrui/Internal/Config.h.temp,VRUI_INTERNAL_CONFIG_TOOLDIR_DEBUG,$(PLUGININSTALLDIR_DEBUG)/$(VRTOOLSDIREXT))
	@$(call CONFIG_SETSTRINGVAR,Vrui/Internal/Config.h.temp,VRUI_INTERNAL_CONFIG_TOOLDIR_RELEASE,$(PLUGININSTALLDIR_RELEASE)/$(VRTOOLSDIREXT))
	@$(call CONFIG_SETSTRINGVAR,Vrui/Internal/Config.h.temp,VRUI_INTERNAL_CONFIG_VISLETDIR_DEBUG,$(PLUGININSTALLDIR_DEBUG)/$(VRVISLETSDIREXT))
	@$(call CONFIG_SETSTRINGVAR,Vrui/Internal/Config.h.temp,VRUI_INTERNAL_CONFIG_VISLETDIR_RELEASE,$(PLUGININSTALLDIR_RELEASE)/$(VRVISLETSDIREXT))
	@$(call CONFIG_SETSTRINGVAR,Vrui/Internal/Config.h.temp,VRUI_INTERNAL_CONFIG_TOOLNAMETEMPLATE,lib%s.$(PLUGINFILEEXT))
	@$(call CONFIG_SETSTRINGVAR,Vrui/Internal/Config.h.temp,VRUI_INTERNAL_CONFIG_VISLETNAMETEMPLATE,lib%s.$(PLUGINFILEEXT))
	@$(call CONFIG_SETSTRINGVAR,Vrui/Internal/Config.h.temp,VRUI_INTERNAL_CONFIG_CONFIGFILENAME,Vrui)
	@$(call CONFIG_SETSTRINGVAR,Vrui/Internal/Config.h.temp,VRUI_INTERNAL_CONFIG_CONFIGFILESUFFIX,.cfg)
	@$(call CONFIG_SETSTRINGVAR,Vrui/Internal/Config.h.temp,VRUI_INTERNAL_CONFIG_SYSCONFIGDIR,$(ETCINSTALLDIR))
	@$(call CONFIG_SETVAR,Vrui/Internal/Config.h.temp,VRUI_INTERNAL_CONFIG_HAVE_USERCONFIGFILE,$(VRUI_READUSERCONFIGFILE))
	@$(call CONFIG_SETSTRINGVAR,Vrui/Internal/Config.h.temp,VRUI_INTERNAL_CONFIG_USERCONFIGDIR,$(VRUI_USERCONFIGDIR))
	@$(call CONFIG_SETVAR,Vrui/Internal/Config.h.temp,VRUI_INTERNAL_CONFIG_VERSION,$(PROJECT_NUMERICVERSION))
	@$(call CONFIG_SETSTRINGVAR,Vrui/Internal/Config.h.temp,VRUI_INTERNAL_CONFIG_ETCDIR,$(ETCINSTALLDIR))
	@$(call CONFIG_SETSTRINGVAR,Vrui/Internal/Config.h.temp,VRUI_INTERNAL_CONFIG_SHAREDIR,$(SHAREINSTALLDIR))
	@if ! diff -qN Vrui/Internal/Config.h.temp Vrui/Internal/Config.h > /dev/null ; then cp Vrui/Internal/Config.h.temp Vrui/Internal/Config.h ; fi
	@rm Vrui/Internal/Config.h.temp
	@touch $(DEPDIR)/Configure-Vrui

VRUI_HEADERS = $(wildcard Vrui/*.h) \
               $(wildcard Vrui/*.icpp)

# Ignore the following sources, which are either unfinished, obsolete, or specifically requested later:
VRUI_IGNORE_SOURCES = Vrui/Internal/InputDeviceDock.cpp \
                      Vrui/Internal/TheoraMovieSaver.cpp \
                      Vrui/Internal/Linux/ScreenSaverInhibitorDBus.cpp

# Temporarily ignore some old VRWindow subclasses:
VRUI_IGNORE_SOURCES += Vrui/Internal/LensCorrector.cpp \
                       Vrui/Internal/VRWindowCombined.cpp

VRUI_SOURCES = $(filter-out $(VRUI_IGNORE_SOURCES),$(wildcard Vrui/*.cpp) \
                                                   $(wildcard Vrui/Internal/*.cpp) \
                                                   $(wildcard Vrui/Internal/$(OSSPECFILEINSERT)/*.cpp))
ifneq ($(SYSTEM_HAVE_THEORA),0)
  VRUI_SOURCES += Vrui/Internal/TheoraMovieSaver.cpp
endif
ifneq ($(SYSTEM_HAVE_LIBDBUS),0)
  VRUI_SOURCES += Vrui/Internal/Linux/ScreenSaverInhibitorDBus.cpp
endif

$(call LIBOBJNAMES,$(VRUI_SOURCES)): | $(DEPDIR)/config

VRUI_PACKAGES = MYSCENEGRAPH MYALSUPPORT MYVIDEO MYSOUND MYGLMOTIF MYIMAGES MYGLGEOMETRY MYGLXSUPPORT MYGLSUPPORT MYGLWRAPPERS MYGEOMETRY MYMATH MYCLUSTER MYCOMM MYIO MYRAWHID MYTHREADS MYPLUGINS MYREALTIME MYMISC GL X11 RT
ifneq ($(SYSTEM_HAVE_OPENAL),0)
  VRUI_PACKAGES += OPENAL
endif
ifneq ($(SYSTEM_HAVE_XINPUT2),0)
  VRUI_PACKAGES += XINPUT2
endif
ifneq ($(SYSTEM_HAVE_XRANDR),0)
  VRUI_PACKAGES += XRANDR
endif
ifneq ($(SYSTEM_HAVE_LIBDBUS),0)
  VRUI_PACKAGES += LIBDBUS
endif
ifeq ($(SYSTEM),DARWIN)
  VRUI_PACKAGES += MYMACOSX IOKIT COREFOUNDATION
endif
$(call LIBRARYNAME,libVrui): PACKAGES = $(VRUI_PACKAGES)
$(call LIBRARYNAME,libVrui): EXTRACINCLUDEFLAGS += $(MYVRUI_INCLUDE)
$(call LIBRARYNAME,libVrui): | $(call DEPENDENCIES,$(VRUI_PACKAGES))
$(call LIBRARYNAME,libVrui): $(call LIBOBJNAMES,$(VRUI_SOURCES))
.PHONY: libVrui
libVrui: $(call LIBRARYNAME,libVrui)

#
# The Vrui VR tool plug-in hierarchy:
#

# Implicit rule for creating VR tool plug-ins:
$(call VRTOOLNAMES,%): PACKAGES = MYVRUI MYGLGEOMETRY MYGLSUPPORT MYGLWRAPPERS MYGEOMETRY MYMATH MYMISC GL
$(call VRTOOLNAMES,%): $(call PLUGINOBJNAMES,Vrui/Tools/%.cpp)
	@mkdir -p $(VRTOOLSDIR)
ifdef SHOWCOMMAND
	$(CCOMP) $(PLUGINLINKFLAGS) -o $@ $^ $(PLUGINLFLAGS)
else
	@echo Linking $@...
	@$(CCOMP) $(PLUGINLINKFLAGS) -o $@ $^ $(PLUGINLFLAGS)
endif

$(call PLUGINOBJNAMES,$(VRTOOLS_SOURCES)): | $(DEPDIR)/config

# Dependencies between Vrui tools and other libraries:
$(call VRTOOLNAMES,WalkNavigationTool): PACKAGES += MYSCENEGRAPH
$(call VRTOOLNAMES,WalkSurfaceNavigationTool): PACKAGES += MYSCENEGRAPH
$(call VRTOOLNAMES,IndexHandTool): PACKAGES += MYSCENEGRAPH MYIO
$(call VRTOOLNAMES,MouseDialogNavigationTool): PACKAGES += MYGLMOTIF
$(call VRTOOLNAMES,ViewpointFileNavigationTool): PACKAGES += MYGLMOTIF MYIO
$(call VRTOOLNAMES,PanelButtonTool): PACKAGES += MYGLMOTIF
$(call VRTOOLNAMES,TouchWidgetTool): PACKAGES += MYGLMOTIF
$(call VRTOOLNAMES,RayMenuTool): PACKAGES += MYGLMOTIF
$(call VRTOOLNAMES,PanelMenuTool): PACKAGES += MYGLMOTIF
$(call VRTOOLNAMES,LookMenuTool): PACKAGES += MYGLMOTIF
$(call VRTOOLNAMES,DaisyWheelTool): PACKAGES += MYGLMOTIF
$(call VRTOOLNAMES,QuikWriteTool): PACKAGES += MYGLMOTIF
$(call VRTOOLNAMES,JediTool): PACKAGES += MYALSUPPORT MYIMAGES MYIO
ifneq ($(SYSTEM_HAVE_OPENAL),0)
  $(call VRTOOLNAMES,JediTool): PACKAGES += OPENAL
endif
$(call VRTOOLNAMES,MeasurementTool): PACKAGES += MYGLMOTIF
$(call VRTOOLNAMES,ScreenshotTool): PACKAGES += MYIO
$(call VRTOOLNAMES,SketchingTool): PACKAGES += MYGLMOTIF MYIO
$(call VRTOOLNAMES,AnnotationTool): PACKAGES += MYGLMOTIF
$(call VRTOOLNAMES,ViewpointSaverTool): PACKAGES += MYIO
$(call VRTOOLNAMES,CurveEditorTool): PACKAGES += MYGLMOTIF MYIO
$(call VRTOOLNAMES,ScriptExecutorTool): PACKAGES += MYGLMOTIF

# Mark all VR tool object files as intermediate:
.SECONDARY: $(call PLUGINOBJNAMES,$(VRTOOLS_SOURCES))

.PHONY: VRTools
VRTools: $(VRTOOLS)

#
# The Vrui vislet plug-in hierarchy:
#

# Implicit rule for creating vislet plug-ins:
$(call VRVISLETNAMES,%): PACKAGES = MYVRUI MYGLGEOMETRY MYGLSUPPORT MYGLWRAPPERS MYGEOMETRY MYMATH MYMISC GL
$(call VRVISLETNAMES,%): $(call PLUGINOBJNAMES,Vrui/Vislets/%.cpp)
	@mkdir -p $(VRVISLETSDIR)
ifdef SHOWCOMMAND
	$(CCOMP) $(PLUGINLINKFLAGS) -o $@ $^ $(PLUGINLFLAGS)
else
	@echo Linking $@...
	@$(CCOMP) $(PLUGINLINKFLAGS) -o $@ $^ $(PLUGINLFLAGS)
endif

# Dependencies between Vrui vislets and other libraries:
$(call VRVISLETNAMES,DeviceRenderer): PACKAGES += MYSCENEGRAPH MYIO
$(call VRVISLETNAMES,SceneGraphViewer): PACKAGES += MYSCENEGRAPH MYIO
$(call VRVISLETNAMES,CAVERenderer): PACKAGES += MYSCENEGRAPH MYIMAGES MYIO
$(call VRVISLETNAMES,ImageViewer): PACKAGES += MYGLMOTIF MYIMAGES
$(call VRVISLETNAMES,ViewerConfiguration): PACKAGES += MYGLMOTIF
$(call VRVISLETNAMES,Filming): PACKAGES += MYGLMOTIF MYCLUSTER MYIO
$(call VRVISLETNAMES,LatencyTester): PACKAGES += MYRAWHID MYTHREADS
$(call VRVISLETNAMES,HMDCameraViewer): PACKAGES += MYVIDEO MYIMAGES MYIO MYTHREADS MYREALTIME

$(call PLUGINOBJNAMES,$(VRVISLETS_SOURCES)): | $(DEPDIR)/config

# Mark all vislet object files as intermediate:
.SECONDARY: $(call PLUGINOBJNAMES,$(VRVISLETS_SOURCES))

.PHONY: VRVislets
VRVislets: $(VRVISLETS)

#
# Vrui resources:
#

$(VRUI_SHAREDIR)/Resources/ViveController.wrl: | $(DEPDIR)/config
$(VRUI_SHAREDIR)/Resources/ViveController.wrl: $(VRUI_SHAREDIR)/Resources/ViveController.wrl.template $(DEPDIR)/Configure-Vrui
	@echo Configuring Vive controller model...
	@sed -e "s:STEAMVRRENDERMODELSDIR:$(STEAMVRRENDERMODELSDIR):" $(VRUI_SHAREDIR)/Resources/ViveController.wrl.template > $(VRUI_SHAREDIR)/Resources/ViveController.wrl

$(VRUI_SHAREDIR)/Resources/IndexControllerLeft.wrl: | $(DEPDIR)/config
$(VRUI_SHAREDIR)/Resources/IndexControllerLeft.wrl: $(VRUI_SHAREDIR)/Resources/IndexControllerLeft.wrl.template $(DEPDIR)/Configure-Vrui
	@echo Configuring left Valve Index controller model...
	@sed -e "s:STEAMVRINDEXCONTROLLERRENDERMODELSDIR:$(STEAMVRINDEXCONTROLLERRENDERMODELSDIR):" $(VRUI_SHAREDIR)/Resources/IndexControllerLeft.wrl.template > $(VRUI_SHAREDIR)/Resources/IndexControllerLeft.wrl

$(VRUI_SHAREDIR)/Resources/IndexControllerRight.wrl: | $(DEPDIR)/config
$(VRUI_SHAREDIR)/Resources/IndexControllerRight.wrl: $(VRUI_SHAREDIR)/Resources/IndexControllerRight.wrl.template $(DEPDIR)/Configure-Vrui
	@echo Configuring right Valve Index controller model...
	@sed -e "s:STEAMVRINDEXCONTROLLERRENDERMODELSDIR:$(STEAMVRINDEXCONTROLLERRENDERMODELSDIR):" $(VRUI_SHAREDIR)/Resources/IndexControllerRight.wrl.template > $(VRUI_SHAREDIR)/Resources/IndexControllerRight.wrl

#
# The VR device driver daemon:
#

$(DEPDIR)/Configure-VRDeviceDaemon: $(DEPDIR)/Configure-Vrui
ifneq ($(VRDEVICES_USE_INPUT_ABSTRACTION),0)
	@echo "Event device support for joysticks enabled"
  ifneq ($(LINUX_INPUT_H_HAS_STRUCTS),0)
	@echo "input.h contains event structure definitions"
  else
	@echo "input.h does not contain event structure definitions"
  endif
else
	@echo "Event device support for joysticks disabled"
endif
ifneq ($(VRDEVICES_USE_BLUETOOTH),0)
	@echo "Bluetooth support (for Nintendo Wii controller) enabled"
else
	@echo "Bluetooth support (for Nintendo Wii controller) disabled"
endif
ifneq ($(SYSTEM_HAVE_LIBUSB1),0)
	@echo "USB support (for Razer Hydra and Oculus Rift tracker) enabled"
else
	@echo "USB support (for Razer Hydra and Oculus Rift tracker) disabled"
endif
ifneq ($(SYSTEM_HAVE_OPENVR),0)
	@echo "OpenVR SDK and SteamVR run-time exist on host system; support for SteamVR-compatible VR headsets enabled"
	@echo "SteamVR run-time root directory: $(STEAMVRDIR)"
	@echo "SteamVR run-time library directories: $(STEAMRUNTIMEDIR1) $(STEAMRUNTIMEDIR2)"
	@echo "SteamVR Lighthouse driver directory: $(STEAMVRDRIVERDIR)"
else
	@echo "OpenVR SDK or SteamVR run-time do not exist on host system; support for SteamVR-compatible VR headsets disabled"
endif
	@cp VRDeviceDaemon/Config.h.template VRDeviceDaemon/Config.h.temp
	@$(call CONFIG_SETSTRINGVAR,VRDeviceDaemon/Config.h.temp,VRDEVICEDAEMON_CONFIG_VRDEVICESDIR_DEBUG,$(PLUGININSTALLDIR_DEBUG)/$(VRDEVICESDIREXT))
	@$(call CONFIG_SETSTRINGVAR,VRDeviceDaemon/Config.h.temp,VRDEVICEDAEMON_CONFIG_VRDEVICESDIR_RELEASE,$(PLUGININSTALLDIR_RELEASE)/$(VRDEVICESDIREXT))
	@$(call CONFIG_SETSTRINGVAR,VRDeviceDaemon/Config.h.temp,VRDEVICEDAEMON_CONFIG_VRCALIBRATORSDIR_DEBUG,$(PLUGININSTALLDIR_DEBUG)/$(VRCALIBRATORSDIREXT))
	@$(call CONFIG_SETSTRINGVAR,VRDeviceDaemon/Config.h.temp,VRDEVICEDAEMON_CONFIG_VRCALIBRATORSDIR_RELEASE,$(PLUGININSTALLDIR_RELEASE)/$(VRCALIBRATORSDIREXT))
	@$(call CONFIG_SETSTRINGVAR,VRDeviceDaemon/Config.h.temp,VRDEVICEDAEMON_CONFIG_DSONAMETEMPLATE,lib%s.$(PLUGINFILEEXT))
	@$(call CONFIG_SETSTRINGVAR,VRDeviceDaemon/Config.h.temp,VRDEVICEDAEMON_CONFIG_CONFIGFILEDIR,$(ETCINSTALLDIR))
	@$(call CONFIG_SETSTRINGVAR,VRDeviceDaemon/Config.h.temp,VRDEVICEDAEMON_CONFIG_CONFIGDIR,$(ETCINSTALLDIR)/VRDeviceDaemon)
	@$(call CONFIG_SETVAR,VRDeviceDaemon/Config.h.temp,VRDEVICEDAEMON_CONFIG_INPUT_H_HAS_STRUCTS,$(LINUX_INPUT_H_HAS_STRUCTS))
	@if ! diff -qN VRDeviceDaemon/Config.h.temp VRDeviceDaemon/Config.h > /dev/null ; then cp VRDeviceDaemon/Config.h.temp VRDeviceDaemon/Config.h ; fi
	@rm VRDeviceDaemon/Config.h.temp
ifneq ($(SYSTEM_HAVE_OPENVR),0)
	@cp VRDeviceDaemon/VRDevices/OpenVRHost-Config.h.template VRDeviceDaemon/VRDevices/OpenVRHost-Config.h.temp
	@$(call CONFIG_SETSTRINGVAR,VRDeviceDaemon/VRDevices/OpenVRHost-Config.h.temp,VRDEVICEDAEMON_CONFIG_OPENVRHOST_STEAMDIR,$(subst $(HOME),$$HOME,$(STEAMDIR)))
	@$(call CONFIG_SETSTRINGVAR,VRDeviceDaemon/VRDevices/OpenVRHost-Config.h.temp,VRDEVICEDAEMON_CONFIG_OPENVRHOST_STEAMVRDIR,$(subst $(STEAMDIR)/,,$(realpath $(STEAMVRDIR))))
	@if ! diff -qN VRDeviceDaemon/VRDevices/OpenVRHost-Config.h.temp VRDeviceDaemon/VRDevices/OpenVRHost-Config.h > /dev/null ; then cp VRDeviceDaemon/VRDevices/OpenVRHost-Config.h.temp VRDeviceDaemon/VRDevices/OpenVRHost-Config.h ; fi
	@rm VRDeviceDaemon/VRDevices/OpenVRHost-Config.h.temp
endif	
	@touch $(DEPDIR)/Configure-VRDeviceDaemon

VRDEVICEDAEMONLIB_SOURCES = VRDeviceDaemon/VRDevice.cpp \
                            VRDeviceDaemon/VRCalibrator.cpp \
                            VRDeviceDaemon/VRDeviceManager.cpp \
                            Vrui/EnvironmentDefinition.cpp \
                            Vrui/Internal/VRDeviceState.cpp \
                            Vrui/Internal/VRDeviceDescriptor.cpp \
                            Vrui/Internal/VRBaseStation.cpp \
                            Vrui/Internal/HMDConfiguration.cpp \
                            Vrui/Internal/VRDeviceProtocol.cpp \
                            VRDeviceDaemon/VRDeviceServer.cpp

$(call LIBOBJNAMES,$(VRDEVICEDAEMONLIB_SOURCES)): | $(DEPDIR)/config

VRDEVICEDAEMONLIB_PACKAGES = MYGEOMETRY MYMATH MYCOMM MYIO MYTHREADS MYREALTIME MYMISC DL
$(call LIBRARYNAME,libVRDeviceDaemon): PACKAGES = $(VRDEVICEDAEMONLIB_PACKAGES)
$(call LIBRARYNAME,libVRDeviceDaemon): EXTRACINCLUDEFLAGS += $(MYVRUI_INCLUDE)
$(call LIBRARYNAME,libVRDeviceDaemon): CFLAGS += -DVERBOSE
$(call LIBRARYNAME,libVRDeviceDaemon): | $(call DEPENDENCIES,$(VRDEVICEDAEMONLIB_PACKAGES))
$(call LIBRARYNAME,libVRDeviceDaemon): $(call LIBOBJNAMES,$(VRDEVICEDAEMONLIB_SOURCES))
.PHONY: libVRDeviceDaemon
libVRDeviceDaemon: $(call LIBRARYNAME,libVRDeviceDaemon)

VRDEVICEDAEMONLIB_BASEDIR = $(VRUI_PACKAGEROOT)
VRDEVICEDAEMONLIB_DEPENDS = MYGEOMETRY MYCOMM MYIO MYTHREADS MYREALTIME MYMISC DL
VRDEVICEDAEMONLIB_DEPENDS_INLINE = MYCOMM MYIO DL
VRDEVICEDAEMONLIB_INCLUDE = -I$(VRUI_INCLUDEDIR)
VRDEVICEDAEMONLIB_LIBDIR  = -L$(VRUI_LIBDIR)
VRDEVICEDAEMONLIB_LIBS    = -lVRDeviceDaemon$(LDEXT)
VRDEVICEDAEMONLIB_RPATH   = $(VRUI_RPATH)

VRDEVICEDAEMON_SOURCES = VRDeviceDaemon/VRDeviceDaemon.cpp

$(VRDEVICEDAEMON_SOURCES:%.cpp=$(OBJDIR)/%.o): | $(DEPDIR)/config

VRDEVICEDAEMON_PACKAGES = VRDEVICEDAEMONLIB MYGEOMETRY MYMATH MYCOMM MYIO MYTHREADS MYREALTIME MYMISC DL
$(EXEDIR)/VRDeviceDaemon: PACKAGES = $(VRDEVICEDAEMON_PACKAGES)
$(EXEDIR)/VRDeviceDaemon: EXTRACINCLUDEFLAGS += $(MYVRUI_INCLUDE)
$(EXEDIR)/VRDeviceDaemon: CFLAGS += -DVERBOSE
$(EXEDIR)/VRDeviceDaemon: LINKFLAGS += $(PLUGINHOSTLINKFLAGS)
ifneq ($(SYSTEM_HAVE_OPENVR),0)
  # Add system library directory first to get current versions, not the outdated cruft in SteamVR:
  $(EXEDIR)/VRDeviceDaemon: LINKFLAGS += -Wl,-rpath,/usr/$(LIBEXT)
  
  # Add SteamVR library directories:
  $(EXEDIR)/VRDeviceDaemon: LINKFLAGS += -Wl,-rpath,$(STEAMRUNTIMEDIR1)
  ifneq ($(strip $(STEAMRUNTIMEDIR2)),)
    $(EXEDIR)/VRDeviceDaemon: LINKFLAGS += -Wl,-rpath,$(STEAMRUNTIMEDIR2)
  endif
  $(EXEDIR)/VRDeviceDaemon: LINKFLAGS += -Wl,-rpath,$(STEAMVRDRIVERDIR)
endif
$(EXEDIR)/VRDeviceDaemon: $(VRDEVICEDAEMON_SOURCES:%.cpp=$(OBJDIR)/%.o)
.PHONY: VRDeviceDaemon
VRDeviceDaemon: $(EXEDIR)/VRDeviceDaemon

#
# The VR device driver plug-ins:
#

# Implicit rule for creating device driver plug-ins:
$(call VRDEVICENAMES,%): PACKAGES = VRDEVICEDAEMONLIB MYGEOMETRY MYMATH MYCOMM MYTHREADS MYMISC
$(call VRDEVICENAMES,%): $(call PLUGINOBJNAMES,VRDeviceDaemon/VRDevices/%.cpp)
	@mkdir -p $(VRDEVICESDIR)
ifdef SHOWCOMMAND
	$(CCOMP) $(PLUGINLINKFLAGS) -o $@ $^ $(PLUGINLFLAGS)
else
	@echo Linking $@...
	@$(CCOMP) $(PLUGINLINKFLAGS) -o $@ $^ $(PLUGINLFLAGS)
endif

$(call PLUGINOBJNAMES,$(VRDEVICES_SOURCES) VRDeviceDaemon/VRDevices/VRPNConnection.cpp VRDeviceDaemon/VRDevices/Wiimote.cpp VRDeviceDaemon/VRDevices/RazerHydra.cpp): | $(DEPDIR)/config

# Dependencies between VR device driver plug-ins and other libraries:
ifeq ($(SYSTEM),DARWIN)
  $(call VRDEVICENAMES,HIDDevice): PACKAGES += OSX_IOKIT OSX_COREFOUNDATION OSX_SYSTEM
endif
$(call VRDEVICENAMES,VRPNClient): $(call PLUGINOBJNAMES,VRDeviceDaemon/VRDevices/VRPNConnection.cpp \
                                                        VRDeviceDaemon/VRDevices/VRPNClient.cpp)
$(call VRDEVICENAMES,WiimoteTracker): PACKAGES += BLUETOOTH
$(call VRDEVICENAMES,WiimoteTracker): $(call PLUGINOBJNAMES,VRDeviceDaemon/VRDevices/Wiimote.cpp \
                                                            VRDeviceDaemon/VRDevices/WiimoteTracker.cpp)
$(call VRDEVICENAMES,RazerHydraDevice): PACKAGES += MYUSB LIBUSB1
$(call VRDEVICENAMES,RazerHydraDevice): $(call PLUGINOBJNAMES,VRDeviceDaemon/VRDevices/RazerHydra.cpp \
                                                              VRDeviceDaemon/VRDevices/RazerHydraDevice.cpp)
$(call VRDEVICENAMES,OculusRift): PACKAGES += MYUSB MYIO LIBUSB1
$(call VRDEVICENAMES,OpenVRHost): PACKAGES += OPENVR
$(call VRDEVICENAMES,OpenVRHost): EXTRACINCLUDEFLAGS += -I$(OPENVR_BASEDIR)/headers
# $(call VRDEVICENAMES,OpenVRHost): CFLAGS += -DVERYVERBOSE

# Mark all VR device driver object files as intermediate:
.SECONDARY: $(call PLUGINOBJNAMES,$(VRDEVICES_SOURCES) VRDeviceDaemon/VRDevices/VRPNConnection.cpp VRDeviceDaemon/VRDevices/Wiimote.cpp VRDeviceDaemon/VRDevices/RazerHydra.cpp)

#
# The VR tracker calibrator plug-ins:
#

# Implicit rule for creating tracker calibrator plug-ins:
$(call VRCALIBRATORNAMES,%): PACKAGES = VRDEVICEDAEMONLIB MYGEOMETRY MYMATH MYTHREADS MYMISC
$(call VRCALIBRATORNAMES,%): $(call PLUGINOBJNAMES,VRDeviceDaemon/VRCalibrators/%.cpp)
	@mkdir -p $(VRCALIBRATORSDIR)
ifdef SHOWCOMMAND
	$(CCOMP) $(PLUGINLINKFLAGS) -o $@ $^ $(PLUGINLFLAGS)
else
	@echo Linking $@...
	@$(CCOMP) $(PLUGINLINKFLAGS) -o $@ $^ $(PLUGINLFLAGS)
endif

$(call PLUGINOBJNAMES,$(VRCALIBRATORS_SOURCES)): | $(DEPDIR)/config

# Mark all VR calibrator object files as intermediate:
.SECONDARY: $(call PLUGINOBJNAMES,$(VRCALIBRATORS_SOURCES))

.PHONY: VRCalibrators
VRCalibrators: $(VRCALIBRATORS)

#
# The OpenVRHost device driver module helper script:
#

ifneq ($(SYSTEM_HAVE_OPENVR),0)
$(EXEDIR)/RunOpenVRTracker.sh: | $(DEPDIR)/config
$(EXEDIR)/RunOpenVRTracker.sh: VRDeviceDaemon/VRDevices/OpenVRHost-Config.h
	@echo Creating helper script to run OpenVRHost tracking device driver...
	@cp $(VRUI_SCRIPTDIR)/RunOpenVRTracker.sh $(EXEDIR)/RunOpenVRTracker.sh
	@sed -i -e 's@STEAMDIR=.*@STEAMDIR=$(subst $(HOME),$$HOME,$(STEAMDIR))@' $(EXEDIR)/RunOpenVRTracker.sh
	@sed -i -e 's@RUNTIMEDIR1=.*@RUNTIMEDIR1=$(subst $(STEAMDIR),$$STEAMDIR,$(STEAMRUNTIMEDIR1))@' $(EXEDIR)/RunOpenVRTracker.sh
	@sed -i -e 's@RUNTIMEDIR2=.*@RUNTIMEDIR2=$(subst $(STEAMDIR),$$STEAMDIR,$(STEAMRUNTIMEDIR2))@' $(EXEDIR)/RunOpenVRTracker.sh
	@sed -i -e 's@STEAMVRDIR=.*@STEAMVRDIR=$(subst $(STEAMDIR),$$STEAMDIR,$(STEAMVRDIR))@' $(EXEDIR)/RunOpenVRTracker.sh
	@sed -i -e 's@VRUIBINDIR=.*@VRUIBINDIR=$(EXECUTABLEINSTALLDIR)@' $(EXEDIR)/RunOpenVRTracker.sh
	@sed -i -e 's@VRUIETCDIR=.*@VRUIETCDIR=$(ETCINSTALLDIR)@' $(EXEDIR)/RunOpenVRTracker.sh
	@chmod a+x $(EXEDIR)/RunOpenVRTracker.sh
endif

#
# The Vrui VR Compositing Server
#

$(DEPDIR)/Configure-VRCompositingServer: $(DEPDIR)/Configure-VRDeviceDaemon
ifneq ($(SYSTEM_HAVE_VULKAN),0)
	@echo "Vulkan 3D graphics API exists on host system; Vrui VR Compositing Server enabled"
	@echo "Vulkan shader directory: $(SHAREINSTALLDIR)/spirv"
	@cp VRCompositingServer/Config.h.template VRCompositingServer/Config.h.temp
	@$(call CONFIG_SETSTRINGVAR,VRCompositingServer/Config.h.temp,VRCOMPOSITOR_SHADERDIR,$(SHAREINSTALLDIR)/spirv)
	@if ! diff -qN VRCompositingServer/Config.h.temp VRCompositingServer/Config.h > /dev/null ; then cp VRCompositingServer/Config.h.temp VRCompositingServer/Config.h ; fi
	@rm VRCompositingServer/Config.h.temp
else
	@echo "Vulkan 3D graphics API does not exist on host system; Vrui VR Compositing Server disabled"
endif
	@touch $(DEPDIR)/Configure-VRCompositingServer

VRCOMPOSITINGSERVER_SOURCES = Vrui/EnvironmentDefinition.cpp \
                              Vrui/Internal/VRDeviceDescriptor.cpp \
                              Vrui/Internal/VRBaseStation.cpp \
                              Vrui/Internal/HMDConfiguration.cpp \
                              Vrui/Internal/VRDeviceState.cpp \
                              Vrui/Internal/VRDeviceProtocol.cpp \
                              Vrui/Internal/VRDeviceClient.cpp \
                              Vrui/Internal/VRCompositorProtocol.cpp \
                              VRCompositingServer/VblankEstimator.cpp \
                              VRCompositingServer/HMD.cpp \
                              VRCompositingServer/LatencyTester.cpp \
                              VRCompositingServer/BoundaryRenderer.cpp \
                              VRCompositingServer/VRCompositor.cpp \
                              VRCompositingServer/VRServer.cpp

$(VRCOMPOSITINGSERVER_SOURCES:%.cpp=$(OBJDIR)/%.o): | $(DEPDIR)/config

$(EXEDIR)/VRCompositingServer: PACKAGES += MYIMAGES MYVULKANXLIB MYVULKAN MYGEOMETRY MYMATH MYCOMM MYIO MYRAWHID MYREALTIME MYMISC
$(EXEDIR)/VRCompositingServer: $(VRCOMPOSITINGSERVER_SOURCES:%.cpp=$(OBJDIR)/%.o)
.PHONY: VRCompositingServer
VRCompositingServer: $(EXEDIR)/VRCompositingServer

#
# Build rules for shader modules from GLSL to SPIR-V
#

GLSLC = glslc

#
# Set default compile commands from .vs/.fs to .spv:
#

$(SPIRVVERTEXDIR)/%.spv: $(VRCOMPOSITINGSERVER_SHADERDIR)/%.vert
	@mkdir -p $(SPIRVVERTEXDIR)/$(*D)
ifdef SHOWCOMMAND
	$(GLSLC) -o $@ $(GLSLCFLAGS) $<
else
	@echo Compiling $<...
	@$(GLSLC) -o $@ $(GLSLCFLAGS) $<
endif

$(SPIRVFRAGMENTDIR)/%.spv: $(VRCOMPOSITINGSERVER_SHADERDIR)/%.frag
	@mkdir -p $(SPIRVFRAGMENTDIR)/$(*D)
ifdef SHOWCOMMAND
	$(GLSLC) -o $@ $(GLSLCFLAGS) $<
else
	@echo Compiling $<...
	@$(GLSLC) -o $@ $(GLSLCFLAGS) $<
endif

$(VRCOMPOSITINGSERVER_SHADERS:%=$(SPIRVVERTEXDIR)/%.spv): | $(DEPDIR)/config
$(VRCOMPOSITINGSERVER_SHADERS:%=$(SPIRVFRAGMENTDIR)/%.spv): | $(DEPDIR)/config

#
# The VRCompositingServer helper script:
#

$(EXEDIR)/RunVRCompositor.sh: | $(DEPDIR)/config
$(EXEDIR)/RunVRCompositor.sh: $(VRUI_SCRIPTDIR)/RunVRCompositor.sh $(DEPDIR)/Configure-Vrui
	@echo Creating helper script to run VR compositing server...
	@cp $(VRUI_SCRIPTDIR)/RunVRCompositor.sh $(EXEDIR)/RunVRCompositor.sh
	@sed -i -e 's@VRUIBINDIR=.*@VRUIBINDIR=$(EXECUTABLEINSTALLDIR)@' $(EXEDIR)/RunVRCompositor.sh
	@sed -i -e 's@VRUIETCDIR=.*@VRUIETCDIR=$(ETCINSTALLDIR)@' $(EXEDIR)/RunVRCompositor.sh
	@chmod a+x $(EXEDIR)/RunVRCompositor.sh

#
# Vrui and calibration utilities:
#

UTILITIES_SOURCES = $(wildcard Vrui/Utilities/*.cpp) \
                    $(wildcard Calibration/*.cpp) \

$(UTILITIES_SOURCES:%.cpp=$(OBJDIR)/%.o): | $(DEPDIR)/config

#
# Utility to concatenate orthogonal transformations:
#

$(EXEDIR)/TransformCalculator: PACKAGES += MYGEOMETRY MYMISC
$(EXEDIR)/TransformCalculator: $(OBJDIR)/Vrui/Utilities/TransformCalculator.o
.PHONY: TransformCalculator
TransformCalculator: $(EXEDIR)/TransformCalculator

#
# The VR Device Daemon test programs:
#

DEVICETEST_SOURCES = Vrui/EnvironmentDefinition.cpp \
                     Vrui/Internal/VRDeviceState.cpp \
                     Vrui/Internal/VRDeviceDescriptor.cpp \
                     Vrui/Internal/HMDConfiguration.cpp \
                     Vrui/Internal/VRBaseStation.cpp \
                     Vrui/Internal/VRDeviceProtocol.cpp \
                     Vrui/Internal/VRDeviceClient.cpp \
                     Vrui/Utilities/DeviceTest.cpp

$(DEVICETEST_SOURCES:%.cpp=$(OBJDIR)/%.o): | $(DEPDIR)/config

$(EXEDIR)/DeviceTest: PACKAGES += MYGEOMETRY MYCOMM MYIO MYTHREADS MYREALTIME MYMISC
$(EXEDIR)/DeviceTest: $(DEVICETEST_SOURCES:%.cpp=$(OBJDIR)/%.o)
.PHONY: DeviceTest
DeviceTest: $(EXEDIR)/DeviceTest

$(EXEDIR)/TrackingTest: PACKAGES += MYVRUI MYGLMOTIF MYGLGEOMETRY MYGLSUPPORT MYGLWRAPPERS MYGEOMETRY MYMATH MYTHREADS MYREALTIME MYMISC GL
$(EXEDIR)/TrackingTest: $(OBJDIR)/Vrui/Utilities/TrackingTest.o
.PHONY: TrackingTest
TrackingTest: $(EXEDIR)/TrackingTest

#
# The HMD detector utility:
#

$(EXEDIR)/FindHMD: PACKAGES += MYUSB XRANDR X11
$(EXEDIR)/FindHMD: $(OBJDIR)/Vrui/Utilities/FindHMD.o
.PHONY: FindHMD
FindHMD: $(EXEDIR)/FindHMD

#
# The Vrui environment setup program:
#

$(EXEDIR)/RoomSetup: PACKAGES += MYVRUI MYGLMOTIF MYGLGEOMETRY MYGLSUPPORT MYGLWRAPPERS MYGEOMETRY MYMATH MYTHREADS MYMISC GL
$(EXEDIR)/RoomSetup: $(OBJDIR)/Vrui/Utilities/RoomSetup.o
.PHONY: RoomSetup
RoomSetup: $(EXEDIR)/RoomSetup

#
# The Vrui sound configuration and test programs:
#

$(EXEDIR)/VruiSoundConfig: PACKAGES += MYVRUI MYSOUND MYGLMOTIF MYIO MYTHREADS MYMISC
$(EXEDIR)/VruiSoundConfig: $(OBJDIR)/Vrui/Utilities/VruiSoundConfig.o
$(EXEDIR)/VruiSoundConfig: | $(call LIBRARYNAME,libVrui)
.PHONY: VruiSoundConfig
VruiSoundConfig: $(EXEDIR)/VruiSoundConfig

$(EXEDIR)/VruiSoundConfigTest: PACKAGES += MYVRUI MYALSUPPORT MYSOUND MYTHREADS MYMISC
$(EXEDIR)/VruiSoundConfigTest: $(OBJDIR)/Vrui/Utilities/VruiSoundTest.o
$(EXEDIR)/VruiSoundConfigTest: | $(call LIBRARYNAME,libVrui)
.PHONY: VruiSoundConfigTest
VruiSoundConfigTest: $(EXEDIR)/VruiSoundConfigTest

#
# The HMD application helper script:
#

$(EXEDIR)/OnHMD: | $(DEPDIR)/config
$(EXEDIR)/OnHMD: $(VRUI_SCRIPTDIR)/OnHMD $(DEPDIR)/Configure-Vrui
	@echo Creating helper script to run Vrui applications on OpenVR-supported head mounted displays...
	@cp $(VRUI_SCRIPTDIR)/OnHMD $(EXEDIR)/OnHMD
	@sed -i -e 's@VRUIBINDIR=.*@VRUIBINDIR=$(EXECUTABLEINSTALLDIR)@' $(EXEDIR)/OnHMD
	@sed -i -e 's@VRUIETCDIR=.*@VRUIETCDIR=$(ETCINSTALLDIR)@' $(EXEDIR)/OnHMD
	@sed -i -e 's@USERETCDIR=.*@USERETCDIR=$$HOME/$(VRUI_USERCONFIGDIR)@' $(EXEDIR)/OnHMD
	@chmod a+x $(EXEDIR)/OnHMD

#
# The Vrui eye calibration program:
#

$(EXEDIR)/EyeCalibrator: PACKAGES += MYVRUI MYGLGEOMETRY MYGEOMETRY MYMATH MYMISC GL
$(EXEDIR)/EyeCalibrator: $(OBJDIR)/Vrui/Utilities/EyeCalibrator.o
.PHONY: EyeCalibrator
EyeCalibrator: $(EXEDIR)/EyeCalibrator

#
# The Vrui input device data file printer:
#

PRINTINPUTDEVICEDATAFILE_SOURCES = Vrui/InputDevice.cpp \
                                   Vrui/Utilities/PrintInputDeviceDataFile.cpp

$(PRINTINPUTDEVICEDATAFILE_SOURCES:%.cpp=$(OBJDIR)/%.o): | $(DEPDIR)/config

$(EXEDIR)/PrintInputDeviceDataFile: PACKAGES += MYGEOMETRY MYIO MYMISC
$(EXEDIR)/PrintInputDeviceDataFile: $(PRINTINPUTDEVICEDATAFILE_SOURCES:%.cpp=$(OBJDIR)/%.o)
.PHONY: PrintInputDeviceDataFile
PrintInputDeviceDataFile: $(EXEDIR)/PrintInputDeviceDataFile

#
# The calibration pattern generator:
#

$(EXEDIR)/XBackground: PACKAGES += X11
$(EXEDIR)/XBackground: EXTRACINCLUDEFLAGS += -ICalibration
$(EXEDIR)/XBackground: $(OBJDIR)/Calibration/XBackground.o
.PHONY: XBackground
XBackground: $(EXEDIR)/XBackground

#
# The measurement utility:
#

MEASUREENVIRONMENT_SOURCES = Calibration/TotalStation.cpp \
                             Calibration/NaturalPointClient.cpp \
                             Calibration/MeasureEnvironment.cpp

$(EXEDIR)/MeasureEnvironment: PACKAGES += MYVRUI MYGLMOTIF MYGLGEOMETRY MYGLWRAPPERS MYGEOMETRY MYMATH MYCOMM MYIO MYTHREADS MYMISC GL
$(EXEDIR)/MeasureEnvironment: EXTRACINCLUDEFLAGS += -ICalibration
$(EXEDIR)/MeasureEnvironment: $(MEASUREENVIRONMENT_SOURCES:%.cpp=$(OBJDIR)/%.o)
.PHONY: MeasureEnvironment
MeasureEnvironment: $(EXEDIR)/MeasureEnvironment

#
# The calibration transformation calculator:
#

$(EXEDIR)/ScreenCalibrator: PACKAGES += MYVRUI MYGLGEOMETRY MYGEOMETRY MYMATH MYIO MYMISC GL
$(EXEDIR)/ScreenCalibrator: EXTRACINCLUDEFLAGS += -ICalibration
$(EXEDIR)/ScreenCalibrator: $(OBJDIR)/Calibration/OGTransformCalculator.o \
                            $(OBJDIR)/Calibration/ScreenCalibrator.o
.PHONY: ScreenCalibrator
ScreenCalibrator: $(EXEDIR)/ScreenCalibrator

#
# The rigid body transformation calculator:
#

ALIGNTRACKINGMARKERS_SOURCES = Calibration/ReadOptiTrackMarkerFile.cpp \
                               Calibration/NaturalPointClient.cpp \
                               Calibration/AlignTrackingMarkers.cpp

$(EXEDIR)/AlignTrackingMarkers: PACKAGES += MYVRUI MYGLMOTIF MYGLGEOMETRY MYGLSUPPORT MYGLWRAPPERS MYGEOMETRY MYMATH MYCOMM MYTHREADS MYMISC GL
$(EXEDIR)/AlignTrackingMarkers: EXTRACINCLUDEFLAGS += -ICalibration
$(EXEDIR)/AlignTrackingMarkers: $(ALIGNTRACKINGMARKERS_SOURCES:%.cpp=$(OBJDIR)/%.o)
.PHONY: AlignTrackingMarkers
AlignTrackingMarkers: $(EXEDIR)/AlignTrackingMarkers

#
# Utility to transform points based on a scene graph PointTransformNode:
#

$(EXEDIR)/TransformPoints: PACKAGES += MYSCENEGRAPH MYGEOMETRY
$(EXEDIR)/TransformPoints: $(OBJDIR)/Vrui/Utilities/TransformPoints.o
.PHONY: TransformPoints
TransformPoints: $(EXEDIR)/TransformPoints

#
# A utility to align point sets using several transformation types:
#

$(EXEDIR)/AlignPoints: PACKAGES += MYVRUI MYGLGEOMETRY MYGLSUPPORT MYGEOMETRY MYMATH MYIO MYMISC GL
$(EXEDIR)/AlignPoints: $(OBJDIR)/Vrui/Utilities/AlignPoints.o
.PHONY: AlignPoints
AlignPoints: $(EXEDIR)/AlignPoints

#
# A utility to measure 3D positions using tracked input devices:
#

$(EXEDIR)/MeasurePoints: PACKAGES += MYVRUI MYGLMOTIF MYGLGEOMETRY MYGLSUPPORT MYGLWRAPPERS MYGEOMETRY MYMATH MYIO MYTHREADS MYMISC GL
$(EXEDIR)/MeasurePoints: $(OBJDIR)/Vrui/Utilities/MeasurePoints.o
.PHONY: MeasurePoints
MeasurePoints: $(EXEDIR)/MeasurePoints

#
# The tracker field sampling utility:
#

$(EXEDIR)/SampleTrackerField: PACKAGES += MYVRUI MYGLMOTIF MYGLGEOMETRY MYGLSUPPORT MYGEOMETRY MYMATH MYIO MYMISC GL
$(EXEDIR)/SampleTrackerField: EXTRACINCLUDEFLAGS += -ICalibration
$(EXEDIR)/SampleTrackerField: $(OBJDIR)/Calibration/SampleTrackerField.o
.PHONY: SampleTrackerField
SampleTrackerField: $(EXEDIR)/SampleTrackerField

#
# A utility to calibrate touchscreen and pen devices:
#

$(EXEDIR)/CalibrateTouchscreen: PACKAGES += MYVRUI MYGLMOTIF MYGLGEOMETRY MYGLSUPPORT MYGEOMETRY MYMATH MYRAWHID MYIO MYMISC GL
$(EXEDIR)/CalibrateTouchscreen: $(OBJDIR)/Calibration/CalibrateTouchscreen.o
.PHONY: CalibrateTouchscreen
CalibrateTouchscreen: $(EXEDIR)/CalibrateTouchscreen

#
# The Oculus Rift tracker calibrator:
#

$(EXEDIR)/OculusCalibrator: PACKAGES += MYVRUI MYGLGEOMETRY MYGLSUPPORT MYGLWRAPPERS MYGEOMETRY MYMATH MYUSB MYIO MYTHREADS MYMISC LIBUSB1 GL
$(EXEDIR)/OculusCalibrator: EXTRACINCLUDEFLAGS += -ICalibration
$(EXEDIR)/OculusCalibrator: $(OBJDIR)/Calibration/OculusCalibrator.o
.PHONY: OculusCalibrator
OculusCalibrator: $(EXEDIR)/OculusCalibrator

########################################################################
# Specify installation rules for header files, libraries, executables,
# configuration files, and shared files.
########################################################################

$(DEPDIR)/Configure-Makefiles: $(MAKEFILEFRAGMENT) \
                               $(MAKECONFIGFILE) \
                               $(PKGCONFIGFILE) \
                               $(TEMPLATEMAKEFILE) \
                               ExamplePrograms/makefile \
                               ExamplePrograms/MeshEditor/makefile \
                               ExamplePrograms/VRMLViewer/makefile \
                               $(DEPDIR)/Configure-Install
	@touch $(DEPDIR)/Configure-Makefiles

SYSTEMPACKAGES = $(patsubst MY%,,$(call _RECPS,$(PACKAGES)))
VRUIAPP_INCLUDEDIRS = -I$$(VRUI_INCLUDEDIR)
VRUIAPP_INCLUDEDIRS += $(sort $(foreach PACKAGENAME,$(SYSTEMPACKAGES),$($(PACKAGENAME)_INCLUDE)))
VRUIAPP_CFLAGS = $(CSYSFLAGS)
VRUIAPP_CFLAGS += $(sort $(foreach PACKAGENAME,$(PACKAGES_RECEXPAND),$($(PACKAGENAME)_CFLAGS)))
VRUIAPP_LDIRS = -L$$(VRUI_LIBDIR)
VRUIAPP_LDIRS += $(sort $(foreach PACKAGENAME,$(SYSTEMPACKAGES),$($(PACKAGENAME)_LIBDIR)))
VRUIAPP_LIBS = $(patsubst lib%,-l%$(LDEXT),$(LIBRARY_NAMES))
VRUIAPP_LIBS += $(foreach PACKAGENAME,$(SYSTEMPACKAGES),$($(PACKAGENAME)_LIBS))
VRUIAPP_LFLAGS =
ifneq ($(SYSTEM_HAVE_RPATH),0)
  ifneq ($(USE_RPATH),0)
    VRUIAPP_LFLAGS += -Wl,-rpath=$$(VRUI_LIBDIR)
  endif
endif

# Pseudo-target to dump Vrui compiler and linker flags to a makefile fragment
$(MAKEFILEFRAGMENT): PACKAGES = MYVRUI
$(MAKEFILEFRAGMENT): $(DEPDIR)/Configure-Install
	@echo Creating application makefile fragment...
	@echo '# Makefile fragment for Vrui applications' > $(MAKEFILEFRAGMENT)
	@echo '# Autogenerated by Vrui installation on $(shell date)' >> $(MAKEFILEFRAGMENT)
	@echo 'VRUI_VERSION = $(PROJECT_NUMERICVERSION)' >> $(MAKEFILEFRAGMENT)
	@echo 'VRUI_INCLUDEDIR = $(HEADERINSTALLDIR)' >> $(MAKEFILEFRAGMENT)
	@echo 'VRUI_CFLAGS = $(VRUIAPP_INCLUDEDIRS) $(VRUIAPP_CFLAGS)' >> $(MAKEFILEFRAGMENT)
	@echo 'VRUI_LIBDIR = $(LIBINSTALLDIR)' >> $(MAKEFILEFRAGMENT)
	@echo 'VRUI_LINKFLAGS = $(VRUIAPP_LDIRS) $(VRUIAPP_LIBS) $(VRUIAPP_LFLAGS)' >> $(MAKEFILEFRAGMENT)
	@echo 'VRUI_PLUGINFILEEXT = $(PLUGINFILEEXT)' >> $(MAKEFILEFRAGMENT)
	@echo 'VRUI_PLUGINCFLAGS = $(CPLUGINFLAGS)' >> $(MAKEFILEFRAGMENT)
	@echo 'VRUI_PLUGINLINKFLAGS = $(PLUGINLINKFLAGS)' >> $(MAKEFILEFRAGMENT)
	@echo 'VRUI_PLUGINHOSTLINKFLAGS = $(PLUGINHOSTLINKFLAGS)' >> $(MAKEFILEFRAGMENT)
	@echo 'VRUI_TOOLSDIR = $(PLUGININSTALLDIR)/$(VRTOOLSDIREXT)' >> $(MAKEFILEFRAGMENT)
	@echo 'VRUI_VISLETSDIR = $(PLUGININSTALLDIR)/$(VRVISLETSDIREXT)' >> $(MAKEFILEFRAGMENT)
ifeq ($(SYSTEM),DARWIN)
	@echo 'export MACOSX_DEPLOYMENT_TARGET = $(SYSTEM_DARWIN_VERSION)' >> $(MAKEFILEFRAGMENT)
endif

# Pseudo-target to dump Vrui configuration settings
$(MAKECONFIGFILE): $(DEPDIR)/Configure-Install
	@echo Creating configuration makefile fragment...
	@echo '# Makefile fragment for Vrui configuration options' > $(MAKECONFIGFILE)
	@echo '# Autogenerated by Vrui installation on $(shell date)' >> $(MAKECONFIGFILE)
	@echo >> $(MAKECONFIGFILE)
	@echo '# Configuration settings:'>> $(MAKECONFIGFILE)
	@echo 'SYSTEM_HAVE_LIBUDEV = $(SYSTEM_HAVE_LIBUDEV)' >> $(MAKECONFIGFILE)
	@echo 'SYSTEM_HAVE_LIBDBUS = $(SYSTEM_HAVE_LIBDBUS)' >> $(MAKECONFIGFILE)
	@echo 'SYSTEM_HAVE_LIBUSB1 = $(SYSTEM_HAVE_LIBUSB1)' >> $(MAKECONFIGFILE)
	@echo 'SYSTEM_HAVE_OPENSSL = $(SYSTEM_HAVE_OPENSSL)' >> $(MAKECONFIGFILE)
	@echo 'SYSTEM_HAVE_LIBPNG = $(SYSTEM_HAVE_LIBPNG)' >> $(MAKECONFIGFILE)
	@echo 'SYSTEM_HAVE_LIBJPEG = $(SYSTEM_HAVE_LIBJPEG)' >> $(MAKECONFIGFILE)
	@echo 'SYSTEM_HAVE_LIBTIFF = $(SYSTEM_HAVE_LIBTIFF)' >> $(MAKECONFIGFILE)
	@echo 'SYSTEM_HAVE_ALSA = $(SYSTEM_HAVE_ALSA)' >> $(MAKECONFIGFILE)
	@echo 'SYSTEM_HAVE_PULSEAUDIO = $(SYSTEM_HAVE_PULSEAUDIO)' >> $(MAKECONFIGFILE)
	@echo 'SYSTEM_HAVE_SPEEX = $(SYSTEM_HAVE_SPEEX)' >> $(MAKECONFIGFILE)
	@echo 'SYSTEM_HAVE_OPENAL = $(SYSTEM_HAVE_OPENAL)' >> $(MAKECONFIGFILE)
	@echo 'SYSTEM_HAVE_V4L2 = $(SYSTEM_HAVE_V4L2)' >> $(MAKECONFIGFILE)
	@echo 'SYSTEM_HAVE_DC1394 = $(SYSTEM_HAVE_DC1394)' >> $(MAKECONFIGFILE)
	@echo 'SYSTEM_HAVE_THEORA = $(SYSTEM_HAVE_THEORA)' >> $(MAKECONFIGFILE)
	@echo 'SYSTEM_HAVE_BLUETOOTH = $(SYSTEM_HAVE_BLUETOOTH)' >> $(MAKECONFIGFILE)
	@echo 'SYSTEM_HAVE_FREETYPE = $(SYSTEM_HAVE_FREETYPE)' >> $(MAKECONFIGFILE)
	@echo 'SYSTEM_HAVE_XRANDR = $(SYSTEM_HAVE_XRANDR)' >> $(MAKECONFIGFILE)
	@echo 'SYSTEM_HAVE_XINPUT2 = $(SYSTEM_HAVE_XINPUT2)' >> $(MAKECONFIGFILE)
	@echo 'SYSTEM_HAVE_VULKAN = $(SYSTEM_HAVE_VULKAN)' >> $(MAKECONFIGFILE)
	@echo 'USE_RPATH = $(USE_RPATH)' >> $(MAKECONFIGFILE)
	@echo 'GLSUPPORT_USE_TLS = $(GLSUPPORT_USE_TLS)' >> $(MAKECONFIGFILE)
	@echo 'LINUX_INPUT_H_HAS_STRUCTS = $(LINUX_INPUT_H_HAS_STRUCTS)' >> $(MAKECONFIGFILE)
	@echo 'VRUI_VRWINDOW_USE_SWAPGROUPS = $(VRUI_VRWINDOW_USE_SWAPGROUPS)' >> $(MAKECONFIGFILE)
	@echo 'VRDEVICES_USE_INPUT_ABSTRACTION = $(VRDEVICES_USE_INPUT_ABSTRACTION)' >> $(MAKECONFIGFILE)
	@echo 'VRDEVICES_USE_BLUETOOTH = $(VRDEVICES_USE_BLUETOOTH)' >> $(MAKECONFIGFILE)
	@echo >> $(MAKECONFIGFILE)
	@echo '# Version information:'>> $(MAKECONFIGFILE)
	@echo 'VRUI_VERSION = $(PROJECT_NUMERICVERSION)' >> $(MAKECONFIGFILE)
	@echo 'VRUI_NAME = $(PROJECT_FULLNAME)' >> $(MAKECONFIGFILE)
	@echo >> $(MAKECONFIGFILE)
	@echo '# Search directories:'>> $(MAKECONFIGFILE)
	@echo 'VRUI_PACKAGEROOT := $(INSTALLROOT)' >> $(MAKECONFIGFILE)
	@echo 'VRUI_INCLUDEDIR := $(HEADERINSTALLDIR)' >> $(MAKECONFIGFILE)
	@echo 'VRUI_LIBDIR_DEBUG := $(LIBINSTALLDIR_DEBUG)' >> $(MAKECONFIGFILE)
	@echo 'VRUI_LIBDIR_RELEASE := $(LIBINSTALLDIR_RELEASE)' >> $(MAKECONFIGFILE)
	@echo 'VRUI_RPATH_DEBUG := $(LIBINSTALLDIR_DEBUG)' >> $(MAKECONFIGFILE)
	@echo 'VRUI_RPATH_RELEASE := $(LIBINSTALLDIR_RELEASE)' >> $(MAKECONFIGFILE)
	@echo 'ifdef DEBUG' >> $(MAKECONFIGFILE)
	@echo '  VRUI_LIBDIR := $$(VRUI_LIBDIR_DEBUG)' >> $(MAKECONFIGFILE)
	@echo '  VRUI_RPATH := $$(VRUI_RPATH_DEBUG)' >> $(MAKECONFIGFILE)
	@echo 'else' >> $(MAKECONFIGFILE)
	@echo '  VRUI_LIBDIR := $$(VRUI_LIBDIR_RELEASE)' >> $(MAKECONFIGFILE)
	@echo '  VRUI_RPATH := $$(VRUI_RPATH_RELEASE)' >> $(MAKECONFIGFILE)
	@echo 'endif' >> $(MAKECONFIGFILE)
	@echo >> $(MAKECONFIGFILE)
	@echo '# Installation directories:'>> $(MAKECONFIGFILE)
	@echo 'VRUI_HEADERINSTALLDIR = $(HEADERINSTALLDIR)' >> $(MAKECONFIGFILE)
	@echo 'VRUI_LIBINSTALLDIR_DEBUG = $(LIBINSTALLDIR_DEBUG)' >> $(MAKECONFIGFILE)
	@echo 'VRUI_LIBINSTALLDIR_RELEASE = $(LIBINSTALLDIR_RELEASE)' >> $(MAKECONFIGFILE)
	@echo 'VRUI_EXECUTABLEINSTALLDIR_DEBUG = $(EXECUTABLEINSTALLDIR_DEBUG)' >> $(MAKECONFIGFILE)
	@echo 'VRUI_EXECUTABLEINSTALLDIR_RELEASE = $(EXECUTABLEINSTALLDIR_RELEASE)' >> $(MAKECONFIGFILE)
	@echo 'VRUI_PLUGININSTALLDIR_DEBUG = $(PLUGININSTALLDIR_DEBUG)' >> $(MAKECONFIGFILE)
	@echo 'VRUI_PLUGININSTALLDIR_RELEASE = $(PLUGININSTALLDIR_RELEASE)' >> $(MAKECONFIGFILE)
	@echo 'ifdef DEBUG' >> $(MAKECONFIGFILE)
	@echo '  VRUI_LIBINSTALLDIR = $$(VRUI_LIBINSTALLDIR_DEBUG)' >> $(MAKECONFIGFILE)
	@echo '  VRUI_EXECUTABLEINSTALLDIR = $$(VRUI_EXECUTABLEINSTALLDIR_DEBUG)' >> $(MAKECONFIGFILE)
	@echo '  VRUI_PLUGININSTALLDIR = $$(VRUI_PLUGININSTALLDIR_DEBUG)' >> $(MAKECONFIGFILE)
	@echo 'else' >> $(MAKECONFIGFILE)
	@echo '  VRUI_LIBINSTALLDIR = $$(VRUI_LIBINSTALLDIR_RELEASE)' >> $(MAKECONFIGFILE)
	@echo '  VRUI_EXECUTABLEINSTALLDIR = $$(VRUI_EXECUTABLEINSTALLDIR_RELEASE)' >> $(MAKECONFIGFILE)
	@echo '  VRUI_PLUGININSTALLDIR = $$(VRUI_PLUGININSTALLDIR_RELEASE)' >> $(MAKECONFIGFILE)
	@echo 'endif' >> $(MAKECONFIGFILE)
	@echo 'VRTOOLSDIREXT = $(VRTOOLSDIREXT)' >> $(MAKECONFIGFILE)
	@echo 'VRVISLETSDIREXT = $(VRVISLETSDIREXT)' >> $(MAKECONFIGFILE)
	@echo 'VRUI_ETCINSTALLDIR = $(ETCINSTALLDIR)' >> $(MAKECONFIGFILE)
	@echo 'VRUI_SHAREINSTALLDIR = $(SHAREINSTALLDIR)' >> $(MAKECONFIGFILE)
	@echo 'VRUI_MAKEINSTALLDIR = $(MAKEINSTALLDIR)' >> $(MAKECONFIGFILE)
	@echo 'VRUI_PKGCONFIGINSTALLDIR = $(PKGCONFIGINSTALLDIR)' >> $(MAKECONFIGFILE)
	@echo 'VRUI_DOCINSTALLDIR = $(DOCINSTALLDIR)' >> $(MAKECONFIGFILE)

ROGUE_SYSTEMPACKAGES = $(filter %_,$(foreach PACKAGENAME,$(SYSTEMPACKAGES),$(PACKAGENAME)_$($(PACKAGENAME)_PKGNAME)))
ROGUE_INCLUDEDIRS = $(sort $(foreach PACKAGENAME,$(ROGUE_SYSTEMPACKAGES),$($(PACKAGENAME)INCLUDE)))
ROGUE_LIBDIRS = $(sort $(foreach PACKAGENAME,$(ROGUE_SYSTEMPACKAGES),$($(PACKAGENAME)LIBDIR)))
ROGUE_LIBS = $(foreach PACKAGENAME,$(ROGUE_SYSTEMPACKAGES),$($(PACKAGENAME)LIBS))

# Pseudo-target to create a metadata file for pkg-config
$(PKGCONFIGFILE): PACKAGES = MYVRUI
$(PKGCONFIGFILE): $(DEPDIR)/Configure-Install
	@echo Creating pkg-config meta data file...
	@echo 'prefix=$(INSTALLDIR)' > $(PKGCONFIGFILE)
	@echo 'exec_prefix=$${prefix}' >> $(PKGCONFIGFILE)
	@echo 'libdir=$(LIBINSTALLDIR)' >> $(PKGCONFIGFILE)
	@echo 'includedir=$(HEADERINSTALLDIR)' >> $(PKGCONFIGFILE)
	@echo '' >> $(PKGCONFIGFILE)
	@echo 'Name: Vrui' >> $(PKGCONFIGFILE)
	@echo 'Description: Vrui (Virtual Reality User Interface) development toolkit' >> $(PKGCONFIGFILE)
	@echo 'Requires: $(strip $(foreach PACKAGENAME,$(SYSTEMPACKAGES),$($(PACKAGENAME)_PKGNAME)))' >> $(PKGCONFIGFILE)
	@echo 'Version: $(PROJECT_NUMERICVERSION)' >> $(PKGCONFIGFILE)
ifneq ($(SYSTEM_HAVE_RPATH),0)
  ifneq ($(USE_RPATH),0)
	@echo 'Libs: -L$${libdir} $(strip $(patsubst lib%,-l%$(LDEXT),$(LIBRARY_NAMES))) $(ROGUE_LIBDIRS) $(ROGUE_LIBS) -Wl,-rpath=$${libdir}' >> $(PKGCONFIGFILE)
  else
	@echo 'Libs: -L$${libdir} $(strip $(patsubst lib%,-l%$(LDEXT),$(LIBRARY_NAMES))) $(ROGUE_LIBDIRS) $(ROGUE_LIBS)' >> $(PKGCONFIGFILE)
  endif
else
	@echo 'Libs: -L$${libdir} $(strip $(patsubst lib%,-l%$(LDEXT),$(LIBRARY_NAMES))) $(ROGUE_LIBDIRS) $(ROGUE_LIBS)' >> $(PKGCONFIGFILE)
endif	
	@echo 'Cflags: -I$${includedir} $(ROGUE_INCLUDEDIRS) $(strip $(VRUIAPP_CFLAGS))' >> $(PKGCONFIGFILE)

# Pseudo-target to configure the template makefile
$(TEMPLATEMAKEFILE): $(VRUI_MAKEDIR)/makefile.template $(DEPDIR)/Configure-Install
	@echo Configuring template makefile...
	@cp $(VRUI_MAKEDIR)/makefile.template $(TEMPLATEMAKEFILE)
	@sed -i -e 's@^VRUI_MAKEDIR = .*@VRUI_MAKEDIR = $(MAKEINSTALLDIR)@' $(TEMPLATEMAKEFILE)

# Pseudo-target to configure the ExamplePrograms makefile
ExamplePrograms/makefile: ExamplePrograms/makefile.template $(DEPDIR)/Configure-Install
	@echo Configuring makefile in ExamplePrograms...
	@cp ExamplePrograms/makefile.template ExamplePrograms/makefile
	@sed -i -e 's@^VRUI_MAKEDIR = .*@VRUI_MAKEDIR = $(MAKEINSTALLDIR)@' ExamplePrograms/makefile

# Pseudo-target to configure the ExamplePrograms/MeshEditor makefile
ExamplePrograms/MeshEditor/makefile: ExamplePrograms/MeshEditor/makefile.template $(DEPDIR)/Configure-Install
	@echo Configuring makefile in ExamplePrograms/MeshEditor...
	@cp ExamplePrograms/MeshEditor/makefile.template ExamplePrograms/MeshEditor/makefile
	@sed -i -e 's@^VRUISHAREDIR = .*@VRUISHAREDIR = $(SHAREINSTALLDIR)@' ExamplePrograms/MeshEditor/makefile

# Pseudo-target to configure the ExamplePrograms/VRMLViewer makefile
ExamplePrograms/VRMLViewer/makefile: ExamplePrograms/VRMLViewer/makefile.template $(DEPDIR)/Configure-Install
	@echo Configuring makefile in ExamplePrograms/VRMLViewer...
	@cp ExamplePrograms/VRMLViewer/makefile.template ExamplePrograms/VRMLViewer/makefile
	@sed -i -e 's@^VRUI_MAKEDIR = .*@VRUI_MAKEDIR = $(MAKEINSTALLDIR)@' ExamplePrograms/VRMLViewer/makefile

BUILDROOT_FILES = $(VRUI_MAKEDIR)/SystemDefinitions \
                  $(VRUI_MAKEDIR)/Packages.System \
                  $(VRUI_MAKEDIR)/Packages.Vrui \
                  $(VRUI_MAKEDIR)/FindInHeader.sh \
                  $(VRUI_MAKEDIR)/FindLibrary.sh \
                  $(VRUI_MAKEDIR)/StripPackages \
                  $(VRUI_MAKEDIR)/BasicMakefile \
                  $(VRUI_MAKEDIR)/BackupIfNEqual.sh \
                  $(VRUI_MAKEDIR)/CleanDir.sh \
                  $(VRUI_MAKEDIR)/CleanDirIfEqual.sh \
                  $(VRUI_MAKEDIR)/makefile

install:
# Install all header files in HEADERINSTALLDIR:
	@echo Installing header files in $(HEADERINSTALLDIR)...
	@install -d $(HEADERINSTALLDIR)/Misc
	@install -m u=rw,go=r $(MISC_HEADERS) $(HEADERINSTALLDIR)/Misc
	@install -d $(HEADERINSTALLDIR)/Realtime
	@install -m u=rw,go=r $(REALTIME_HEADERS) $(HEADERINSTALLDIR)/Realtime
	@install -d $(HEADERINSTALLDIR)/Threads
	@install -m u=rw,go=r $(THREADS_HEADERS) $(HEADERINSTALLDIR)/Threads
ifneq ($(SYSTEM_HAVE_LIBUSB1),0)
	@install -d $(HEADERINSTALLDIR)/USB
	@install -m u=rw,go=r $(USB_HEADERS) $(HEADERINSTALLDIR)/USB
endif
	@install -d $(HEADERINSTALLDIR)/RawHID
	@install -m u=rw,go=r $(RAWHID_HEADERS) $(HEADERINSTALLDIR)/RawHID
	@install -d $(HEADERINSTALLDIR)/IO
	@install -m u=rw,go=r $(IO_HEADERS) $(HEADERINSTALLDIR)/IO
	@install -d $(HEADERINSTALLDIR)/Plugins
	@install -m u=rw,go=r $(PLUGINS_HEADERS) $(HEADERINSTALLDIR)/Plugins
	@install -d $(HEADERINSTALLDIR)/Comm
	@install -m u=rw,go=r $(COMM_HEADERS) $(HEADERINSTALLDIR)/Comm
	@install -d $(HEADERINSTALLDIR)/Cluster
	@install -m u=rw,go=r $(CLUSTER_HEADERS) $(HEADERINSTALLDIR)/Cluster
	@install -d $(HEADERINSTALLDIR)/Math
	@install -m u=rw,go=r $(MATH_HEADERS) $(HEADERINSTALLDIR)/Math
	@install -d $(HEADERINSTALLDIR)/Geometry
	@install -m u=rw,go=r $(GEOMETRY_HEADERS) $(HEADERINSTALLDIR)/Geometry
ifeq ($(SYSTEM),DARWIN)
	@install -d $(HEADERINSTALLDIR)/MacOSX
	@install -m u=rw,go=r $(MACOSX_HEADERS) $(HEADERINSTALLDIR)/MacOSX
endif
	@install -d $(HEADERINSTALLDIR)/GL
	@install -m u=rw,go=r $(GLWRAPPERS_HEADERS) $(HEADERINSTALLDIR)/GL
	@install -m u=rw,go=r $(GLSUPPORT_HEADERS) $(HEADERINSTALLDIR)/GL
	@install -d $(HEADERINSTALLDIR)/GL/Extensions
	@install -m u=rw,go=r $(GLSUPPORTEXTENSION_HEADERS) $(HEADERINSTALLDIR)/GL/Extensions
	@install -m u=rw,go=r $(GLXSUPPORT_HEADERS) $(HEADERINSTALLDIR)/GL
	@install -m u=rw,go=r $(GLGEOMETRY_HEADERS) $(HEADERINSTALLDIR)/GL
ifneq ($(SYSTEM_HAVE_VULKAN),0)
	@install -d $(HEADERINSTALLDIR)/Vulkan
	@install -m u=rw,go=r $(VULKAN_HEADERS) $(HEADERINSTALLDIR)/Vulkan
	@install -d $(HEADERINSTALLDIR)/VulkanXlib
	@install -m u=rw,go=r $(VULKANXLIB_HEADERS) $(HEADERINSTALLDIR)/VulkanXlib
endif
	@install -d $(HEADERINSTALLDIR)/Images
	@install -m u=rw,go=r $(IMAGES_HEADERS) $(HEADERINSTALLDIR)/Images
	@install -d $(HEADERINSTALLDIR)/GLMotif
	@install -m u=rw,go=r $(GLMOTIF_HEADERS) $(HEADERINSTALLDIR)/GLMotif
	@install -d $(HEADERINSTALLDIR)/Sound
	@install -m u=rw,go=r $(SOUND_HEADERS) $(HEADERINSTALLDIR)/Sound
ifeq ($(SYSTEM),LINUX)
  ifneq ($(strip $(SOUND_LINUX_HEADERS)),)
	@install -d $(HEADERINSTALLDIR)/Sound/Linux
	@install -m u=rw,go=r $(SOUND_LINUX_HEADERS) $(HEADERINSTALLDIR)/Sound/Linux
  endif
endif
	@install -d $(HEADERINSTALLDIR)/Video
	@install -m u=rw,go=r $(VIDEO_HEADERS) $(HEADERINSTALLDIR)/Video
	@install -d $(HEADERINSTALLDIR)/AL
	@install -m u=rw,go=r $(ALSUPPORT_HEADERS) $(HEADERINSTALLDIR)/AL
	@install -d $(HEADERINSTALLDIR)/SceneGraph
	@install -m u=rw,go=r $(SCENEGRAPH_HEADERS) $(HEADERINSTALLDIR)/SceneGraph
	@install -d $(HEADERINSTALLDIR)/Vrui
	@install -m u=rw,go=r $(VRUI_HEADERS) $(HEADERINSTALLDIR)/Vrui
# Install all library files in LIBINSTALLDIR:
	@echo Installing libraries in $(LIBINSTALLDIR)...
	@install -d $(LIBINSTALLDIR)
	@install $(LIBRARIES) $(LIBINSTALLDIR)
	@echo Configuring run-time linker...
	$(foreach LIBNAME,$(LIBRARY_NAMES),$(call CREATE_SYMLINK,$(LIBNAME)))
# Install all binaries in EXECUTABLEINSTALLDIR:
	@echo Installing executables in $(EXECUTABLEINSTALLDIR)...
	@install -d $(EXECUTABLEINSTALLDIR)
	@install $(EXECUTABLES) $(EXECUTABLEINSTALLDIR)
ifeq ($(SYSTEM),DARWIN)
# Install a Mac OS X helper script:
	@cp $(VRUI_SCRIPTDIR)/MacOSX/runwithx $(VRUI_SCRIPTDIR)/MacOSX/runwithx.tmp
	@sed -i -e "s:_VRUIETCDIR_:$(ETCINSTALLDIR):" $(VRUI_SCRIPTDIR)/MacOSX/runwithx.tmp
	@sed -i -e "s:_VRUIBINDIR_:$(EXECUTABLEINSTALLDIR):" $(VRUI_SCRIPTDIR)/MacOSX/runwithx.tmp
	@install $(VRUI_SCRIPTDIR)/MacOSX/runwithx.tmp $(EXECUTABLEINSTALLDIR)/runwithx
	@rm $(VRUI_SCRIPTDIR)/MacOSX/runwithx.tmp
endif
# Install all plug-ins in PLUGININSTALLDIR:
	@echo Installing plug-ins in $(PLUGININSTALLDIR)...
	@install -d $(PLUGININSTALLDIR)/$(VRTOOLSDIREXT)
	@install $(VRTOOLS) $(PLUGININSTALLDIR)/$(VRTOOLSDIREXT)
	@install -d $(PLUGININSTALLDIR)/$(VRVISLETSDIREXT)
	@install $(VRVISLETS) $(PLUGININSTALLDIR)/$(VRVISLETSDIREXT)
	@install -d $(PLUGININSTALLDIR)/$(VRDEVICESDIREXT)
	@install $(VRDEVICES) $(PLUGININSTALLDIR)/$(VRDEVICESDIREXT)
	@install -d $(PLUGININSTALLDIR)/$(VRCALIBRATORSDIREXT)
	@install $(VRCALIBRATORS) $(PLUGININSTALLDIR)/$(VRCALIBRATORSDIREXT)
# Install all configuration files in ETCINSTALLDIR while keeping backups of existing files:
	@echo Installing configuration files in $(ETCINSTALLDIR)...
	@echo Existing configuration files in $(ETCINSTALLDIR) will be backed up
	@install -d $(ETCINSTALLDIR)
	@$(VRUI_MAKEDIR)/BackupIfNEqual.sh $(ETCINSTALLDIR) $(VRUI_ETCDIR)/*.cfg
	@install -m u=rw,go=r $(VRUI_ETCDIR)/*.cfg $(ETCINSTALLDIR)
ifeq ($(SYSTEM),DARWIN)
	@install -m u=rw,go=r $(VRUI_SCRIPTDIR)/MacOSX/Vrui.xinitrc $(ETCINSTALLDIR)
endif
# Install all shared files in SHAREINSTALLDIR:
	@echo Installing shared files in $(SHAREINSTALLDIR)...
	@install -d $(SHAREINSTALLDIR)/GLFonts
	@install -m u=rw,go=r $(VRUI_SHAREDIR)/GLFonts/* $(SHAREINSTALLDIR)/GLFonts
	@install -d $(SHAREINSTALLDIR)/Textures
	@install -m u=rw,go=r $(VRUI_SHAREDIR)/Textures/* $(SHAREINSTALLDIR)/Textures
	@install -d $(SHAREINSTALLDIR)/Shaders
	@install -d $(SHAREINSTALLDIR)/Shaders/GLSupport
	@install -m u=rw,go=r $(VRUI_SHAREDIR)/Shaders/GLSupport/* $(SHAREINSTALLDIR)/Shaders/GLSupport
	@install -d $(SHAREINSTALLDIR)/Shaders/SceneGraph
	@install -m u=rw,go=r $(VRUI_SHAREDIR)/Shaders/SceneGraph/* $(SHAREINSTALLDIR)/Shaders/SceneGraph
	@install -d $(SHAREINSTALLDIR)/Resources
	@install -m u=rw,go=r $(VRUI_SHAREDIR)/Resources/*.wrl $(SHAREINSTALLDIR)/Resources
	@install -d $(SHAREINSTALLDIR)/Resources/IKAvatar
	@install -m u=rw,go=r $(VRUI_SHAREDIR)/Resources/IKAvatar/* $(SHAREINSTALLDIR)/Resources/IKAvatar
ifneq ($(SYSTEM_HAVE_VULKAN),0)
	@install -d $(SHAREINSTALLDIR)/spirv
	@install -d $(SHAREINSTALLDIR)/spirv/vertex
	@install -m u=rw,go=r $(VRUI_SHAREDIR)/spirv/vertex/* $(SHAREINSTALLDIR)/spirv/vertex
	@install -d $(SHAREINSTALLDIR)/spirv/fragment
	@install -m u=rw,go=r $(VRUI_SHAREDIR)/spirv/fragment/* $(SHAREINSTALLDIR)/spirv/fragment
endif	
# Install makefile fragment in SHAREINSTALLDIR:
	@echo Installing application makefile fragment in $(SHAREINSTALLDIR)...
	@install -m u=rw,go=r $(MAKEFILEFRAGMENT) $(SHAREINSTALLDIR)
# Install full build system in MAKEINSTALLDIR:
	@echo Installing build system in $(MAKEINSTALLDIR)...
	@install -d $(MAKEINSTALLDIR)
	@install -m u=rw,go=r $(BUILDROOT_FILES) $(MAKEINSTALLDIR)
	@chmod a+x $(MAKEINSTALLDIR)/StripPackages $(MAKEINSTALLDIR)/*.sh
	@cp $(MAKECONFIGFILE) $(MAKEINSTALLDIR)/Configuration.Vrui
	@chmod u=rw,go=r $(MAKEINSTALLDIR)/Configuration.Vrui
# Install pkg-config metafile in PKGCONFIGINSTALLDIR:
	@echo Installing pkg-config metafile in $(PKGCONFIGINSTALLDIR)...
	@install -d $(PKGCONFIGINSTALLDIR)
	@install -m u=rw,go=r $(PKGCONFIGFILE) $(PKGCONFIGINSTALLDIR)
# Install documentation in DOCINSTALLDIR:
	@echo Installing documentation in $(DOCINSTALLDIR)...
	@install -d $(DOCINSTALLDIR)
	@install -m u=rw,go=r $(VRUI_DOCDIR)/* $(DOCINSTALLDIR)

installudevrules:
# Copy device udev rule into rules directory:
	@echo Installing udev rules into $(UDEVRULEDIR)...
	@install -m u=rw,go=r $(VRUI_SCRIPTDIR)/69-3d-inputdevices-acl.rules $(UDEVRULEDIR)/69-Vrui-devices.rules

uninstall:
# Remove documentation from DOCINSTALLDIR:
	@echo Removing documentation from $(DOCINSTALLDIR)...
	@$(VRUI_MAKEDIR)/CleanDir.sh $(DOCINSTALLDIR) $(wildcard $(VRUI_DOCDIR)/*)
# Remove pkg-config metafile from PKGCONFIGINSTALLDIR:
	@echo Removing pkg-config metafile from $(PKGCONFIGINSTALLDIR)...
	@$(VRUI_MAKEDIR)/CleanDir.sh $(PKGCONFIGINSTALLDIR) $(PKGCONFIGFILE)
# Remove full build system from MAKEINSTALLDIR:
	@echo Removing build system from $(MAKEINSTALLDIR)...
	@$(VRUI_MAKEDIR)/CleanDirIfEqual.sh $(MAKEINSTALLDIR) $(VRUI_MAKEDIR) $(wildcard $(VRUI_MAKEDIR)/*)
# Remove makefile fragment from SHAREINSTALLDIR:
	@echo Removing application makefile fragment from $(SHAREINSTALLDIR)...
	@$(VRUI_MAKEDIR)/CleanDir.sh $(SHAREINSTALLDIR) $(MAKEFILEFRAGMENT)
# Remove all shared files from SHAREINSTALLDIR:
	@echo Removing shared files from $(SHAREINSTALLDIR)...
	@$(VRUI_MAKEDIR)/CleanDirIfEqual.sh $(SHAREINSTALLDIR)/GLFonts $(VRUI_SHAREDIR)/GLFonts $(wildcard $(VRUI_SHAREDIR)/GLFonts/*)
	@$(VRUI_MAKEDIR)/CleanDirIfEqual.sh $(SHAREINSTALLDIR)/Textures $(VRUI_SHAREDIR)/Textures $(wildcard $(VRUI_SHAREDIR)/Textures/*)
	@$(VRUI_MAKEDIR)/CleanDirIfEqual.sh $(SHAREINSTALLDIR)/Shaders/SceneGraph $(VRUI_SHAREDIR)/Shaders/SceneGraph $(wildcard $(VRUI_SHAREDIR)/Shaders/SceneGraph/*)
	@$(VRUI_MAKEDIR)/CleanDirIfEqual.sh $(SHAREINSTALLDIR)/Resources $(VRUI_SHAREDIR)/Resources $(wildcard $(VRUI_SHAREDIR)/Resources/*)
	@$(VRUI_MAKEDIR)/CleanDir.sh $(SHAREINSTALLDIR)/Shaders
ifneq ($(SYSTEM_HAVE_VULKAN),0)
	@$(VRUI_MAKEDIR)/CleanDir.sh $(SHAREINSTALLDIR)/spirv/vertex
	@$(VRUI_MAKEDIR)/CleanDir.sh $(SHAREINSTALLDIR)/spirv/fragment
	@$(VRUI_MAKEDIR)/CleanDir.sh $(SHAREINSTALLDIR)/spirv
endif	
	@$(VRUI_MAKEDIR)/CleanDir.sh $(SHAREINSTALLDIR)
# Remove all configuration files from ETCINSTALLDIR:
	@echo Removing configuration files from $(ETCINSTALLDIR)...
	@$(VRUI_MAKEDIR)/CleanDirIfEqual.sh $(ETCINSTALLDIR) $(VRUI_ETCDIR) $(wildcard $(VRUI_ETCDIR)/*.cfg)
ifeq ($(SYSTEM),DARWIN)
	@$(VRUI_MAKEDIR)/CleanDirIfEqual.sh $(ETCINSTALLDIR) $(VRUI_SCRIPTDIR)/MacOSX Vrui.xinitrc
endif
# Remove all plug-ins from PLUGININSTALLDIR:
	@echo Removing plug-ins from $(PLUGININSTALLDIR)...
	@$(VRUI_MAKEDIR)/CleanDir.sh $(PLUGININSTALLDIR)/$(VRTOOLSDIREXT) $(VRTOOLS)
	@$(VRUI_MAKEDIR)/CleanDir.sh $(PLUGININSTALLDIR)/$(VRVISLETSDIREXT) $(VRVISLETS)
	@$(VRUI_MAKEDIR)/CleanDir.sh $(PLUGININSTALLDIR)/$(VRDEVICESDIREXT) $(VRDEVICES)
	@$(VRUI_MAKEDIR)/CleanDir.sh $(PLUGININSTALLDIR)/$(VRCALIBRATORSDIREXT) $(VRCALIBRATORS)
	@$(VRUI_MAKEDIR)/CleanDir.sh $(PLUGININSTALLDIR)
# Remove all binaries from EXECUTABLEINSTALLDIR:
	@echo Removing executables from $(EXECUTABLEINSTALLDIR)...
	@$(VRUI_MAKEDIR)/CleanDir.sh $(EXECUTABLEINSTALLDIR) $(EXECUTABLES)
ifeq ($(SYSTEM),DARWIN)
	@$(VRUI_MAKEDIR)/CleanDir.sh $(EXECUTABLEINSTALLDIR) runwithx
endif
ifneq ($(SYSTEM_HAVE_OPENVR),0)
	@$(VRUI_MAKEDIR)/CleanDir.sh $(EXECUTABLEINSTALLDIR) RunOpenVRTracker.sh
endif
	@$(VRUI_MAKEDIR)/CleanDir.sh $(EXECUTABLEINSTALLDIR) OnHMD
# Remove all library files from LIBINSTALLDIR:
	@echo Removing library files from $(LIBINSTALLDIR)...
	@$(VRUI_MAKEDIR)/CleanDir.sh $(LIBINSTALLDIR) $(foreach LIBNAME,$(LIBRARY_NAMES),$(call LINKDSONAME,$(LIBNAME)) $(call DSONAME,$(LIBNAME)) $(call FULLDSONAME,$(LIBNAME)))
# Remove all header files from HEADERINSTALLDIR:
	@echo Removing header files from $(HEADERINSTALLDIR)...
	@$(VRUI_MAKEDIR)/CleanDir.sh $(HEADERINSTALLDIR)/Misc $(MISC_HEADERS)
	@$(VRUI_MAKEDIR)/CleanDir.sh $(HEADERINSTALLDIR)/Realtime $(REALTIME_HEADERS)
	@$(VRUI_MAKEDIR)/CleanDir.sh $(HEADERINSTALLDIR)/Threads $(THREADS_HEADERS)
	@$(VRUI_MAKEDIR)/CleanDir.sh $(HEADERINSTALLDIR)/USB $(USB_HEADERS)
	@$(VRUI_MAKEDIR)/CleanDir.sh $(HEADERINSTALLDIR)/RawHID $(RAWHID_HEADERS)
	@$(VRUI_MAKEDIR)/CleanDir.sh $(HEADERINSTALLDIR)/IO $(IO_HEADERS)
	@$(VRUI_MAKEDIR)/CleanDir.sh $(HEADERINSTALLDIR)/Plugins $(PLUGINS_HEADERS)
	@$(VRUI_MAKEDIR)/CleanDir.sh $(HEADERINSTALLDIR)/Comm $(COMM_HEADERS)
	@$(VRUI_MAKEDIR)/CleanDir.sh $(HEADERINSTALLDIR)/Cluster $(CLUSTER_HEADERS)
	@$(VRUI_MAKEDIR)/CleanDir.sh $(HEADERINSTALLDIR)/Math $(MATH_HEADERS)
	@$(VRUI_MAKEDIR)/CleanDir.sh $(HEADERINSTALLDIR)/Geometry $(GEOMETRY_HEADERS)
	@$(VRUI_MAKEDIR)/CleanDir.sh $(HEADERINSTALLDIR)/GL/Extensions $(GLSUPPORTEXTENSION_HEADERS)
	@$(VRUI_MAKEDIR)/CleanDir.sh $(HEADERINSTALLDIR)/GL $(GLWRAPPERS_HEADERS) $(GLSUPPORT_HEADERS) $(GLXSUPPORT_HEADERS) $(GLGEOMETRY_HEADERS)
ifneq ($(SYSTEM_HAVE_VULKAN),0)
	@$(VRUI_MAKEDIR)/CleanDir.sh $(HEADERINSTALLDIR)/Vulkan $(VULKAN_HEADERS)
	@$(VRUI_MAKEDIR)/CleanDir.sh $(HEADERINSTALLDIR)/VulkanXlib $(VULKANXLIB_HEADERS)
endif
	@$(VRUI_MAKEDIR)/CleanDir.sh $(HEADERINSTALLDIR)/Images $(IMAGES_HEADERS)
	@$(VRUI_MAKEDIR)/CleanDir.sh $(HEADERINSTALLDIR)/GLMotif $(GLMOTIF_HEADERS)
	@$(VRUI_MAKEDIR)/CleanDir.sh $(HEADERINSTALLDIR)/Sound/Linux $(SOUND_LINUX_HEADERS)
	@$(VRUI_MAKEDIR)/CleanDir.sh $(HEADERINSTALLDIR)/Sound $(SOUND_HEADERS)
	@$(VRUI_MAKEDIR)/CleanDir.sh $(HEADERINSTALLDIR)/Video $(VIDEO_HEADERS)
	@$(VRUI_MAKEDIR)/CleanDir.sh $(HEADERINSTALLDIR)/AL $(ALSUPPORT_HEADERS)
	@$(VRUI_MAKEDIR)/CleanDir.sh $(HEADERINSTALLDIR)/SceneGraph $(SCENEGRAPH_HEADERS)
	@$(VRUI_MAKEDIR)/CleanDir.sh $(HEADERINSTALLDIR)/Vrui $(VRUI_HEADERS)
	@$(VRUI_MAKEDIR)/CleanDir.sh $(HEADERINSTALLDIR)
# Remove the installation directory:
	@$(VRUI_MAKEDIR)/CleanDir.sh $(INSTALLDIR)

uninstalludevrules:
# Remove device udev rule from rules directory:
	@echo Removing udev rules from $(UDEVRULEDIR)...
	@rm $(UDEVRULEDIR)/69-Vrui-devices.rules

#
# Target to install a "live" version of Vrui in the chosen installation
# directory that automatically reflects changes made to the Vrui sources.
#

devinstall:
# Install all header files in HEADERINSTALLDIR:
	@echo Installing header files in $(HEADERINSTALLDIR)...
	@mkdir -p $(HEADERINSTALLDIR)/Misc
	@ln -sf $(MISC_HEADERS:%=$(PROJECT_ROOT)/%) $(HEADERINSTALLDIR)/Misc
	@mkdir -p $(HEADERINSTALLDIR)/Realtime
	@ln -sf $(REALTIME_HEADERS:%=$(PROJECT_ROOT)/%) $(HEADERINSTALLDIR)/Realtime
	@mkdir -p $(HEADERINSTALLDIR)/Threads
	@ln -sf $(THREADS_HEADERS:%=$(PROJECT_ROOT)/%) $(HEADERINSTALLDIR)/Threads
ifneq ($(SYSTEM_HAVE_LIBUSB1),0)
	@mkdir -p $(HEADERINSTALLDIR)/USB
	@ln -sf $(USB_HEADERS:%=$(PROJECT_ROOT)/%) $(HEADERINSTALLDIR)/USB
endif
	@mkdir -p $(HEADERINSTALLDIR)/RawHID
	@ln -sf $(RAWHID_HEADERS:%=$(PROJECT_ROOT)/%) $(HEADERINSTALLDIR)/RawHID
	@mkdir -p $(HEADERINSTALLDIR)/IO
	@ln -sf $(IO_HEADERS:%=$(PROJECT_ROOT)/%) $(HEADERINSTALLDIR)/IO
	@mkdir -p $(HEADERINSTALLDIR)/Plugins
	@ln -sf $(PLUGINS_HEADERS:%=$(PROJECT_ROOT)/%) $(HEADERINSTALLDIR)/Plugins
	@mkdir -p $(HEADERINSTALLDIR)/Comm
	@ln -sf $(COMM_HEADERS:%=$(PROJECT_ROOT)/%) $(HEADERINSTALLDIR)/Comm
	@mkdir -p $(HEADERINSTALLDIR)/Cluster
	@ln -sf $(CLUSTER_HEADERS:%=$(PROJECT_ROOT)/%) $(HEADERINSTALLDIR)/Cluster
	@mkdir -p $(HEADERINSTALLDIR)/Math
	@ln -sf $(MATH_HEADERS:%=$(PROJECT_ROOT)/%) $(HEADERINSTALLDIR)/Math
	@mkdir -p $(HEADERINSTALLDIR)/Geometry
	@ln -sf $(GEOMETRY_HEADERS:%=$(PROJECT_ROOT)/%) $(HEADERINSTALLDIR)/Geometry
ifeq ($(SYSTEM),DARWIN)
	@mkdir -p $(HEADERINSTALLDIR)/MacOSX
	@ln -sf $(MACOSX_HEADERS:%=$(PROJECT_ROOT)/%) $(HEADERINSTALLDIR)/MacOSX
endif
	@mkdir -p $(HEADERINSTALLDIR)/GL
	@ln -sf $(GLWRAPPERS_HEADERS:%=$(PROJECT_ROOT)/%) $(HEADERINSTALLDIR)/GL
	@ln -sf $(GLSUPPORT_HEADERS:%=$(PROJECT_ROOT)/%) $(HEADERINSTALLDIR)/GL
	@mkdir -p $(HEADERINSTALLDIR)/GL/Extensions
	@ln -sf $(GLSUPPORTEXTENSION_HEADERS:%=$(PROJECT_ROOT)/%) $(HEADERINSTALLDIR)/GL/Extensions
	@ln -sf $(GLXSUPPORT_HEADERS:%=$(PROJECT_ROOT)/%) $(HEADERINSTALLDIR)/GL
	@ln -sf $(GLGEOMETRY_HEADERS:%=$(PROJECT_ROOT)/%) $(HEADERINSTALLDIR)/GL
ifneq ($(SYSTEM_HAVE_VULKAN),0)
	@mkdir -p $(HEADERINSTALLDIR)/Vulkan
	@ln -sf $(VULKAN_HEADERS:%=$(PROJECT_ROOT)/%) $(HEADERINSTALLDIR)/Vulkan
	@mkdir -p $(HEADERINSTALLDIR)/VulkanXlib
	@ln -sf $(VULKANXLIB_HEADERS:%=$(PROJECT_ROOT)/%) $(HEADERINSTALLDIR)/VulkanXlib
endif
	@mkdir -p $(HEADERINSTALLDIR)/Images
	@ln -sf $(IMAGES_HEADERS:%=$(PROJECT_ROOT)/%) $(HEADERINSTALLDIR)/Images
	@mkdir -p $(HEADERINSTALLDIR)/GLMotif
	@ln -sf $(GLMOTIF_HEADERS:%=$(PROJECT_ROOT)/%) $(HEADERINSTALLDIR)/GLMotif
	@mkdir -p $(HEADERINSTALLDIR)/Sound
	@ln -sf $(SOUND_HEADERS:%=$(PROJECT_ROOT)/%) $(HEADERINSTALLDIR)/Sound
ifeq ($(SYSTEM),LINUX)
  ifneq ($(strip $(SOUND_LINUX_HEADERS)),)
	@mkdir -p $(HEADERINSTALLDIR)/Sound/Linux
	@ln -sf $(SOUND_LINUX_HEADERS:%=$(PROJECT_ROOT)/%) $(HEADERINSTALLDIR)/Sound/Linux
  endif
endif
	@mkdir -p $(HEADERINSTALLDIR)/Video
	@ln -sf $(VIDEO_HEADERS:%=$(PROJECT_ROOT)/%) $(HEADERINSTALLDIR)/Video
	@mkdir -p $(HEADERINSTALLDIR)/AL
	@ln -sf $(ALSUPPORT_HEADERS:%=$(PROJECT_ROOT)/%) $(HEADERINSTALLDIR)/AL
	@mkdir -p $(HEADERINSTALLDIR)/SceneGraph
	@ln -sf $(SCENEGRAPH_HEADERS:%=$(PROJECT_ROOT)/%) $(HEADERINSTALLDIR)/SceneGraph
	@mkdir -p $(HEADERINSTALLDIR)/Vrui
	@ln -sf $(VRUI_HEADERS:%=$(PROJECT_ROOT)/%) $(HEADERINSTALLDIR)/Vrui
# Install all library files in LIBINSTALLDIR:
	@echo Installing libraries in $(LIBINSTALLDIR)...
	@mkdir -p $(LIBINSTALLDIR)
	@ln -sf $(LIBRARIES) $(LIBINSTALLDIR)
	@echo Configuring run-time linker...
	$(foreach LIBNAME,$(LIBRARY_NAMES),$(call CREATE_SYMLINK,$(LIBNAME)))
# Install all binaries in EXECUTABLEINSTALLDIR:
	@echo Installing executables in $(EXECUTABLEINSTALLDIR)...
	@mkdir -p $(EXECUTABLEINSTALLDIR)
	@ln -sf $(EXECUTABLES:%=$(PROJECT_ROOT)/%) $(EXECUTABLEINSTALLDIR)
ifeq ($(SYSTEM),DARWIN)
# Install a Mac OS X helper script:
	@cp $(VRUI_SCRIPTDIR)/MacOSX/runwithx $(VRUI_SCRIPTDIR)/MacOSX/runwithx.tmp
	@sed -i -e "s:_VRUIETCDIR_:$(ETCINSTALLDIR):" $(VRUI_SCRIPTDIR)/MacOSX/runwithx.tmp
	@sed -i -e "s:_VRUIBINDIR_:$(EXECUTABLEINSTALLDIR):" $(VRUI_SCRIPTDIR)/MacOSX/runwithx.tmp
	@cp $(VRUI_SCRIPTDIR)/MacOSX/runwithx.tmp $(EXECUTABLEINSTALLDIR)/runwithx
	@rm $(VRUI_SCRIPTDIR)/MacOSX/runwithx.tmp
endif
# Install all plug-ins in PLUGININSTALLDIR:
	@echo Installing plug-ins in $(PLUGININSTALLDIR)...
	@mkdir -p $(PLUGININSTALLDIR)/$(VRTOOLSDIREXT)
	@ln -sf $(VRTOOLS) $(PLUGININSTALLDIR)/$(VRTOOLSDIREXT)
	@mkdir -p $(PLUGININSTALLDIR)/$(VRVISLETSDIREXT)
	@ln -sf $(VRVISLETS) $(PLUGININSTALLDIR)/$(VRVISLETSDIREXT)
	@mkdir -p $(PLUGININSTALLDIR)/$(VRDEVICESDIREXT)
	@ln -sf $(VRDEVICES) $(PLUGININSTALLDIR)/$(VRDEVICESDIREXT)
	@mkdir -p $(PLUGININSTALLDIR)/$(VRCALIBRATORSDIREXT)
	@ln -sf $(VRCALIBRATORS) $(PLUGININSTALLDIR)/$(VRCALIBRATORSDIREXT)
# Install all configuration files in ETCINSTALLDIR while keeping backups of existing files:
	@echo Installing configuration files in $(ETCINSTALLDIR)...
	@echo Existing configuration files in $(ETCINSTALLDIR) will be backed up
	@mkdir -p $(ETCINSTALLDIR)
	@$(VRUI_MAKEDIR)/BackupIfNEqual.sh $(ETCINSTALLDIR) $(VRUI_ETCDIR)/*.cfg
	@ln -sf $(VRUI_ETCDIR)/*.cfg $(ETCINSTALLDIR)
ifeq ($(SYSTEM),DARWIN)
	@ln -sf $(VRUI_SCRIPTDIR)/MacOSX/Vrui.xinitrc $(ETCINSTALLDIR)
endif
# Install all shared files in SHAREINSTALLDIR:
	@echo Installing shared files in $(SHAREINSTALLDIR)...
	@mkdir -p $(SHAREINSTALLDIR)/GLFonts
	@ln -sf $(VRUI_SHAREDIR)/GLFonts/* $(SHAREINSTALLDIR)/GLFonts
	@mkdir -p $(SHAREINSTALLDIR)/Textures
	@ln -sf $(VRUI_SHAREDIR)/Textures/* $(SHAREINSTALLDIR)/Textures
	@mkdir -p $(SHAREINSTALLDIR)/Shaders
	@mkdir -p $(SHAREINSTALLDIR)/Shaders/GLSupport
	@ln -sf $(VRUI_SHAREDIR)/Shaders/GLSupport/* $(SHAREINSTALLDIR)/Shaders/GLSupport
	@mkdir -p $(SHAREINSTALLDIR)/Shaders/SceneGraph
	@ln -sf $(VRUI_SHAREDIR)/Shaders/SceneGraph/* $(SHAREINSTALLDIR)/Shaders/SceneGraph
	@mkdir -p $(SHAREINSTALLDIR)/Resources
	@ln -sf $(VRUI_SHAREDIR)/Resources/*.wrl $(SHAREINSTALLDIR)/Resources
	@mkdir -p $(SHAREINSTALLDIR)/Resources/IKAvatar
	@ln -sf $(VRUI_SHAREDIR)/Resources/IKAvatar/* $(SHAREINSTALLDIR)/Resources/IKAvatar
ifneq ($(SYSTEM_HAVE_VULKAN),0)
	@mkdir -p $(SHAREINSTALLDIR)/spirv
	@mkdir -p $(SHAREINSTALLDIR)/spirv/vertex
	@ln -sf $(VRUI_SHAREDIR)/spirv/vertex/* $(SHAREINSTALLDIR)/spirv/vertex
	@mkdir -p $(SHAREINSTALLDIR)/spirv/fragment
	@ln -sf $(VRUI_SHAREDIR)/spirv/fragment/* $(SHAREINSTALLDIR)/spirv/fragment
endif	
# Install makefile fragment in SHAREINSTALLDIR:
	@echo Installing application makefile fragment in $(SHAREINSTALLDIR)...
	@ln -sf $(MAKEFILEFRAGMENT) $(SHAREINSTALLDIR)
# Install full build system in MAKEINSTALLDIR:
	@echo Installing build system in $(MAKEINSTALLDIR)...
	@mkdir -p $(MAKEINSTALLDIR)
	@ln -sf $(BUILDROOT_FILES) $(MAKEINSTALLDIR)
	@chmod a+x $(MAKEINSTALLDIR)/StripPackages $(MAKEINSTALLDIR)/*.sh
	@ln -sf $(MAKECONFIGFILE) $(MAKEINSTALLDIR)/Configuration.Vrui
	@chmod u=rw,go=r $(MAKEINSTALLDIR)/Configuration.Vrui
# Install pkg-config metafile in PKGCONFIGINSTALLDIR:
	@echo Installing pkg-config metafile in $(PKGCONFIGINSTALLDIR)...
	@mkdir -p $(PKGCONFIGINSTALLDIR)
	@ln -sf $(PKGCONFIGFILE) $(PKGCONFIGINSTALLDIR)
# Install documentation in DOCINSTALLDIR:
	@echo Installing documentation in $(DOCINSTALLDIR)...
	@mkdir -p $(DOCINSTALLDIR)
	@ln -sf $(VRUI_DOCDIR)/* $(DOCINSTALLDIR)
