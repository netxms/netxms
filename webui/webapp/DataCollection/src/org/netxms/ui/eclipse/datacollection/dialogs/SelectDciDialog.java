/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.datacollection.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.SashForm;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.widgets.DciList;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectTree;

/**
 * Dialog for DCI selection
 */
public class SelectDciDialog extends Dialog
{
	private static final long serialVersionUID = 1L;

	private SashForm splitter;
	private ObjectTree objectTree;
	private DciList dciList;
	private DciValue selection;
	
	/**
	 * @param parentShell
	 */
	public SelectDciDialog(Shell parentShell)
	{
		super(parentShell);
		setShellStyle(getShellStyle() | SWT.RESIZE);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText("Select DCI");
		IDialogSettings settings = Activator.getDefault().getDialogSettings();
		try
		{
			newShell.setSize(settings.getInt("SelectDciDialog.cx"), settings.getInt("SelectDciDialog.cy")); //$NON-NLS-1$ //$NON-NLS-2$
		}
		catch(NumberFormatException e)
		{
			newShell.setSize(600, 350);
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
		
		splitter = new SashForm(dialogArea, SWT.HORIZONTAL);
		
		objectTree = new ObjectTree(splitter, SWT.BORDER, ObjectTree.NONE, null, ObjectSelectionDialog.createNodeSelectionFilter());
		IDialogSettings settings = Activator.getDefault().getDialogSettings();
		String text = settings.get("SelectDciDialog.Filter"); //$NON-NLS-1$
		if (text != null)
			objectTree.setFilter(text);

		dciList = new DciList(null, splitter, SWT.BORDER, null, "SelectDciDialog.dciList");  //$NON-NLS-1$
		dciList.getViewer().addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				okPressed();
			}
		});

		try
		{
			int[] weights = new int[2];
			weights[0] = settings.getInt("SelectDciDialog.weight1"); //$NON-NLS-1$
			weights[1] = settings.getInt("SelectDciDialog.weight2"); //$NON-NLS-1$
			splitter.setWeights(weights);
		}
		catch(NumberFormatException e)
		{
			splitter.setWeights(new int[] { 30, 70 });
		}
		
		objectTree.getTreeViewer().addSelectionChangedListener(new ISelectionChangedListener()	{
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				GenericObject object = objectTree.getFirstSelectedObject2();
				if ((object != null) && (object instanceof Node))
					dciList.setNode((Node)object);
				else
					dciList.setNode(null);
			}
		});
		
		return dialogArea;
	}

	/**
	 * Save dialog settings
	 */
	private void saveSettings()
	{
		Point size = getShell().getSize();
		IDialogSettings settings = Activator.getDefault().getDialogSettings();

		settings.put("SelectDciDialog.cx", size.x); //$NON-NLS-1$
		settings.put("SelectDciDialog.cy", size.y); //$NON-NLS-1$
		settings.put("SelectDciDialog.Filter", objectTree.getFilter()); //$NON-NLS-1$
		
		int[] weights = splitter.getWeights();
		settings.put("SelectDciDialog.weight1", weights[0]); //$NON-NLS-1$
		settings.put("SelectDciDialog.weight2", weights[1]); //$NON-NLS-1$
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
	@Override
	protected void okPressed()
	{
		selection = dciList.getSelection();
		if (selection == null)
		{
			MessageDialog.openWarning(getShell(), "Warning", "Please select DCI fro the list and then press OK");
			return;
		}
		saveSettings();
		super.okPressed();
	}

	/**
	 * @return the selection
	 */
	public DciValue getSelection()
	{
		return selection;
	}
}
