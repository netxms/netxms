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

import java.util.Set;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.objectbrowser.Messages;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.AbstractSelector;

/**
 * Object selector
 */
public class ObjectSelector extends AbstractSelector
{
	private static final long serialVersionUID = 1L;

	private long objectId = 0;
	private Class<? extends GenericObject> objectClass = Node.class;
	private Set<Integer> classFilter = null;
	private String emptySelectionName = Messages.ObjectSelector_None;
	
	/**
	 * @param parent
	 * @param style
	 */
	public ObjectSelector(Composite parent, int style)
	{
		super(parent, style, 0);
		setText(emptySelectionName);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractSelector#selectionButtonHandler()
	 */
	@Override
	protected void selectionButtonHandler()
	{
		ObjectSelectionDialog dlg = new ObjectSelectionDialog(getShell(), null, classFilter);
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
				setText(emptySelectionName);
			}
			fireModifyListeners();
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
	 * @return the object name
	 */
	public String getObjectName()
	{
		return getText();
	}

	/**
	 * @param objectId the objectId to set
	 */
	public void setObjectId(long objectId)
	{
		this.objectId = objectId;
		if (objectId == 0)
		{
			setText(emptySelectionName); //$NON-NLS-1$
		}
		else
		{
			final GenericObject object = ((NXCSession)ConsoleSharedData.getSession()).findObjectById(objectId);
			setText((object != null) ? object.getObjectName() : ("<" + Long.toString(objectId) + ">")); //$NON-NLS-1$ //$NON-NLS-2$
		}
	}

	/**
	 * @return the objectClass
	 */
	public Class<? extends GenericObject> getObjectClass()
	{
		return objectClass;
	}

	/**
	 * @param objectClass the objectClass to set
	 */
	public void setObjectClass(Class<? extends GenericObject> objectClass)
	{
		this.objectClass = objectClass;
	}

	/**
	 * @return the emptySelectionName
	 */
	public String getEmptySelectionName()
	{
		return emptySelectionName;
	}

	/**
	 * @param emptySelectionName the emptySelectionName to set
	 */
	public void setEmptySelectionName(String emptySelectionName)
	{
		this.emptySelectionName = emptySelectionName;
	}

	/**
	 * @return the classFilter
	 */
	public Set<Integer> getClassFilter()
	{
		return classFilter;
	}

	/**
	 * @param classFilter the classFilter to set
	 */
	public void setClassFilter(Set<Integer> classFilter)
	{
		this.classFilter = classFilter;
	}
}
