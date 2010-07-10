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
package org.netxms.ui.eclipse.objectbrowser.dialogs;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.objectbrowser.Activator;
import org.netxms.ui.eclipse.shared.NXMCSharedData;
import org.netxms.ui.eclipse.tools.SortableTableViewer;

/**
 * Dialog used to select child object(s) of given object
 *
 */
public class ChildObjectListDialog extends Dialog
{
	private long parentObject;
	private Set<Integer> classFilter;
	private TableViewer objectList;
	private List<GenericObject> selectedObjects;
	
	/**
	 * @param parentShell
	 */
	public ChildObjectListDialog(Shell parentShell, long parentObject, Set<Integer> classFilter)
	{
		super(parentShell);
		
		this.parentObject = parentObject;
		this.classFilter = classFilter;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(Messages.getString("ChildObjectListDialog.title")); //$NON-NLS-1$
		IDialogSettings settings = Activator.getDefault().getDialogSettings();
		try
		{
			newShell.setSize(settings.getInt("ChildObjectListDialog.cx"), settings.getInt("ChildObjectListDialog.cy")); //$NON-NLS-1$ //$NON-NLS-2$
		}
		catch(NumberFormatException e)
		{
			newShell.setSize(300, 350);
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		dialogArea.setLayout(new FillLayout());
		
		objectList = new SortableTableViewer(dialogArea, new String[] { "Name" }, new int[] { 280 }, 0, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
		objectList.setContentProvider(new ArrayContentProvider());
		objectList.setLabelProvider(new WorkbenchLabelProvider());
		objectList.setComparator(new ViewerComparator());
		
		GenericObject object = NXMCSharedData.getInstance().getSession().findObjectById(parentObject);
		if (object != null)
			objectList.setInput(object.getChildsAsArray());
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#cancelPressed()
	 */
	@Override
	protected void cancelPressed()
	{
		saveSettings();
		super.cancelPressed();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@SuppressWarnings("unchecked")
	@Override
	protected void okPressed()
	{
		IStructuredSelection selection = (IStructuredSelection)objectList.getSelection();
		selectedObjects = new ArrayList<GenericObject>(selection.size());
		Iterator it = selection.iterator();
		while(it.hasNext())
			selectedObjects.add((GenericObject)it.next());
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

		settings.put("ChildObjectListDialog.cx.cx", size.x); //$NON-NLS-1$
		settings.put("ChildObjectListDialog.cx.cy", size.y); //$NON-NLS-1$
	}

	/**
	 * @return the selectedObjects
	 */
	public List<GenericObject> getSelectedObjects()
	{
		return selectedObjects;
	}
}
