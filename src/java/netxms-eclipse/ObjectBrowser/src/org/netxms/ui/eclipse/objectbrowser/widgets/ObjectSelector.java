/**
 * 
 */
package org.netxms.ui.eclipse.objectbrowser.widgets;

import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCObject;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.shared.NXMCSharedData;
import org.netxms.ui.eclipse.widgets.AbstractSelector;

/**
 * @author Victor
 *
 */
public class ObjectSelector extends AbstractSelector
{
	private long objectId = 0;
	private int objectClass = NXCObject.OBJECT_NODE;
	
	/**
	 * @param parent
	 * @param style
	 */
	public ObjectSelector(Composite parent, int style)
	{
		super(parent, style);
		setText("<none>");
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.AbstractSelector#selectionButtonHandler()
	 */
	@Override
	protected void selectionButtonHandler()
	{
		ObjectSelectionDialog dlg = new ObjectSelectionDialog(getShell(), null);
		if (dlg.open() == Window.OK)
		{
			NXCObject[] objects = dlg.getAllCheckedObjects(objectClass);
			if (objects.length > 0)
			{
				objectId = objects[0].getObjectId();
				setText(objects[0].getObjectName());
			}
			else
			{
				objectId = 0;
				setText("<none>");
			}
		}
	}

	/**
	 * @return the objectId
	 */
	public long getObjectId()
	{
		return objectId;
	}

	/**
	 * @param objectId the objectId to set
	 */
	public void setObjectId(long objectId)
	{
		this.objectId = objectId;
		if (objectId == 0)
		{
			setText("<none>");
		}
		else
		{
			final NXCObject object = NXMCSharedData.getInstance().getSession().findObjectById(objectId);
			setText((object != null) ? object.getObjectName() : "<unknown>");
		}
	}

	/**
	 * @return the objectClass
	 */
	public int getObjectClass()
	{
		return objectClass;
	}

	/**
	 * @param objectClass the objectClass to set
	 */
	public void setObjectClass(int objectClass)
	{
		this.objectClass = objectClass;
	}
}
