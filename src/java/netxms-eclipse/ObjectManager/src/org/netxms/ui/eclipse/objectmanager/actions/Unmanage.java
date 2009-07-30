package org.netxms.ui.eclipse.objectmanager.actions;

import org.netxms.client.NXCObject;
import org.netxms.client.NXCSession;

public class Unmanage extends MultipleObjectAction
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
	protected void runObjectAction(NXCSession session, NXCObject object) throws Exception
	{
		session.setObjectManaged(object.getObjectId(), false);
	}
}
