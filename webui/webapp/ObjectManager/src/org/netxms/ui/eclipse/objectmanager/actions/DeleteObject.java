/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectmanager.actions;

import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Delete selected object(s)
 */
public class DeleteObject extends MultipleObjectAction
{
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectmanager.actions.MultipleObjectAction#confirm()
	 */
	protected boolean confirm()
	{
		IStructuredSelection selection = getSelection();
		String question;
		if (selection.size() == 1)
		{
			question = "Are you sure you want to delete '" + ((AbstractObject)selection.getFirstElement()).getObjectName() + "'?";
		}
		else
		{
			question = "Are you sure you want to delete selected objects?";
		}
		boolean confirmed = MessageDialogHelper.openConfirm(getWindow().getShell(), "Confirm Delete", question);
		return confirmed;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectmanager.actions.MultipleObjectAction#runObjectAction(org.netxms.client.NXCSession, org.netxms.client.NXCObject)
	 */
	@Override
	protected void runObjectAction(NXCSession session, AbstractObject object) throws Exception
	{
		session.deleteObject(object.getObjectId());
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectmanager.actions.MultipleObjectAction#formatJobDescription(org.netxms.client.objects.AbstractObject)
	 */
	@Override
	protected String formatJobDescription(AbstractObject object)
	{
		return String.format("Delete object %s [%d]", object.getObjectName(), object.getObjectId());
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectmanager.actions.MultipleObjectAction#formatErrorMessage(org.netxms.client.objects.AbstractObject, org.eclipse.swt.widgets.Display)
	 */
	@Override
	protected String formatErrorMessage(AbstractObject object, Display display)
	{
		return String.format("Cannot delete object %s [%d]", object.getObjectName(), object.getObjectId());
	}
}
