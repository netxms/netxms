/**
 * 
 */
package org.netxms.ui.eclipse.actionmanager.views.helpers;

import org.eclipse.jface.viewers.ILabelProviderListener;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.ServerAction;
import org.netxms.ui.eclipse.actionmanager.Activator;
import org.netxms.ui.eclipse.actionmanager.views.ActionManager;

/**
 * Label provider for action list 
 *
 */
public class ActionLabelProvider implements ITableLabelProvider
{
	private static final String[] ACTION_TYPE = { "Execute", "Remote Execute", "Send E-Mail", "Send SMS", "Forward Event" };
	
	private Image[] actionImage = new Image[5];
	
	/**
	 * The constructor
	 */
	public ActionLabelProvider()
	{
		actionImage[0] = Activator.getImageDescriptor("icons/exec_local.png").createImage(true);
		actionImage[1] = Activator.getImageDescriptor("icons/exec_remote.png").createImage(true);
		actionImage[2] = Activator.getImageDescriptor("icons/email.png").createImage(true);
		actionImage[3] = Activator.getImageDescriptor("icons/sms.png").createImage(true);
		actionImage[4] = Activator.getImageDescriptor("icons/fwd_event.png").createImage(true);
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		if (columnIndex == ActionManager.COLUMN_NAME)
		{
			try
			{
				return actionImage[((ServerAction)element).getType()];
			}
			catch(IndexOutOfBoundsException e)
			{
			}
		}
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
					return "Unknown";
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
		for(int i = 0; i < actionImage.length; i++)
			if (actionImage[i] != null)
				actionImage[i].dispose();
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
