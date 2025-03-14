/***********************************************************************
ListBox - Class for widgets containing lists of text strings.
Copyright (c) 2008-2024 Oliver Kreylos

This file is part of the GLMotif Widget Library (GLMotif).

The GLMotif Widget Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GLMotif Widget Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the GLMotif Widget Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef GLMOTIF_LISTBOX_INCLUDED
#define GLMOTIF_LISTBOX_INCLUDED

#include <vector>
#include <Misc/CallbackData.h>
#include <Misc/CallbackList.h>
#include <GL/gl.h>
#include <GL/GLLabel.h>
#include <GLMotif/Widget.h>

/* Forward declarations: */
class GLFont;

namespace GLMotif {

class ListBox:public Widget
	{
	/* Embedded classes: */
	public:
	enum SelectionMode // Enumerated type for selection modes
		{
		ATMOST_ONE,ALWAYS_ONE,MULTIPLE
		};
	
	class CallbackData:public Misc::CallbackData // Base class for callback data sent by list boxes
		{
		/* Elements: */
		public:
		ListBox* listBox; // Pointer to the list box that caused the event
		
		/* Constructors and destructors: */
		CallbackData(ListBox* sListBox)
			:listBox(sListBox)
			{
			}
		};
	
	class ListChangedCallbackData:public CallbackData // Callback data when the list of items changes
		{
		/* Embedded classes: */
		public:
		enum ChangeReason // Enumerated type for different change reasons
			{
			ITEM_INSERTED,ITEM_CHANGED,ITEM_REMOVED,LIST_CLEARED
			};
		
		/* Elements: */
		ChangeReason reason; // Reason for the item list change
		int item; // Index of inserted, changed, or removed item
		
		/* Constructors and destructors: */
		ListChangedCallbackData(ListBox* sListBox,ChangeReason sReason,int sItem)
			:CallbackData(sListBox),
			 reason(sReason),item(sItem)
			{
			}
		};
	
	class PageChangedCallbackData:public CallbackData // Callback data when the properties of the visible page change
		{
		/* Embedded classes: */
		public:
		enum ChangeReason // Enumerated type for different change reasons
			{
			POSITION_CHANGED=0x1,NUMITEMS_CHANGED=0x2,PAGESIZE_CHANGED=0x4,
			HORIZONTALOFFSET_CHANGED=0x8,MAXITEMWIDTH_CHANGED=0x10,LISTWIDTH_CHANGED=0x20
			};
		
		/* Elements: */
		public:
		int reasonMask; // Bitwise or of reasons for change
		int position; // New list position
		int numItems; // New number of list items
		int pageSize; // New list page size
		GLfloat horizontalOffset; // New horizontal offset
		GLfloat maxItemWidth; // New maximum width of any visible list item
		GLfloat listWidth; // New list width
		
		/* Constructors and destructors: */
		PageChangedCallbackData(ListBox* sListBox,int sReasonMask,int sPosition,int sNumItems,int sPageSize,GLfloat sHorizontalOffset,GLfloat sMaxItemWidth,GLfloat sListWidth)
			:CallbackData(sListBox),
			 reasonMask(sReasonMask),
			 position(sPosition),numItems(sNumItems),pageSize(sPageSize),
			 horizontalOffset(sHorizontalOffset),maxItemWidth(sMaxItemWidth),listWidth(sListWidth)
			{
			}
		};
	
	class ValueChangedCallbackData:public CallbackData // Callback data when the selected item in a single-selection list changes; not generated for multiple-selection lists
		{
		/* Elements: */
		public:
		int oldSelectedItem; // Previously selected list item
		int newSelectedItem; // Newly selected list item
		bool interactive; // Flag if the callback is in response to a direct user action
		
		/* Constructors and destructors: */
		ValueChangedCallbackData(ListBox* sListBox,int sOldSelectedItem,int sNewSelectedItem,bool sInteractive)
			:CallbackData(sListBox),
			 oldSelectedItem(sOldSelectedItem),
			 newSelectedItem(sNewSelectedItem),
			 interactive(sInteractive)
			{
			}
		};
	
	class ItemSelectedCallbackData:public CallbackData // Callback data when an item gets selected with a double-click
		{
		/* Elements: */
		public:
		int selectedItem; // The double-clicked list item
		
		/* Constructors and destructors: */
		ItemSelectedCallbackData(ListBox* sListBox,int sSelectedItem)
			:CallbackData(sListBox),
			 selectedItem(sSelectedItem)
			{
			}
		};
	
	class SelectionChangedCallbackData:public CallbackData // Callback data when the set of selected items changes; generated for single- and multiple-selection lists
		{
		/* Embedded classes: */
		public:
		enum ChangeReason // Enumerated type for different change reasons
			{
			NUMITEMS_CHANGED,ITEM_SELECTED,ITEM_DESELECTED,SELECTION_CLEARED
			};
		
		/* Elements: */
		ChangeReason reason; // Reason for the selection change
		int item; // Index of selected or deselected item
		bool interactive; // Flag if the callback is in response to a direct user action
		
		/* Constructors and destructors: */
		SelectionChangedCallbackData(ListBox* sListBox,ChangeReason sReason,int sItem,bool sInteractive)
			:CallbackData(sListBox),
			 reason(sReason),item(sItem),interactive(sInteractive)
			{
			}
		};
	
	private:
	struct Item // Structure to hold list items
		{
		/* Elements: */
		public:
		char* item; // Pointer to item's string
		unsigned int itemLength; // Length of item's string
		bool selected; // Flag whether item is currently selected
		};
	
	struct PageSlot // Structure to hold state of a slot in a list box's visible page
		{
		/* Elements: */
		public:
		GLLabel* label; // Pointer to the label displaying the item currently in the slot
		Vector rightCorners[2]; // Positions of the bottom and top right corners of the page slot
		};
	
	/* Elements: */
	SelectionMode selectionMode; // List box's selection mode
	GLfloat marginWidth; // Width of margin around text strings
	GLfloat itemSep; // Vertical separation between text strings
	const GLFont* font; // Pointer to the font used to display list items
	int preferredWidth; // Preferred list box width in characters
	int preferredPageSize; // Preferred number of items visible in the list box
	bool autoResize; // Flag whether the list box shall attempt to resize its width to the visible items
	Color selectionFgColor; // Foreground color for selected list items
	Color selectionBgColor; // Background color for selected list items
	Box itemsBox; // Box surrounding list items
	std::vector<Item> items; // Vector of text strings
	int pageSize; // Number of items visible in the list box
	PageSlot* pageSlots; // Array of states of currently visible items
	int position; // Index of the top item currently visible in the list box
	GLfloat maxVisibleItemWidth; // Maximum width of currentbly visible items
	GLfloat horizontalOffset; // Horizontal offset for drawing all list items
	int lastSelectedItem; // Index of the most recently selected list item, or -1 if none
	Misc::CallbackList listChangedCallbacks; // List of callbacks to be called when the list of items changes
	Misc::CallbackList pageChangedCallbacks; // List of callbacks to be called when any parameters of the displayed page change
	Misc::CallbackList valueChangedCallbacks; // List of callbacks to be called when a different list item is selected
	Misc::CallbackList itemSelectedCallbacks; // List of callbacks to be called when a list item is double-clicked
	Misc::CallbackList selectionChangedCallbacks; // List of callbacks to be called when the selection state of a list item changes
	int lastClickedItem; // Index of item which received last button down event
	double lastClickTime; // Time of last pointer button down event, to detect double clicks
	int numClicks; // Number of clicks on the current selected item
	
	/* Private methods: */
	GLfloat calcMaxVisibleItemWidth(void) const; // Returns the maximum width of all visible list items
	void setPageSlotColor(GLLabel& label,bool selected); // Sets the colors of the given page slot label to reflect the given selection state
	void updatePageSlots(int itemsBegin,int itemsEnd); // Updates the page slot's labels in the given range of item indices
	void positionPageSlots(void); // Updates the positions of all page slots
	int updatePage(void); // Updates the stare of the visible page; returns a change mask for a PageChangedCallbackData structure
	int setPagePosition(int newPosition); // Sets the position of the visible page; returns a change mask for a PageChangedCallbackData structure
	void selectItem(int index,bool moveToPage,bool interactive); // Selects the given list item
	void deselectItem(int index,bool moveToPage,bool interactive); // De-selects the given list item
	
	/* Constructors and destructors: */
	public:
	ListBox(const char* sName,Container* sParent,SelectionMode sSelectionMode,int sPreferredWidth,int sPreferredPageSize,bool manageChild =true);
	virtual ~ListBox(void);
	
	/* Methods inherited from Widget: */
	virtual Vector calcNaturalSize(void) const;
	virtual void resize(const Box& newExterior);
	virtual void draw(GLContextData& contextData) const;
	virtual void pointerButtonDown(Event& event);
	virtual void pointerButtonUp(Event& event);
	virtual void pointerMotion(Event& event);
	virtual bool giveTextFocus(void);
	virtual void textControlEvent(const TextControlEvent& event);
	
	/* New methods: */
	
	/* Methods to query or change the list box's appearance and behavior: */
	const GLFont* getFont(void) const // Returns the font used by the list box items
		{
		return font;
		}
	void setMarginWidth(GLfloat newMarginWidth); // Changes the width of the margin around the list box items
	void setItemSeparation(GLfloat newItemSep); // Sets the separation between list box items
	void setAutoResize(bool newAutoResize); // Sets the automatic resizing flag
	void setSelectionFgColor(const Color& newSelectionFgColor); // Sets the foreground color for selected list items
	void setSelectionBgColor(const Color& newSelectionBgColor); // Sets the background color for selected list items
	
	/* Methods to query or change the list box's list of items: */
	int getNumItems(void) const // Returns the number of items in the list box
		{
		return items.size();
		}
	const char* getItem(int index) const // Returns the text of the given item
		{
		return items[index].item;
		}
	void insertItem(int index,const char* newItem,bool moveToPage =false); // Inserts a new item before the current item of the given index and moves it to the page if it is not visible and moveToPage is true
	int addItem(const char* newItem,bool moveToPage =false) // Adds a new item to the end of the list and moves it to the page if it is not visible and moveToPage is true; returns index of new item
		{
		/* Insert the item at the end of the list: */
		insertItem(int(items.size()),newItem,moveToPage);
		
		/* Return the new item's index: */
		return int(items.size())-1;
		}
	void setItem(int index,const char* newItem); // Sets the text of the given item
	void removeItem(int index); // Removes the item at the given index
	void clear(void); // Clears the list
	
	/* Methods to query or change the list box's page of visible items: */
	int getPageSize(void) const // Returns the list box's current page size
		{
		return pageSize;
		}
	int getPosition(void) const // Returns the index of the item at the top of the list box
		{
		return position;
		}
	void setPosition(int newPosition); // Sets the index of the item at the top of the list box
	GLfloat getListBoxWidth(void) const // Returns the current width of the item list
		{
		return itemsBox.size[0];
		}
	GLfloat getVisibleItemsWidth(void) const // Returns the maximum width of the currently displayed list items
		{
		return maxVisibleItemWidth;
		}
	GLfloat getHorizontalOffset(void) const // Returns the current horizontal offset
		{
		return horizontalOffset;
		}
	void setHorizontalOffset(GLfloat newHorizontalOffset); // Sets a new horizontal offset to display list items
	
	/* Methods to query and change the list box's set of selected items: */
	int getSelectedItem(void) const // Returns the index of the selected item in a single-selection list box, or -1 if no item is selected
		{
		return lastSelectedItem;
		}
	bool isItemSelected(int index) const // Returns true if the given item is currently selected
		{
		return items[index].selected;
		}
	int getNumSelectedItems(void) const; // Returns the number of currently selected items
	std::vector<int> getSelectedItems(void) const; // Returns list of indices of all currently selected items
	void selectItem(int index,bool moveToPage =false) // Selects the given list item and moves it to the page if it is not visible and the flag is true; in single-selection list boxes, deselects previously selected item
		{
		/* Select an item non-interactively: */
		selectItem(index,moveToPage,false);
		}
	void deselectItem(int index,bool moveToPage =false) // Deselects the given list item and moves it to the page if it is not visible and the flag is true; ignored if selection mode is ALWAYS_ONE
		{
		/* De-select an item non-interactively: */
		deselectItem(index,moveToPage,false);
		}
	void clearSelection(void); // Deselects all selected items; ignored if selection mode is ALWAYS_ONE
	
	/* Methods to query the list box's callbacks: */
	Misc::CallbackList& getListChangedCallbacks(void) // Returns list of list changed callbacks
		{
		return listChangedCallbacks;
		}
	Misc::CallbackList& getPageChangedCallbacks(void) // Returns list of page changed callbacks
		{
		return pageChangedCallbacks;
		}
	Misc::CallbackList& getValueChangedCallbacks(void) // Returns list of value changed callbacks
		{
		return valueChangedCallbacks;
		}
	Misc::CallbackList& getItemSelectedCallbacks(void) // Returns list of item selected callbacks
		{
		return itemSelectedCallbacks;
		}
	Misc::CallbackList& getSelectionChangedCallbacks(void) // Returns list of selection changed callbacks
		{
		return selectionChangedCallbacks;
		}
	};

}

#endif
