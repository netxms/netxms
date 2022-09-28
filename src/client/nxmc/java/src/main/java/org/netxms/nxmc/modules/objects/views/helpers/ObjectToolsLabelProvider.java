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
package org.netxms.nxmc.modules.objects.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.ObjectToolsEditor;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for object tools
 *
 */
public class ObjectToolsLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private static final I18n i18n = LocalizationHelper.getI18n(ObjectToolsLabelProvider.class);
	private static final String[] TOOL_TYPES = { 
		i18n.tr("Internal"),
		i18n.tr("Agent Command"),
		i18n.tr("SNMP Table"),
		i18n.tr("Agent List"),
		i18n.tr("URL"),
		i18n.tr("Local Command"),
		i18n.tr("Server Command"),
		i18n.tr("Download File"),
      i18n.tr("Server Script"),
      i18n.tr("Agent Table"),
      i18n.tr("SSH Command")
	};
	
	private Image[] toolTypeImages = new Image[TOOL_TYPES.length + 1];
	
	/**
	 * The constructor
	 */
	public ObjectToolsLabelProvider()
	{
		toolTypeImages[0] = ResourceManager.getImageDescriptor("icons/object-tools/internal_tool.gif").createImage(); //$NON-NLS-1$
		toolTypeImages[1] = ResourceManager.getImageDescriptor("icons/object-tools/agent_action.gif").createImage(); //$NON-NLS-1$
		toolTypeImages[2] = ResourceManager.getImageDescriptor("icons/object-tools/table.gif").createImage(); //$NON-NLS-1$
		toolTypeImages[3] = ResourceManager.getImageDescriptor("icons/object-tools/table.gif").createImage(); //$NON-NLS-1$
		toolTypeImages[4] = ResourceManager.getImageDescriptor("icons/object-tools/url.gif").createImage(); //$NON-NLS-1$
		toolTypeImages[5] = ResourceManager.getImageDescriptor("icons/object-tools/console.png").createImage(); //$NON-NLS-1$
		toolTypeImages[6] = ResourceManager.getImageDescriptor("icons/object-tools/console.png").createImage(); //$NON-NLS-1$
		toolTypeImages[7] = ResourceManager.getImageDescriptor("icons/object-tools/file_download.png").createImage(); //$NON-NLS-1$
      toolTypeImages[8] = ResourceManager.getImageDescriptor("icons/object-tools/script.png").createImage(); //$NON-NLS-1$
      toolTypeImages[9] = ResourceManager.getImageDescriptor("icons/object-tools/table.gif").createImage(); //$NON-NLS-1$
      toolTypeImages[10] = ResourceManager.getImageDescriptor("icons/object-tools/console.png").createImage(); //$NON-NLS-1$
      toolTypeImages[11] = ResourceManager.getImageDescriptor("icons/object-tools/stop.png").createImage(); //$NON-NLS-1$
	}

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		if (columnIndex != 0)
			return null;

		try
		{
		   if ((((ObjectTool)element).getFlags() & ObjectTool.DISABLED) == 0)
		   {
		      return toolTypeImages[((ObjectTool)element).getToolType()];
		   }
		   else
		   {
		      return toolTypeImages[toolTypeImages.length - 1];
		   }
		}
		catch(ArrayIndexOutOfBoundsException e)
		{
			return null;
		}
	}

   /**
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
					return TOOL_TYPES[tool.getToolType()];
				}
				catch(ArrayIndexOutOfBoundsException e)
				{
					return "<unknown>"; //$NON-NLS-1$
				}
			case ObjectToolsEditor.COLUMN_DESCRIPTION:
				return tool.getDescription();
		}
		return null;
	}

   /**
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
			return TOOL_TYPES[tool.getToolType()];
		}
		catch(ArrayIndexOutOfBoundsException e)
		{
			return "<unknown>"; //$NON-NLS-1$
		}
	}
	
	/**
	 * Get names of all tool types.
	 * @return
	 */
	public static String[] getAllToolTypes()
	{
		return TOOL_TYPES;
	}
}
