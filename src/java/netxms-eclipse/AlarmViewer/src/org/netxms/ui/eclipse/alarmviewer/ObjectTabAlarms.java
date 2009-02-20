/**
 * Alarm tab for object browser
 */
package org.netxms.ui.eclipse.alarmviewer;

import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CTabFolder;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCObject;
import org.netxms.ui.eclipse.console.extensionproviders.ObjectTab;

/**
 * @author victor
 *
 */
public class ObjectTabAlarms extends ObjectTab
{
	private AlarmView control;
	
	
	/**
	 * @param tabFolder
	 */
	public ObjectTabAlarms(CTabFolder tabFolder)
	{
		super(tabFolder);
		control = new AlarmView(null, tabFolder, SWT.NONE);
	}

	/* (non-Javadoc)
	 * @see org.netxms.nxmc.core.ObjectTab#getTabName()
	 */
	@Override
	protected String getTabName()
	{
		return "Alarms";
	}

	/* (non-Javadoc)
	 * @see org.netxms.nxmc.core.ObjectTab#isValidForObject(org.netxms.client.NXCObject)
	 */
	@Override
	public boolean isValidForObject(NXCObject object)
	{
		return (object.getObjectClass() == NXCObject.OBJECT_NETWORK) ||
		       (object.getObjectClass() == NXCObject.OBJECT_SUBNET) ||
		       (object.getObjectClass() == NXCObject.OBJECT_SERVICEROOT) ||
		       (object.getObjectClass() == NXCObject.OBJECT_CONTAINER) ||
		       (object.getObjectClass() == NXCObject.OBJECT_CLUSTER) ||
		       (object.getObjectClass() == NXCObject.OBJECT_NODE);
	}

	/* (non-Javadoc)
	 * @see org.netxms.nxmc.core.ObjectTab#getControl()
	 */
	@Override
	protected Composite getControl()
	{
		return control;
	}

	/* (non-Javadoc)
	 * @see org.netxms.nxmc.core.extensionproviders.ObjectTab#onObjectChange()
	 */
	@Override
	protected void onObjectChange()
	{
		control.setRootObject(currentObject.getObjectId());
	}
}
