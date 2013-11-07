/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.ui.eclipse.objecttools.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.ui.eclipse.objecttools.Activator;
import org.netxms.ui.eclipse.objecttools.views.ObjectToolsEditor;

/**
 * Label provider for object tools
 *
 */
public class ObjectToolsLabelProvider extends LabelProvider implements ITableLabelProvider
{
	private static final String[] toolTypes = { 
		"Internal", 
		"Action", 
		"SNMP Table", 
		"Agent Table", 
		"URL",
		"Local Command",
		"Server Command",
		"Download File"
	};
	
	private Image[] toolTypeImages = new Image[toolTypes.length];
	
	/**
	 * The constructor
	 */
	public ObjectToolsLabelProvider()
	{
		toolTypeImages[0] = Activator.getImageDescriptor("icons/internal_tool.gif").createImage();
		toolTypeImages[1] = Activator.getImageDescriptor("icons/agent_action.gif").createImage();
		toolTypeImages[2] = Activator.getImageDescriptor("icons/table.gif").createImage();
		toolTypeImages[3] = Activator.getImageDescriptor("icons/table.gif").createImage();
		toolTypeImages[4] = Activator.getImageDescriptor("icons/url.gif").createImage();
		toolTypeImages[5] = Activator.getImageDescriptor("icons/console.png").createImage();
		toolTypeImages[6] = Activator.getImageDescriptor("icons/console.png").createImage();
		toolTypeImages[7] = Activator.getImageDescriptor("icons/file_download.png").createImage();
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		if (columnIndex != 0)
			return null;

		try
		{
			return toolTypeImages[((ObjectTool)element).getType()];
		}
		catch(ArrayIndexOutOfBoundsException e)
		{
			return null;
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
	 */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		ObjectTool tool = (ObjectTool)element;
		switch(columnIndex)
		{
			case ObjectToolsEditor.COLUMN_ID:
				return Long.toString(tool.getId());
			case ObjectToolsEditor.COLUMN_NAME:
				return tool.getName();
			case ObjectToolsEditor.COLUMN_TYPE:
				try
				{
					return toolTypes[tool.getType()];
				}
				catch(ArrayIndexOutOfBoundsException e)
				{
					return "?unknown?";
				}
			case ObjectToolsEditor.COLUMN_DESCRIPTION:
				return tool.getDescription();
		}
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
	 */
	@Override
	public void dispose()
	{
		for(Image i : toolTypeImages)
			i.dispose();
		super.dispose();
	}
	
	/**
	 * Get display name for object tool's type
	 * @param tool object tool
	 * @return tool type's name
	 */
	public static String getToolTypeName(ObjectTool tool)
	{
		try
		{
			return toolTypes[tool.getType()];
		}
		catch(ArrayIndexOutOfBoundsException e)
		{
			return "?unknown?";
		}
	}
	
	/**
	 * Get names of all tool types.
	 * @return
	 */
	public static String[] getAllToolTypes()
	{
		return toolTypes;
	}
}
