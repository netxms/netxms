/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectbrowser.widgets;

import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.objectbrowser.Messages;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.AbstractSelector;

/**
 * @author Victor
 *
 */
public class ObjectSelector extends AbstractSelector
{
	private long objectId = 0;
	private int objectClass = GenericObject.OBJECT_NODE;
	
	/**
	 * @param parent
	 * @param style
	 */
	public ObjectSelector(Composite parent, int style)
	{
		super(parent, style);
		setText(Messages.getString("ObjectSelector.none")); //$NON-NLS-1$
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractSelector#selectionButtonHandler()
	 */
	@Override
	protected void selectionButtonHandler()
	{
		ObjectSelectionDialog dlg = new ObjectSelectionDialog(getShell(), null, null);
		dlg.enableMultiSelection(false);
		if (dlg.open() == Window.OK)
		{
			GenericObject[] objects = dlg.getSelectedObjects(objectClass);
			if (objects.length > 0)
			{
				objectId = objects[0].getObjectId();
				setText(objects[0].getObjectName());
			}
			else
			{
				objectId = 0;
				setText(Messages.getString("ObjectSelector.none")); //$NON-NLS-1$
			}
		}
	}

	/**
	 * @return the objectId
	 */
	public long getObjectId()
	{
		return objectId;
	}

	/**
	 * @param objectId the objectId to set
	 */
	public void setObjectId(long objectId)
	{
		this.objectId = objectId;
		if (objectId == 0)
		{
			setText(Messages.getString("ObjectSelector.none")); //$NON-NLS-1$
		}
		else
		{
			final GenericObject object = ConsoleSharedData.getInstance().getSession().findObjectById(objectId);
			setText((object != null) ? object.getObjectName() : Messages.getString("ObjectSelector.unknown")); //$NON-NLS-1$
		}
	}

	/**
	 * @return the objectClass
	 */
	public int getObjectClass()
	{
		return objectClass;
	}

	/**
	 * @param objectClass the objectClass to set
	 */
	public void setObjectClass(int objectClass)
	{
		this.objectClass = objectClass;
	}
}
