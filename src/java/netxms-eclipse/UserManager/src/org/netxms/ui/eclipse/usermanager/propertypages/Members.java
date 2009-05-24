package org.netxms.ui.eclipse.usermanager.propertypages;

import java.util.ArrayList;
import java.util.List;

import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.dialogs.PropertyPage;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.NXCSession;
import org.netxms.client.NXCUser;
import org.netxms.client.NXCUserDBObject;
import org.netxms.client.NXCUserGroup;
import org.netxms.ui.eclipse.shared.NXMCSharedData;

public class Members extends PropertyPage
{

	private NXCSession session;
	private NXCUserGroup object;

	public Members()
	{
		session = NXMCSharedData.getInstance().getSession();
	}

	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		object = (NXCUserGroup) getElement().getAdapter(NXCUserGroup.class);

		dialogArea.setLayout(new FillLayout());

		TableViewer objectList = new TableViewer(dialogArea, SWT.SINGLE | SWT.FULL_SELECTION);
		objectList.setContentProvider(new ArrayContentProvider());
		objectList.setLabelProvider(new WorkbenchLabelProvider());
		//objectList.setComparator(new ObjectTreeComparator());

		//final AdaptableList adaptableList = new AdaptableList(asList);
		final List<NXCUserDBObject> users = new ArrayList<NXCUserDBObject>(0);
		final long[] members = object.getMembers();
		for(long userId : members)
		{
			System.out.println(">>> " + userId);
			final NXCUserDBObject user = session.findUserDBObjectById(userId);
			if (user != null)
			{
				users.add(user);
			}
		}
		objectList.setInput(users);

		return dialogArea;
	}
}
