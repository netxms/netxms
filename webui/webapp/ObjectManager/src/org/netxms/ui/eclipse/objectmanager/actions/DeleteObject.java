package org.netxms.ui.eclipse.objectmanager.actions;

import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.NXCSession;

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
		boolean confirmed = MessageDialog.openConfirm(getWindow().getShell(), "Confirm Delete", question);
		return confirmed;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectmanager.actions.MultipleObjectAction#errorPrefix()
	 */
	@Override
	protected String errorPrefix()
	{
		return "Cannot delete object";
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectmanager.actions.MultipleObjectAction#jobDescription()
	 */
	@Override
	protected String jobDescription()
	{
		return "Delete object";
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectmanager.actions.MultipleObjectAction#runObjectAction(org.netxms.client.NXCSession, org.netxms.client.NXCObject)
	 */
	@Override
	protected void runObjectAction(NXCSession session, AbstractObject object) throws Exception
	{
		session.deleteObject(object.getObjectId());
	}
}
