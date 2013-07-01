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

import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;

/**
 * Set selected object(s) to managed state
 */
public class Manage extends MultipleObjectAction
{
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectmanager.actions.MultipleObjectAction#runObjectAction(org.netxms.client.NXCSession, org.netxms.client.objects.AbstractObject)
	 */
	@Override
	protected void runObjectAction(final NXCSession session, final AbstractObject object) throws Exception
	{
		session.setObjectManaged(object.getObjectId(), true);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectmanager.actions.MultipleObjectAction#formatJobDescription(org.netxms.client.objects.AbstractObject)
	 */
	@Override
	protected String formatJobDescription(AbstractObject object)
	{
		return String.format("Changing management status for object %s [%d]", object.getObjectName(), object.getObjectId());
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectmanager.actions.MultipleObjectAction#formatErrorMessage(org.netxms.client.objects.AbstractObject, org.eclipse.swt.widgets.Display)
	 */
	@Override
	protected String formatErrorMessage(AbstractObject object, Display display)
	{
		return String.format("Cannot change management status for object %s [%d]", object.getObjectName(), object.getObjectId());
	}
}
