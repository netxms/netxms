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
package org.netxms.ui.eclipse.actionmanager.views.helpers;

import org.eclipse.jface.viewers.ILabelProvider;
import org.eclipse.jface.viewers.ILabelProviderListener;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.swt.graphics.Image;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.ServerAction;
import org.netxms.ui.eclipse.actionmanager.Messages;
import org.netxms.ui.eclipse.actionmanager.views.ActionManager;

/**
 * Label provider for action list 
 *
 */
public class ActionLabelProvider implements ITableLabelProvider
{
	private static final String[] ACTION_TYPE = { Messages.ActionLabelProvider_ActionTypeExecute, Messages.ActionLabelProvider_ActionTypeRemoteExec, Messages.ActionLabelProvider_ActionTypeMail, Messages.ActionLabelProvider_ActionTypeSMS, Messages.ActionLabelProvider_ActionTypeForward };
	
	private ILabelProvider workbenchLabelProvider;
	
	/**
	 * The constructor
	 */
	public ActionLabelProvider()
	{
		workbenchLabelProvider = WorkbenchLabelProvider.getDecoratingWorkbenchLabelProvider();
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		if (columnIndex == ActionManager.COLUMN_NAME)
			return workbenchLabelProvider.getImage(element);
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
	 */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		ServerAction action = (ServerAction)element;
		switch(columnIndex)
		{
			case ActionManager.COLUMN_NAME:
				return action.getName();
			case ActionManager.COLUMN_TYPE:
				try
				{
					return ACTION_TYPE[action.getType()];
				}
				catch(IndexOutOfBoundsException e)
				{
					return Messages.ActionLabelProvider_Unknown;
				}
			case ActionManager.COLUMN_RCPT:
				return action.getRecipientAddress();
			case ActionManager.COLUMN_SUBJ:
				return action.getEmailSubject();
			case ActionManager.COLUMN_DATA:
				return action.getData();
		}
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#addListener(org.eclipse.jface.viewers.ILabelProviderListener)
	 */
	@Override
	public void addListener(ILabelProviderListener listener)
	{
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#dispose()
	 */
	@Override
	public void dispose()
	{
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#isLabelProperty(java.lang.Object, java.lang.String)
	 */
	@Override
	public boolean isLabelProperty(Object element, String property)
	{
		return false;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#removeListener(org.eclipse.jface.viewers.ILabelProviderListener)
	 */
	@Override
	public void removeListener(ILabelProviderListener listener)
	{
	}
}
