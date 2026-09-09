/***********************************************************************
ImageViewer - Small image viewer using Vrui.
Copyright (c) 2011-2026 Oliver Kreylos

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef IMAGEVIEWER_INCLUDED
#define IMAGEVIEWER_INCLUDED

#include <string>
#include <vector>
#include <IO/Directory.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <GL/gl.h>
#include <GL/GLColor.h>
#include <GL/GLObject.h>
#include <Images/Types.h>
#include <Images/BaseImage.h>
#include <GLMotif/TextFieldSlider.h>
#include <GLMotif/FileSelectionDialog.h>
#include <GLMotif/FileSelectionHelper.h>
#include <Vrui/Application.h>
#include <Vrui/Tool.h>
#include <Vrui/GenericToolFactory.h>
#include <Vrui/TransformTool.h>

/* Forward declarations: */
namespace Threads {
template <class ParameterParam>
class FunctionCall;
}
namespace GLMotif {
class PopupMenu;
class PopupWindow;
class Label;
class ToggleButton;
class TextField;
}

class ImageViewer:public Vrui::Application,public GLObject
	{
	/* Embedded classes: */
	public:
	typedef double Scalar;
	typedef Geometry::Point<Scalar,2> Point; // Type for points in image space
	typedef Geometry::Vector<Scalar,2> Vector; // Type for vectors in image space
	typedef GLColor<GLfloat,4> Color; // Type for RGBA image colors
	
	struct ImageSource // Structure describing the source of an image that can be loaded
		{
		/* Elements: */
		public:
		IO::DirectoryPtr directory; // Pointer to the directory containing the image source
		std::string fileName; // Name of the image source relative to the containing directory
		
		/* Constructors and destructors: */
		ImageSource(IO::Directory& sDirectory,const std::string& sFileName) // Elementwise constructor with directory reference
			:directory(&sDirectory),fileName(sFileName)
			{
			}
		ImageSource(IO::Directory& sDirectory,const char* sFileName) // Ditto, using a C string
			:directory(&sDirectory),fileName(sFileName)
			{
			}
		};
	
	private:
	typedef std::vector<ImageSource> ImageSourceList; // Type for lists of image sources
	
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		bool haveAutomaticMipMapGeneration; // Flag if the local OpenGL supports automatic mipmap generation
		bool haveAnisotropicFiltering; // Flag if the local OpenGL supports anisotropic filtering
		GLuint textureId; // ID of the texture object holding the currently displayed image
		unsigned int textureVersion; // Version number of the image currently held in the texture object
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	class PixelSnapperTool; // Forward declaration
	typedef Vrui::GenericToolFactory<PixelSnapperTool> PixelSnapperToolFactory; // Pixel snapper tool class uses the generic factory class
	
	class PixelSnapperTool:public Vrui::TransformTool,public Vrui::Application::Tool<ImageViewer> // A tool class to snap input device positions to pixel corners
		{
		friend class Vrui::GenericToolFactory<PixelSnapperTool>;
		
		/* Elements: */
		private:
		static PixelSnapperToolFactory* factory; // Pointer to the factory object for this class
		
		/* Constructors and destructors: */
		public:
		static void initClass(void); // Initializes the pixel snapper tool factory class
		PixelSnapperTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment);
		
		/* Methods: */
		virtual void initialize(void);
		virtual const Vrui::ToolFactory* getFactory(void) const;
		virtual void frame(void);
		};
	
	class PipetteTool; // Forward declaration
	typedef Vrui::GenericToolFactory<PipetteTool> PipetteToolFactory; // Pipette tool class uses the generic factory class
	
	class PipetteTool:public Vrui::Tool,public Vrui::Application::Tool<ImageViewer> // A tool class to pick color values from an image, derived from application tool class
		{
		friend class Vrui::GenericToolFactory<PipetteTool>;
		
		/* Elements: */
		private:
		static PipetteToolFactory* factory; // Pointer to the factory object for this class
		bool dragging; // Flag whether there is a current dragging operation
		int x0,y0; // Initial pixel position for dragging operations
		int x,y; // Current pixel position during dragging operations
		
		/* Private methods: */
		void setPixelPos(void); // Sets the current pixel position based on current input device selection ray
		
		/* Constructors and destructors: */
		public:
		static void initClass(void); // Initializes the pipette tool factory class
		PipetteTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment);
		
		/* Methods: */
		virtual const Vrui::ToolFactory* getFactory(void) const;
		virtual void buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData);
		virtual void frame(void);
		virtual void display(GLContextData& contextData) const;
		};
	
	friend class HomographySamplerTool;
	
	class HomographySamplerTool; // Forward declaration
	typedef Vrui::GenericToolFactory<HomographySamplerTool> HomographySamplerToolFactory; // HomographySampler tool class uses the generic factory class
	
	class HomographySamplerTool:public Vrui::Tool,public Vrui::Application::Tool<ImageViewer> // A tool class to sample a perspective-distorted image quadrilateral into a rectangular sub-image
		{
		friend class Vrui::GenericToolFactory<HomographySamplerTool>;
		
		/* Elements: */
		private:
		static HomographySamplerToolFactory* factory; // Pointer to the factory object for this class
		int numVertices; // Number of quad vertices that have already been set
		Point quad[4]; // Four corners of the distorted quadrilateral in order lower-left, lower-right, upper-right, upper-left
		Point edge[4]; // Four points along the edges of the distorted quadrilateral to approximate lens distortion
		Images::Size size; // Size of the rectangular result image in pixels
		bool dragging; // Flag whether there is a current dragging operation
		int dragIndex; // Index of quad corner currently being dragged
		Vector dragOffset; // Offset from device position to dragged vertex position during dragging
		
		/* Private methods: */
		Point calcPixelPos(void) const; // Returns the current pixel position based on current input device selection ray
		void resample(void); // Resamples the image based on the current quad
		void draw(void) const; // Draws the current quad
		
		/* Constructors and destructors: */
		public:
		static void initClass(void); // Initializes the homography sampler tool factory class
		HomographySamplerTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment);
		
		/* Methods: */
		virtual const Vrui::ToolFactory* getFactory(void) const;
		virtual void buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData);
		virtual void frame(void);
		virtual void display(GLContextData& contextData) const;
		};
	
	friend class HomographySamplerTool;
	
	/* Elements: */
	ImageSourceList imageSources; // List of image sources that can be loaded
	unsigned int numImages; // The number of image sources in the list
	unsigned int currentImage; // Index of the currently displayed image source
	int request; // A counter to order image loading requests
	Images::BaseImage image; // The currently displayed image
	int loaded; // Loading request counter of the currently displayed image
	unsigned int imageVersion; // Version number of the currently displayed image
	GLMotif::FileSelectionHelper imageHelper; // Helper object to load image files
	bool smoothPixels; // Flag to enable bilinear interpolation when magnifying images
	bool flipH; // Flag to flip images horizontally
	GLMotif::PopupMenu* mainMenu; // The application's main menu
	GLMotif::PopupWindow* selectorDialog; // Dialog window to select the currently displayed image
	GLMotif::TextFieldSlider* imageIndexSlider; // Slider to select the currently displayed image
	GLMotif::PopupWindow* infoDialog; // Dialog window displaying information about the currently displayed image
	GLMotif::TextField* imageIndex; // Index of the currently displayed image in the set
	GLMotif::TextField* imageNumImages; // Total number of images in the set
	GLMotif::TextField* imageDirectoryName; // Name of the directory containing the currently displayed image
	GLMotif::TextField* imageFileName; // The currently displayed image's file name
	GLMotif::TextField* imageSize[2]; // The currently displayed image's width and height
	GLMotif::TextField* imageNumChannels; // The currently displayed image's number of channels
	GLMotif::Label* imageChannelLayoutLabel1;
	GLMotif::TextField* imageChannelSize; // The currently displayed image's channel size in bytes
	GLMotif::Label* imageChannelLayoutLabel2;
	GLMotif::TextField* imageChannelType; // The currently displayed image's channel data type
	
	/* Private methods: */
	void addDirectory(IO::Directory& directory); // Adds all readable image files in the given directory to the image sources list
	Color getPixel(unsigned int x,unsigned int y) const; // Returns an RGBA color for the given pixel position
	void updateInfoDialog(void); // Updates the image information dialog after an image has been loaded
	void loadImageCompleteCallback(Threads::FunctionCall<int>& job); // Callback called when a new image has been loaded
	void loadImageCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData); // Callback called when a new image is to be loaded
	void showSelectorDialogButtonSelectedCallback(Misc::CallbackData* cbData); // Callback called when the image selector dialog is to be shown
	void showInfoDialogButtonSelectedCallback(Misc::CallbackData* cbData); // Callback called when the image information dialog is to be shown
	GLMotif::PopupMenu* createMainMenu(void); // Creates the application's main menu
	void imageIndexSliderValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData);
	GLMotif::PopupWindow* createSelectorDialog(void); // Creates the image selector dialog
	GLMotif::PopupWindow* createInfoDialog(void); // Creates the image information dialog
	
	/* Constructors and destructors: */
	public:
	ImageViewer(int& argc,char**& argv);
	virtual ~ImageViewer(void);
	
	/* Methods from class Vrui::Application: */
	virtual void display(GLContextData& contextData) const;
	virtual void resetNavigation(void);
	virtual void eventCallback(EventID eventId,Vrui::InputDevice::ButtonCallbackData* cbData);
	
	/* Methods from class GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	};

#endif
