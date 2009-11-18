/**
 * 
 */
package org.netxms.ui.eclipse.objectbrowser.dialogs;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.objectbrowser.Activator;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectTree;
import org.netxms.ui.eclipse.shared.NXMCSharedData;

/**
 * @author Victor
 * 
 */
public class ObjectSelectionDialog extends Dialog
{
	private ObjectTree objectTree;
	private long[] rootObjects;
	private long[] selectedObjects;
	private Set<Integer> classFilter;
	private boolean multiSelection;
	
	/**
	 * Create filter for node selection - it allows node objects and possible
	 * parents - subnets and containers.
	 * 
	 * @return Class filter for node selection
	 */
	public static Set<Integer> createNodeSelectionFilter()
	{
		HashSet<Integer> classFilter = new HashSet<Integer>(5);
		classFilter.add(GenericObject.OBJECT_NETWORK);
		classFilter.add(GenericObject.OBJECT_SUBNET);
		classFilter.add(GenericObject.OBJECT_SERVICEROOT);
		classFilter.add(GenericObject.OBJECT_CONTAINER);
		classFilter.add(GenericObject.OBJECT_NODE);
		return classFilter;
	}

	/**
	 * @param parentShell
	 */
	public ObjectSelectionDialog(Shell parentShell, long[] rootObjects, Set<Integer> classFilter)
	{
		super(parentShell);
		setShellStyle(getShellStyle() | SWT.RESIZE);
		this.rootObjects = rootObjects;
		this.classFilter = classFilter;
		multiSelection = true;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets
	 * .Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(Messages.getString("ObjectSelectionDialog.title")); //$NON-NLS-1$
		IDialogSettings settings = Activator.getDefault().getDialogSettings();
		try
		{
			newShell.setSize(settings.getInt("SelectObject.cx"), settings.getInt("SelectObject.cy")); //$NON-NLS-1$ //$NON-NLS-2$
		}
		catch(NumberFormatException e)
		{
			newShell.setSize(400, 350);
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets
	 * .Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		IDialogSettings settings = Activator.getDefault().getDialogSettings();
		Composite dialogArea = (Composite)super.createDialogArea(parent);

		dialogArea.setLayout(new FormLayout());

		objectTree = new ObjectTree(dialogArea, SWT.NONE, multiSelection ? ObjectTree.CHECKBOXES : 0, rootObjects, classFilter);
		
		String text = settings.get("SelectObject.Filter"); //$NON-NLS-1$
		if (text != null)
			objectTree.setFilter(text);

		FormData fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(0, 0);
		fd.right = new FormAttachment(100, 0);
		fd.bottom = new FormAttachment(100, 0);
		objectTree.setLayoutData(fd);

		return dialogArea;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.dialogs.Dialog#cancelPressed()
	 */
	@Override
	protected void cancelPressed()
	{
		saveSettings();
		super.cancelPressed();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		if (multiSelection)
		{
			Long[] objects = objectTree.getCheckedObjects();
			selectedObjects = new long[objects.length];
			for(int i = 0; i < objects.length; i++)
				selectedObjects[i] = objects[i].longValue();
		}
		else
		{
			long objectId = objectTree.getSelectedObject();
			if (objectId != 0)
			{
				selectedObjects = new long[1];
				selectedObjects[0] = objectId;
			}
			else
			{
				MessageDialog.openWarning(getShell(), "Warning", "Please select object and than press OK");
				return;
			}
		}
		
		saveSettings();
		super.okPressed();
	}

	/**
	 * Save dialog settings
	 */
	private void saveSettings()
	{
		Point size = getShell().getSize();
		IDialogSettings settings = Activator.getDefault().getDialogSettings();

		settings.put("SelectObject.cx", size.x); //$NON-NLS-1$
		settings.put("SelectObject.cy", size.y); //$NON-NLS-1$
		settings.put("SelectObject.Filter", objectTree.getFilter()); //$NON-NLS-1$
	}

	/**
	 * Retrieve selected objects
	 * 
	 * @return
	 */
	public GenericObject[] getCheckedObjects()
	{
		if (selectedObjects == null)
			return new GenericObject[0];

		return NXMCSharedData.getInstance().getSession().findMultipleObjects(selectedObjects);
	}

	/**
	 * Retrieve all selected objects (including childs)
	 * 
	 * @return
	 */
	public GenericObject[] getAllCheckedObjects()
	{
		return getAllCheckedObjects(-1);
	}

	/**
	 * Retrieve all selected objects by type
	 * 
	 * @return
	 */
	public GenericObject[] getAllCheckedObjects(int objectType)
	{
		if (selectedObjects == null)
			return new GenericObject[0];

		final NXCSession session = NXMCSharedData.getInstance().getSession();

		final List<GenericObject> resuls = new ArrayList<GenericObject>();
		final GenericObject[] objects = session.findMultipleObjects(selectedObjects);
		for(int i = 0; i < objects.length; i++)
		{
			resuls.addAll(extractAllObjects(session, objects[i]));
		}

		if (objectType != -1)
		{
			final Iterator<GenericObject> it = resuls.iterator();
			while (it.hasNext()) {
				final GenericObject next = it.next();
				if (next.getObjectClass() != objectType) {
					it.remove();
				}
			}
		}

		return resuls.toArray(new GenericObject[] {});
	}

	private List<GenericObject> extractAllObjects(NXCSession session, GenericObject object)
	{
		final List<GenericObject> ret = new ArrayList<GenericObject>(0);

		ret.add(object);

		final Iterator<Long> it = object.getChilds();
		while(it.hasNext())
		{
			final Long childId = it.next();
			final GenericObject child = session.findObjectById(childId);
			if (child != null)
			{
				ret.addAll(extractAllObjects(session, child));
			}
		}

		return ret;
	}

	/**
	 * @return true if multiple object selection is enabled
	 */
	public boolean isMultiSelectionEnabled()
	{
		return multiSelection;
	}

	/**
	 * Enable or disable selection of multiple objects. If multiselection is enabled,
	 * object tree will appear with check boxes, and objects can be selected by checking them.
	 * 
	 * @param enable true to enable multiselection, false to disable
	 */
	public void enableMultiSelection(boolean enable)
	{
		this.multiSelection = enable;
	}
}
