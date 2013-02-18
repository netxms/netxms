package org.netxms.ui.eclipse.objectmanager.actions;

import org.eclipse.jface.viewers.IStructuredSelection;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

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
			question = "Are you sure you want to delete '" + ((GenericObject)selection.getFirstElement()).getObjectName() + "'?";
		}
		else
		{
			question = "Are you sure you want to delete selected objects?";
		}
		boolean confirmed = MessageDialogHelper.openConfirm(getWindow().getShell(), "Confirm Delete", question);
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
	protected void runObjectAction(NXCSession session, GenericObject object) throws Exception
	{
		session.deleteObject(object.getObjectId());
	}
}
