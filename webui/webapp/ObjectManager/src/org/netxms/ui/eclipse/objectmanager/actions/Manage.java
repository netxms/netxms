package org.netxms.ui.eclipse.objectmanager.actions;

import org.netxms.client.NXCSession;
import org.netxms.client.objects.GenericObject;

public class Manage extends MultipleObjectAction
{
	@Override
	protected String errorPrefix()
	{
		return "Cannot change management status for object";
	}

	@Override
	protected String jobDescription()
	{
		return "Changing management status for object";
	}

	@Override
	protected void runObjectAction(final NXCSession session, final GenericObject object) throws Exception
	{
		session.setObjectManaged(object.getObjectId(), true);
	}
}
