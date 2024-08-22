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

#include <string.h>
#include <utility>
#include <Math/Math.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLTexCoordTemplates.h>
#include <GL/GLVertexTemplates.h>
#include <GL/GLTexEnvTemplates.h>
#include <GL/GLContextData.h>
#include <GL/GLFont.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/Event.h>
#include <GLMotif/TextControlEvent.h>
#include <GLMotif/Container.h>

#include <GLMotif/ListBox.h>

namespace GLMotif {

/************************
Methods of class ListBox:
************************/

GLfloat ListBox::calcMaxVisibleItemWidth(void) const
	{
	/* Find the maximum natural width of all list items on the current page: */
	GLfloat result=0.0f;
	int pageEnd=Math::min(pageSize,int(items.size())-position);
	for(int i=0;i<pageEnd;++i)
		result=Math::max(result,pageSlots[i].label->calcNaturalSize()[0]);
	
	return result;
	}

void ListBox::setPageSlotColor(GLLabel& label,bool selected)
	{
	/* Set the label's foreground and background colors based on the list item's selection state: */
	label.setBackground(selected?selectionBgColor:backgroundColor);
	label.setForeground(selected?selectionFgColor:foregroundColor);
	}

void ListBox::updatePageSlots(int itemsBegin,int itemsEnd)
	{
	itemsEnd=Math::min(Math::min(itemsEnd,position+pageSize),int(items.size()));
	for(int i=itemsBegin;i<itemsEnd;++i)
		{
		/* Update the existing label to reflect its list item: */
		Item& item=items[i];
		GLLabel& label=*pageSlots[i-position].label;
		label.setString(item.item,item.item+item.itemLength);
		setPageSlotColor(label,item.selected);
		}
	}

void ListBox::positionPageSlots(void)
	{
	/* Update the page slot states: */
	int pageEnd=Math::min(pageSize,int(items.size())-position);
	GLfloat slotH=font->getTextHeight();
	Vector slotOrigin=itemsBox.getCorner(2);
	slotOrigin[0]-=horizontalOffset;
	slotOrigin[1]-=slotH;
	GLfloat slotRightX=itemsBox.origin[0]+itemsBox.size[0];
	for(int i=0;i<pageEnd;++i,slotOrigin[1]-=slotH+itemSep)
		{
		PageSlot& ps=pageSlots[i];
		
		/* Position the page slot label relative to the visible page: */
		ps.label->resetBox();
		ps.label->setOrigin(slotOrigin);
		
		/* Clip the label to the page's interior: */
		ps.label->clipBox(itemsBox);
		
		/* Set the page slot's right corners: */
		ps.rightCorners[0]=Vector(slotRightX,Math::max(slotOrigin[1],itemsBox.origin[1]),slotOrigin[2]);
		ps.rightCorners[1]=Vector(slotRightX,Math::max(slotOrigin[1]+slotH,itemsBox.origin[1]),slotOrigin[2]);
		}
	}

int ListBox::updatePage(void)
	{
	int pageChangeReasonMask=0x0;
	
	/* Calculate the new maximum visible item width: */
	GLfloat newMaxVisibleItemWidth=calcMaxVisibleItemWidth();
	if(maxVisibleItemWidth!=newMaxVisibleItemWidth)
		pageChangeReasonMask|=PageChangedCallbackData::MAXITEMWIDTH_CHANGED;
	maxVisibleItemWidth=newMaxVisibleItemWidth;
	
	/* Limit the horizontal offset to the valid range: */
	GLfloat newHorizontalOffset=Math::max(Math::min(horizontalOffset,maxVisibleItemWidth-itemsBox.size[0]),0.0f);
	if(horizontalOffset!=newHorizontalOffset)
		pageChangeReasonMask|=PageChangedCallbackData::HORIZONTALOFFSET_CHANGED;
	horizontalOffset=newHorizontalOffset;
	
	/* Position all page slots' labels: */
	positionPageSlots();
	
	return pageChangeReasonMask;
	}

int ListBox::setPagePosition(int newPosition)
	{
	/* Keep track of changes to the visible page: */
	int pageChangeReasonMask=PageChangedCallbackData::POSITION_CHANGED;
	
	/* Shift the page slots around to re-use as much of the existing state as possible: */
	int offset=newPosition-position;
	position=newPosition;
	if(offset>=0)
		{
		/* Swap slots starting from the top: */
		for(int i=offset;i<pageSize;++i)
			std::swap(pageSlots[i-offset].label,pageSlots[i].label);
		
		/* Update the page slots that couldn't be moved: */
		updatePageSlots(position+pageSize-offset,position+pageSize);
		}
	else
		{
		/* Swap slots starting from the bottom: */
		for(int i=pageSize-1+offset;i>=0;--i)
			std::swap(pageSlots[i-offset].label,pageSlots[i].label);
		
		/* Update the page slots that couldn't be moved: */
		updatePageSlots(position,position-offset);
		}
	
	/* Update the page: */
	pageChangeReasonMask|=updatePage();
	
	return pageChangeReasonMask;
	}

void ListBox::selectItem(int index,bool moveToPage,bool interactive)
	{
	/* Bail out if the request is invalid or a no-op: */
	if(index<0||size_t(index)>=items.size())
		index=-1;
	if(selectionMode==ALWAYS_ONE&&index==-1)
		return;
	if(selectionMode==MULTIPLE&&(index<0||items[index].selected))
		return;
	if(selectionMode!=MULTIPLE&&index==lastSelectedItem)
		return;
	
	/* Deselect the previously selected item in single-item selection modes: */
	if(selectionMode!=MULTIPLE&&lastSelectedItem>=0)
		{
		/* Deselect the last selected item: */
		items[lastSelectedItem].selected=false;
		
		{
		/* Call the selection changed callbacks: */
		SelectionChangedCallbackData cbData(this,SelectionChangedCallbackData::ITEM_DESELECTED,lastSelectedItem,interactive);
		selectionChangedCallbacks.call(&cbData);
		}
		
		/* Update the old selected item's page slot if it is visible: */
		if(lastSelectedItem>=position&&lastSelectedItem<position+pageSize)
			setPageSlotColor(*pageSlots[lastSelectedItem-position].label,false);
		}
	
	/* Check if the item is valid: */
	if(index>=0)
		{
		/* Select the item: */
		items[index].selected=true;
		
		{
		/* Call the selection changed callbacks: */
		SelectionChangedCallbackData cbData(this,SelectionChangedCallbackData::ITEM_SELECTED,index,interactive);
		selectionChangedCallbacks.call(&cbData);
		}
		
		if(index>=position&&index<position+pageSize)
			{
			/* Update the selected item's page slot: */
			setPageSlotColor(*pageSlots[index-position].label,true);
			}
		else if(moveToPage)
			{
			/* Move the selected item to the page: */
			if(index<position)
				setPosition(index);
			else
				setPosition(index-(pageSize-1));
			}
		}
	
	/* Remember the last selected item: */
	int oldLastSelectedItem=lastSelectedItem;
	lastSelectedItem=index;
	
	{
	/* Call the value changed callbacks: */
	ValueChangedCallbackData cbData(this,oldLastSelectedItem,lastSelectedItem,interactive);
	valueChangedCallbacks.call(&cbData);
	}
	
	/* Invalidate the visual representation: */
	update();
	}

void ListBox::deselectItem(int index,bool moveToPage,bool interactive)
	{
	/* Bail out if the request is invalid or a no-op: */
	if(selectionMode==ALWAYS_ONE)
		return;
	if(index<0||size_t(index)>=items.size())
		return;
	if(!items[index].selected)
		return;
	
	/* Deselect the item: */
	items[index].selected=false;
	
	{
	/* Call the selection changed callbacks: */
	SelectionChangedCallbackData cbData(this,SelectionChangedCallbackData::ITEM_DESELECTED,index,interactive);
	selectionChangedCallbacks.call(&cbData);
	}
	
	if(index>=position&&index<position+pageSize)
		{
		/* Update the selected item's page slot: */
		setPageSlotColor(*pageSlots[index-position].label,false);
		}
	else if(moveToPage)
		{
		/* Move the selected item to the page: */
		if(index<position)
			setPosition(index);
		else
			setPosition(index-(pageSize-1));
		}
	
	/* Update the last selected item: */
	if(selectionMode!=MULTIPLE)
		{
		/* Select the invalid element: */
		int oldLastSelectedItem=lastSelectedItem;
		lastSelectedItem=-1;
		
		/* Call the value changed callbacks: */
		ValueChangedCallbackData cbData(this,oldLastSelectedItem,lastSelectedItem,interactive);
		valueChangedCallbacks.call(&cbData);
		}
	else if(lastSelectedItem!=index)
		{
		/* Select the deselected element: */
		int oldLastSelectedItem=lastSelectedItem;
		lastSelectedItem=index;
		
		/* Call the value changed callbacks: */
		ValueChangedCallbackData cbData(this,oldLastSelectedItem,lastSelectedItem,interactive);
		valueChangedCallbacks.call(&cbData);
		}
	
	/* Invalidate the visual representation: */
	update();
	}

ListBox::ListBox(const char* sName,Container* sParent,ListBox::SelectionMode sSelectionMode,int sPreferredWidth,int sPreferredPageSize,bool sManageChild)
	:Widget(sName,sParent,false),
	 selectionMode(sSelectionMode),
	 marginWidth(0.0f),itemSep(0.0f),
	 font(0),
	 preferredWidth(sPreferredWidth),preferredPageSize(sPreferredPageSize),
	 autoResize(false),
	 itemsBox(Vector(0.0f,0.0f,0.0f),Vector(0.0f,0.0f,0.0f)),
	 pageSize(0),pageSlots(0),
	 position(0),
	 maxVisibleItemWidth(0.0f),
	 horizontalOffset(0.0f),
	 lastSelectedItem(-1),
	 lastClickedItem(-1),lastClickTime(0.0),numClicks(0)
	{
	/* Get the style sheet: */
	const StyleSheet* ss=getStyleSheet();
	
	/* Get the font: */
	font=ss->font;
	
	setBorderWidth(ss->textfieldBorderWidth);
	setBorderType(Widget::LOWERED);
	setBackgroundColor(ss->textfieldBgColor);
	setForegroundColor(ss->textfieldFgColor);
	marginWidth=ss->textfieldMarginWidth;
	selectionFgColor=ss->selectionFgColor;
	selectionBgColor=ss->selectionBgColor;
	
	/* Manage me: */
	if(sManageChild)
		manageChild();
	}

ListBox::~ListBox(void)
	{
	/* Delete the page slots: */
	for(int i=0;i<pageSize;++i)
		delete pageSlots[i].label;
	delete[] pageSlots;
	
	/* Delete the list items: */
	for(std::vector<Item>::iterator iIt=items.begin();iIt!=items.end();++iIt)
		delete[] iIt->item;
	}

Vector ListBox::calcNaturalSize(void) const
	{
	/* Calculate the list box's preferred size: */
	Vector result;
	result[0]=GLfloat(preferredWidth)*font->getCharacterWidth();
	if(autoResize&&result[0]<maxVisibleItemWidth)
		result[0]=maxVisibleItemWidth;
	result[0]+=2.0f*marginWidth;
	result[1]=GLfloat(preferredPageSize)*(font->getTextHeight()+itemSep)-itemSep+2.0f*marginWidth;
	result[2]=0.0f;
	
	return calcExteriorSize(result);
	}

void ListBox::resize(const Box& newExterior)
	{
	/* Resize the parent class widget: */
	Widget::resize(newExterior);
	
	/* Keep track of changing page parameters: */
	int pageChangeReasonMask=0x0;
	
	/* Resize the item box: */
	GLfloat oldWidth=itemsBox.size[0];
	itemsBox=getInterior();
	itemsBox.doInset(Vector(marginWidth,marginWidth,0.0f));
	if(oldWidth!=itemsBox.size[0])
		pageChangeReasonMask|=PageChangedCallbackData::LISTWIDTH_CHANGED;
	
	/* Calculate and adapt to the new page size: */
	GLfloat itemH=font->getTextHeight();
	int newPageSize=Math::max(int(Math::ceil(itemsBox.size[1]/(itemH+itemSep))),0);
	if(newPageSize!=pageSize)
		{
		/* Resize the page slot array: */
		pageChangeReasonMask|=PageChangedCallbackData::PAGESIZE_CHANGED;
		PageSlot* newPageSlots=new PageSlot[newPageSize];
		
		/* Limit the page position to the valid range: */
		int numListItems=int(items.size());
		int newPosition=Math::max(Math::min(position,numListItems-pageSize),0);
		if(position!=newPosition)
			pageChangeReasonMask|=PageChangedCallbackData::POSITION_CHANGED;
		
		/* Determine the range of list items that are common to the old and new pages: */
		int commonBegin=Math::max(position,newPosition);
		int commonEnd=Math::min(position+pageSize,newPosition+newPageSize);
		
		/* Delete old page slots outside the common range: */
		for(int i=0;i<commonBegin-position;++i)
			delete pageSlots[i].label;
		for(int i=commonEnd-position;i<pageSize;++i)
			delete pageSlots[i].label;
		
		/* Populate the new page slots: */
		for(int i=newPosition;i<newPosition+newPageSize;++i)
			{
			PageSlot& nps=newPageSlots[i-newPosition];
			if(i>=commonBegin&&i<commonEnd)
				{
				/* Move the existing list item and label from the old page slots: */
				nps=pageSlots[i-position];
				}
			else if(i<numListItems)
				{
				/* Assign a new list item and create a new label: */
				Item& item=items[i];
				nps.label=new GLLabel(item.item,item.item+item.itemLength,*font);
				setPageSlotColor(*nps.label,item.selected);
				}
			else
				{
				/* Create a dummy label: */
				nps.label=new GLLabel("",*font);
				}
			}
		
		/* Update the current page size and list position: */
		pageSize=newPageSize;
		delete[] pageSlots;
		pageSlots=newPageSlots;
		position=newPosition;
		
		/* Calculate the new maximum visible item width: */
		GLfloat newMaxVisibleItemWidth=calcMaxVisibleItemWidth();
		if(maxVisibleItemWidth!=newMaxVisibleItemWidth)
			pageChangeReasonMask|=PageChangedCallbackData::MAXITEMWIDTH_CHANGED;
		maxVisibleItemWidth=newMaxVisibleItemWidth;
		}
	
	/* Limit the horizontal offset to the valid range: */
	GLfloat newHorizontalOffset=Math::max(Math::min(horizontalOffset,maxVisibleItemWidth-itemsBox.size[0]),0.0f);
	if(horizontalOffset!=newHorizontalOffset)
		pageChangeReasonMask|=PageChangedCallbackData::HORIZONTALOFFSET_CHANGED;
	horizontalOffset=newHorizontalOffset;
	
	/* Position the currently visible items: */
	positionPageSlots();
	
	if(pageChangeReasonMask!=0x0)
		{
		/* Send a page change callback: */
		PageChangedCallbackData cbData(this,pageChangeReasonMask,position,int(items.size()),pageSize,horizontalOffset,maxVisibleItemWidth,itemsBox.size[0]);
		pageChangedCallbacks.call(&cbData);
		}
	}

void ListBox::draw(GLContextData& contextData) const
	{
	/* Draw the parent class widget: */
	Widget::draw(contextData);
	
	glColor(backgroundColor);
	glNormal3f(0.0f,0.0f,1.0f);
	
	/* Check whether the page is non-empty: */
	if(position<int(items.size()))
		{
		/* Draw the margin around the list items: */
		int pageEnd=Math::min(pageSize,int(items.size())-position);
		glBegin(GL_TRIANGLE_FAN);
		glVertex(getInterior().getCorner(0));
		glVertex(getInterior().getCorner(1));
		glVertex(pageSlots[pageEnd-1].rightCorners[0]);
		glVertex(pageSlots[pageEnd-1].label->getLabelBox().getCorner(1));
		for(int i=pageEnd-1;i>=0;--i)
			{
			glVertex(pageSlots[i].label->getLabelBox().getCorner(0));
			glVertex(pageSlots[i].label->getLabelBox().getCorner(2));
			}
		glVertex(getInterior().getCorner(2));
		glEnd();
		glBegin(GL_TRIANGLE_FAN);
		glVertex(getInterior().getCorner(3));
		glVertex(getInterior().getCorner(2));
		glVertex(pageSlots[0].label->getLabelBox().getCorner(2));
		glVertex(pageSlots[0].label->getLabelBox().getCorner(3));
		for(int i=0;i<pageEnd;++i)
			{
			glVertex(pageSlots[i].rightCorners[1]);
			glVertex(pageSlots[i].rightCorners[0]);
			}
		glVertex(getInterior().getCorner(1));
		glEnd();
		
		/* Fill the right side of the list box: */
		std::vector<Item>::const_iterator itemIt=items.begin()+position;
		glBegin(GL_QUAD_STRIP);
		for(int i=0;i<pageEnd;++i,++itemIt)
			{
			glVertex(pageSlots[i].rightCorners[1]);
			glVertex(pageSlots[i].label->getLabelBox().getCorner(3));
			if(itemIt->selected)
				{
				/* Draw the item's right side in the selection background color: */
				glColor(selectionBgColor);
				glVertex(pageSlots[i].rightCorners[1]);
				glVertex(pageSlots[i].label->getLabelBox().getCorner(3));
				glVertex(pageSlots[i].rightCorners[0]);
				glVertex(pageSlots[i].label->getLabelBox().getCorner(1));
				glColor(backgroundColor);
				}
			glVertex(pageSlots[i].rightCorners[0]);
			glVertex(pageSlots[i].label->getLabelBox().getCorner(1));
			}
		glEnd();
		
		/* Draw the list item separators: */
		glBegin(GL_QUADS);
		for(int i=0;i<pageEnd-1;++i)
			{
			glVertex(pageSlots[i].label->getLabelBox().getCorner(1));
			glVertex(pageSlots[i].label->getLabelBox().getCorner(0));
			glVertex(pageSlots[i+1].label->getLabelBox().getCorner(2));
			glVertex(pageSlots[i+1].label->getLabelBox().getCorner(3));
			}
		glEnd();
		
		/* Draw the list items: */
		for(int i=0;i<pageEnd;++i)
			pageSlots[i].label->draw(contextData);
		}
	else
		{
		/* Draw the interior of the list box: */
		glBegin(GL_QUADS);
		glVertex(getInterior().getCorner(0));
		glVertex(getInterior().getCorner(1));
		glVertex(getInterior().getCorner(3));
		glVertex(getInterior().getCorner(2));
		glEnd();
		}
	}

void ListBox::pointerButtonDown(Event& event)
	{
	/* Determine which page slot was clicked on: */
	Point p=event.getWidgetPoint().getPoint();
	if(p[0]>=itemsBox.origin[0]&&p[0]<itemsBox.origin[0]+itemsBox.size[0])
		{
		int pageEnd=Math::min(pageSize,int(items.size())-position);
		for(int i=0;i<pageEnd;++i)
			{
			PageSlot& ps=pageSlots[i];
			if(p[1]>=ps.rightCorners[0][1]&&p[1]<ps.rightCorners[1][1])
				{
				/* Check for a multi-click: */
				if(lastClickedItem==position+i&&getManager()->getTime()-lastClickTime<getManager()->getStyleSheet()->multiClickTime)
					{
					/* Increase the click counter: */
					++numClicks;
					}
				else
					{
					/* Toggle the list item's selection state: */
					if(items[position+i].selected)
						deselectItem(position+i,false,true);
					else
						selectItem(position+i,false,true);
					
					/* Reset the click counter: */
					numClicks=1;
					}
				
				/* Remember the click event: */
				lastClickedItem=position+i;
				lastClickTime=getManager()->getTime();
				
				/* Stop looking: */
				break;
				}
			}
		}
	
	/* Request text focus: */
	getManager()->requestFocus(this);
	}

void ListBox::pointerButtonUp(Event& event)
	{
	if(numClicks>=2)
		{
		/* Call the item selection callbacks: */
		ItemSelectedCallbackData cbData(this,lastClickedItem);
		itemSelectedCallbacks.call(&cbData);
		
		/* Reset the click counter: */
		numClicks=0;
		}
	}

void ListBox::pointerMotion(Event& event)
	{
	}

bool ListBox::giveTextFocus(void)
	{
	return true;
	}

void ListBox::textControlEvent(const TextControlEvent& event)
	{
	switch(event.event)
		{
		case TextControlEvent::CURSOR_TEXT_START:
		case TextControlEvent::CURSOR_START:
			setPosition(0);
			break;
		
		case TextControlEvent::CURSOR_PAGE_UP:
			setPosition(position-pageSize);
			break;
		
		case TextControlEvent::CURSOR_UP:
			setPosition(position-1);
			break;
		
		case TextControlEvent::CURSOR_DOWN:
			setPosition(position+1);
			break;
		
		case TextControlEvent::CURSOR_PAGE_DOWN:
			setPosition(position+pageSize);
			break;
		
		case TextControlEvent::CURSOR_END:
		case TextControlEvent::CURSOR_TEXT_END:
			setPosition(items.size());
			break;
		
		default:
			;
		}
	}

void ListBox::setMarginWidth(GLfloat newMarginWidth)
	{
	/* Set the margin width: */
	marginWidth=newMarginWidth;
	
	if(isManaged)
		{
		/* Try to resize the widget to accommodate the new setting: */
		parent->requestResize(this,calcNaturalSize());
		}
	}

void ListBox::setItemSeparation(GLfloat newItemSep)
	{
	/* Set the item separation: */
	itemSep=newItemSep;
	
	if(isManaged)
		{
		/* Try to resize the widget to accommodate the new setting: */
		parent->requestResize(this,calcNaturalSize());
		}
	}

void ListBox::setAutoResize(bool newAutoResize)
	{
	/* Set the autoresize flag: */
	autoResize=newAutoResize;
	
	if(autoResize&&itemsBox.size[0]<maxVisibleItemWidth)
		{
		/* Resize the list box to accommodate the largest visible item: */
		if(isManaged)
			parent->requestResize(this,calcNaturalSize());
		else
			resize(Box(Vector(0.0f,0.0f,0.0f),calcNaturalSize()));
		}
	}

void ListBox::setSelectionFgColor(const Color& newSelectionFgColor)
	{
	/* Set the new color: */
	selectionFgColor=newSelectionFgColor;
	
	/* Update the colors of selected visible list items: */
	int pageEnd=Math::min(pageSize,int(items.size())-position);
	for(int i=0;i<pageEnd;++i)
		if(items[position+i].selected)
			pageSlots[i].label->setForeground(selectionFgColor);
	}

void ListBox::setSelectionBgColor(const Color& newSelectionBgColor)
	{
	/* Set the new color: */
	selectionBgColor=newSelectionBgColor;
	
	/* Update the colors of selected visible list items: */
	int pageEnd=Math::min(pageSize,int(items.size())-position);
	for(int i=0;i<pageEnd;++i)
		if(items[position+i].selected)
			pageSlots[i].label->setBackground(selectionBgColor);
	}

void ListBox::insertItem(int index,const char* newItem,bool moveToPage)
	{
	/* Keep track of changes to the page state: */
	int pageChangedReasonMask=PageChangedCallbackData::NUMITEMS_CHANGED;
	
	/* Update the page position to reflect the caller's intent: */
	if(moveToPage)
		{
		if(index<position)
			{
			/* Move the page so that the newly inserted item will be at the top: */
			pageChangedReasonMask|=setPagePosition(index);
			}
		else if(index>position+(pageSize-1))
			{
			/* Move the page so that the newly inserted item will be at the bottom: */
			pageChangedReasonMask|=setPagePosition(index-(pageSize-1));
			}
		}
	else if(index<=position&&position+pageSize<int(items.size()))
		{
		/* Update the page position so that the list of displayed items doesn't change: */
		++position;
		pageChangedReasonMask|=PageChangedCallbackData::POSITION_CHANGED;
		}
	
	/* Add the new item to the list: */
	Item it;
	it.itemLength=strlen(newItem);
	it.item=new char[it.itemLength+1];
	memcpy(it.item,newItem,it.itemLength+1);
	it.selected=false;
	items.insert(items.begin()+index,it);
	
	{
	/* Call the list changed callbacks: */
	ListChangedCallbackData cbData(this,ListChangedCallbackData::ITEM_INSERTED,index);
	listChangedCallbacks.call(&cbData);
	}
	
	{
	/* Call the selection change callbacks: */
	SelectionChangedCallbackData cbData(this,SelectionChangedCallbackData::NUMITEMS_CHANGED,-1,false);
	selectionChangedCallbacks.call(&cbData);
	}
	
	/* Update the selected item if it is affected: */
	if(index<=lastSelectedItem)
		{
		/* Adjust the selected item's index: */
		++lastSelectedItem;
		
		/* Call the value changed callbacks: */
		ValueChangedCallbackData cbData(this,lastSelectedItem-1,lastSelectedItem,false);
		valueChangedCallbacks.call(&cbData);
		}
	
	/* Select this item if it is the first one in an always-one list: */
	if(lastSelectedItem==-1&&selectionMode==ALWAYS_ONE)
		{
		/* Select the new item: */
		items[index].selected=it.selected=true;
		lastSelectedItem=index;
		
		{
		/* Call the selection change callbacks: */
		SelectionChangedCallbackData cbData(this,SelectionChangedCallbackData::ITEM_SELECTED,lastSelectedItem,false);
		selectionChangedCallbacks.call(&cbData);
		}
		
		{
		/* Call the value changed callbacks: */
		ValueChangedCallbackData cbData(this,-1,lastSelectedItem,false);
		valueChangedCallbacks.call(&cbData);
		}
		}
	
	/* Recreate the visible page if it changed: */
	if(index>=position&&index<position+pageSize)
		{
		/* Make room in the visible page: */
		GLLabel* lastLabel=pageSlots[pageSize-1].label;
		for(int i=pageSize-1;position+i>index;--i)
			pageSlots[i].label=pageSlots[i-1].label;
		
		/* Create a label for the new item: */
		lastLabel->setString(it.item,it.item+it.itemLength);
		setPageSlotColor(*lastLabel,it.selected);
		pageSlots[index-position].label=lastLabel;
		
		/* Update the page: */
		pageChangedReasonMask|=updatePage();
		}
	
	{
	/* Call the page change callbacks: */
	PageChangedCallbackData cbData(this,pageChangedReasonMask,position,int(items.size()),pageSize,horizontalOffset,maxVisibleItemWidth,itemsBox.size[0]);
	pageChangedCallbacks.call(&cbData);
	}
	
	/* Resize the listbox if enabled and necessary: */
	if(autoResize&&itemsBox.size[0]<maxVisibleItemWidth)
		{
		if(isManaged)
			parent->requestResize(this,calcNaturalSize());
		else
			resize(Box(Vector(0.0f,0.0f,0.0f),calcNaturalSize()));
		}
	
	/* Invalidate the visual representation: */
	update();
	}

void ListBox::setItem(int index,const char* newItem)
	{
	/* Replace the list item: */
	Item& it=items[index];
	it.itemLength=strlen(newItem);
	delete[] it.item;
	it.item=new char[it.itemLength+1];
	memcpy(it.item,newItem,it.itemLength+1);
	
	{
	/* Call the list changed callbacks: */
	ListChangedCallbackData cbData(this,ListChangedCallbackData::ITEM_CHANGED,index);
	listChangedCallbacks.call(&cbData);
	}
	
	/* Keep track of changes to the page state: */
	int pageChangedReasonMask=0x0;
	
	if(index>=position&&index<position+pageSize)
		{
		/* Update the page slot's label: */
		pageSlots[index-position].label->setString(it.item,it.item+it.itemLength);
		
		/* Update the page: */
		pageChangedReasonMask|=updatePage();
		}
	
	/* Call the page change callbacks if necessary: */
	if(pageChangedReasonMask!=0x0)
		{
		PageChangedCallbackData cbData(this,pageChangedReasonMask,position,int(items.size()),pageSize,horizontalOffset,maxVisibleItemWidth,itemsBox.size[0]);
		pageChangedCallbacks.call(&cbData);
		}
	
	/* Resize the listbox if enabled and necessary: */
	if(autoResize&&itemsBox.size[0]<maxVisibleItemWidth)
		{
		if(isManaged)
			parent->requestResize(this,calcNaturalSize());
		else
			resize(Box(Vector(0.0f,0.0f,0.0f),calcNaturalSize()));
		}
	
	/* Invalidate the visual representation: */
	update();
	}

void ListBox::removeItem(int index)
	{
	/* Remove the list item: */
	delete[] items[index].item;
	items.erase(items.begin()+index);
	
	{
	/* Call the list changed callbacks: */
	ListChangedCallbackData cbData(this,ListChangedCallbackData::ITEM_REMOVED,index);
	listChangedCallbacks.call(&cbData);
	}
	
	{
	/* Call the selection change callbacks: */
	SelectionChangedCallbackData cbData(this,SelectionChangedCallbackData::NUMITEMS_CHANGED,-1,false);
	selectionChangedCallbacks.call(&cbData);
	}
	
	/* Update the selected item if it is affected: */
	bool newSelection=false;
	if(lastSelectedItem==index)
		{
		if(selectionMode==ALWAYS_ONE&&!items.empty())
			{
			/* Select the next item in the list: */
			if(lastSelectedItem>int(items.size())-1)
				lastSelectedItem=int(items.size())-1;
			items[lastSelectedItem].selected=true;
			newSelection=true;
			
			/* Call the selection change callbacks: */
			SelectionChangedCallbackData cbData(this,SelectionChangedCallbackData::ITEM_SELECTED,lastSelectedItem,false);
			selectionChangedCallbacks.call(&cbData);
			}
		else
			{
			/* Select the invalid item: */
			lastSelectedItem=-1;
			}
		
		/* Call the value changed callbacks: */
		ValueChangedCallbackData cbData(this,index,lastSelectedItem,false);
		valueChangedCallbacks.call(&cbData);
		}
	else if(lastSelectedItem>index)
		{
		/* Adjust the selected item's index: */
		--lastSelectedItem;
		
		/* Call the value changed callbacks: */
		ValueChangedCallbackData cbData(this,lastSelectedItem+1,lastSelectedItem,false);
		valueChangedCallbacks.call(&cbData);
		}
	
	/* Keep track of changes to the page state: */
	int pageChangedReasonMask=PageChangedCallbackData::NUMITEMS_CHANGED;
	
	if(index<position)
		{
		/* Adjust the position so that the list of visible items does not change: */
		--position;
		pageChangedReasonMask|=PageChangedCallbackData::POSITION_CHANGED;
		}
	else if(index<position+pageSize)
		{
		/* Remove the removed item from the page: */
		GLLabel* removed=pageSlots[index-position].label;
		for(int i=index-position+1;i<pageSize;++i)
			pageSlots[i-1].label=pageSlots[i].label;
		
		if(position+pageSize-1<int(items.size()))
			{
			/* Bring the item after the page into the page: */
			Item& it=items[position+pageSize-1];
			removed->setString(it.item,it.item+it.itemLength);
			setPageSlotColor(*removed,it.selected);
			pageSlots[pageSize-1].label=removed;
			}
		else if(position>0)
			{
			/* Adjust the position so that the page remains full: */
			--position;
			pageChangedReasonMask|=PageChangedCallbackData::POSITION_CHANGED;
			for(int i=pageSize-1;i>0;--i)
				pageSlots[i].label=pageSlots[i-1].label;
			
			/* Bring the item before the page into the page: */
			Item& it=items[position];
			removed->setString(it.item,it.item+it.itemLength);
			setPageSlotColor(*removed,it.selected);
			pageSlots[0].label=removed;
			}
		else
			pageSlots[pageSize-1].label=removed;
		
		/* Update the page: */
		pageChangedReasonMask|=updatePage();
		}
	
	/* Update the page slot of the new selected item if necessary: */
	if(newSelection&&lastSelectedItem>=position&&lastSelectedItem-position<pageSize)
		setPageSlotColor(*pageSlots[lastSelectedItem-position].label,true);
	
	{
	/* Call the page change callbacks: */
	PageChangedCallbackData cbData(this,pageChangedReasonMask,position,int(items.size()),pageSize,horizontalOffset,maxVisibleItemWidth,itemsBox.size[0]);
	pageChangedCallbacks.call(&cbData);
	}
	
	/* Resize the listbox if enabled and necessary: */
	if(autoResize&&itemsBox.size[0]<maxVisibleItemWidth)
		{
		if(isManaged)
			parent->requestResize(this,calcNaturalSize());
		else
			resize(Box(Vector(0.0f,0.0f,0.0f),calcNaturalSize()));
		}
	
	/* Invalidate the visual representation: */
	update();
	}

void ListBox::clear(void)
	{
	/* Do nothing if the list is already empty: */
	if(items.empty())
		return;
	
	/* Clear the list: */
	for(std::vector<Item>::iterator iIt=items.begin();iIt!=items.end();++iIt)
		delete[] iIt->item;
	items.clear();
	
	{
	/* Call the list changed callbacks: */
	ListChangedCallbackData cbData(this,ListChangedCallbackData::LIST_CLEARED,-1);
	listChangedCallbacks.call(&cbData);
	}
	
	{
	/* Call the selection change callbacks: */
	SelectionChangedCallbackData cbData(this,SelectionChangedCallbackData::NUMITEMS_CHANGED,-1,false);
	selectionChangedCallbacks.call(&cbData);
	}
	
	if(lastSelectedItem>=0)
		{
		/* Select the invalid item: */
		int oldLastSelectedItem=lastSelectedItem;
		lastSelectedItem=-1;
		
		/* Call the value changed callbacks: */
		ValueChangedCallbackData cbData(this,oldLastSelectedItem,lastSelectedItem,false);
		valueChangedCallbacks.call(&cbData);
		}
	
	/* Keep track of changes to the page state: */
	int pageChangedReasonMask=PageChangedCallbackData::NUMITEMS_CHANGED;
	if(position!=0)
		{
		position=0;
		pageChangedReasonMask|=PageChangedCallbackData::POSITION_CHANGED;
		}
	if(maxVisibleItemWidth!=0.0f)
		{
		maxVisibleItemWidth=0.0f;
		pageChangedReasonMask|=PageChangedCallbackData::MAXITEMWIDTH_CHANGED;
		}
	if(horizontalOffset!=0.0f)
		{
		horizontalOffset=0.0f;
		pageChangedReasonMask|=PageChangedCallbackData::HORIZONTALOFFSET_CHANGED;
		}
	
	{
	/* Call the page change callbacks: */
	PageChangedCallbackData cbData(this,pageChangedReasonMask,position,int(items.size()),pageSize,horizontalOffset,maxVisibleItemWidth,itemsBox.size[0]);
	pageChangedCallbacks.call(&cbData);
	}
	
	/* Resize the listbox if enabled and necessary: */
	if(autoResize&&itemsBox.size[0]<maxVisibleItemWidth)
		{
		if(isManaged)
			parent->requestResize(this,calcNaturalSize());
		else
			resize(Box(Vector(0.0f,0.0f,0.0f),calcNaturalSize()));
		}
	
	/* Invalidate the visual representation: */
	update();
	}

void ListBox::setPosition(int newPosition)
	{
	/* Limit the new position to the valid range and bail out if nothing changed: */
	newPosition=Math::max(Math::min(newPosition,int(items.size())-pageSize),0);
	if(position==newPosition)
		return;
	
	/* Set the list position: */
	int pageChangeReasonMask=setPagePosition(newPosition);
	
	{
	PageChangedCallbackData cbData(this,pageChangeReasonMask,position,int(items.size()),pageSize,horizontalOffset,maxVisibleItemWidth,itemsBox.size[0]);
	pageChangedCallbacks.call(&cbData);
	}
	
	/* Invalidate the visual representation: */
	update();
	}

void ListBox::setHorizontalOffset(GLfloat newHorizontalOffset)
	{
	/* Limit the new horizontal offset to the valid range and bail out if nothing changed: */
	newHorizontalOffset=Math::max(Math::min(newHorizontalOffset,maxVisibleItemWidth-itemsBox.size[0]),0.0f);
	if(horizontalOffset==newHorizontalOffset)
		return;
	
	/* Update the visible page: */
	horizontalOffset=newHorizontalOffset;
	positionPageSlots();
	
	/* Call the page change callbacks: */
	PageChangedCallbackData cbData(this,PageChangedCallbackData::HORIZONTALOFFSET_CHANGED,position,int(items.size()),pageSize,horizontalOffset,maxVisibleItemWidth,itemsBox.size[0]);
	pageChangedCallbacks.call(&cbData);
	
	/* Invalidate the visual representation: */
	update();
	}

int ListBox::getNumSelectedItems(void) const
	{
	int result=0;
	
	/* Increment the counter for each selected item in the list: */
	for(size_t i=0;i<items.size();++i)
		if(items[i].selected)
			++result;
	
	return result;
	}

std::vector<int> ListBox::getSelectedItems(void) const
	{
	std::vector<int> result;
	
	/* Store the indices of all selected items in the list: */
	for(size_t i=0;i<items.size();++i)
		if(items[i].selected)
			result.push_back(int(i));
	
	return result;
	}

void ListBox::clearSelection(void)
	{
	if(selectionMode==MULTIPLE)
		{
		/* Deselect all selected items, and check if any items were actually selected: */
		bool hadSelectedItems=false;
		for(std::vector<Item>::iterator iIt=items.begin();iIt!=items.end();++iIt)
			{
			hadSelectedItems=hadSelectedItems||iIt->selected;
			iIt->selected=false;
			}
		if(hadSelectedItems)
			{
			/* Call the selection changed callbacks: */
			SelectionChangedCallbackData cbData(this,SelectionChangedCallbackData::SELECTION_CLEARED,-1,false);
			selectionChangedCallbacks.call(&cbData);
			}
		}
	else if(selectionMode==ATMOST_ONE&&lastSelectedItem>=0)
		{
		/* Deselect the last selected item: */
		items[lastSelectedItem].selected=false;
		
		/* Call the selection changed callbacks: */
		SelectionChangedCallbackData cbData(this,SelectionChangedCallbackData::SELECTION_CLEARED,-1,false);
		selectionChangedCallbacks.call(&cbData);
		}
	
	if(lastSelectedItem>=0)
		{
		/* Select the invalid item: */
		int oldLastSelectedItem=lastSelectedItem;
		lastSelectedItem=-1;
		
		/* Call the value changed callbacks: */
		ValueChangedCallbackData cbData(this,oldLastSelectedItem,lastSelectedItem,false);
		valueChangedCallbacks.call(&cbData);
		}
	
	/* Update the selection states of all visible items: */
	int pageEnd=Math::min(pageSize,int(items.size())-position);
	for(int i=0;i<pageEnd;++i)
		setPageSlotColor(*pageSlots[i].label,items[position+i].selected);
	
	/* Invalidate the visual representation: */
	update();
	}

}
