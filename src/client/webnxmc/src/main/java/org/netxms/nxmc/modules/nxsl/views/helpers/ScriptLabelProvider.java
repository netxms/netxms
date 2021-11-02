/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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
package org.netxms.nxmc.modules.nxsl.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.Script;
import org.netxms.nxmc.modules.nxsl.views.ScriptLibraryView;
import org.netxms.nxmc.resources.ResourceManager;

/**
 * Label provider for script objects
 */
public class ScriptLabelProvider extends LabelProvider implements ITableLabelProvider
{
	private Image iconScript;
	
   /**
    * The constructor.
    */
	public ScriptLabelProvider()
	{
      iconScript = ResourceManager.getImageDescriptor("icons/script.png").createImage();
	}

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		if (columnIndex == 0)
			return iconScript;
		return null;
	}

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		switch(columnIndex)
		{
         case ScriptLibraryView.COLUMN_ID:
				return Long.toString(((Script)element).getId());
         case ScriptLibraryView.COLUMN_NAME:
				return ((Script)element).getName();
		}
		return null;
	}

   /**
    * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
    */
	@Override
	public void dispose()
	{
		iconScript.dispose();
      super.dispose();
	}
}
