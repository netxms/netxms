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
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.ControlListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.ScrollBar;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.objectbrowser.Activator;
import org.netxms.ui.eclipse.objectbrowser.Messages;
import org.netxms.ui.eclipse.objectbrowser.widgets.internal.ObjectListFilter;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Dialog used to select child object(s) of given object
 *
 */
public class ChildObjectListDialog extends Dialog
{
	private long parentObject;
	private Set<Integer> classFilter;
	private ObjectListFilter filter;
	private Text filterText;
	private TableViewer objectList;
	private List<GenericObject> selectedObjects;
	
	/**
	 * Create class filter from array of allowed classes.
	 * 
	 * @param classes
	 * @return
	 */
	public static Set<Integer> createClassFilter(int[] classes)
	{
		Set<Integer> filter = new HashSet<Integer>(classes.length);
		for(int i = 0; i < classes.length; i++)
			filter.add(classes[i]);
		return filter;
	}

	/**
	 * Create class filter with single allowed class
	 * 
	 * @param c allowed class
	 * @return
	 */
	public static Set<Integer> createClassFilter(int c)
	{
		Set<Integer> filter = new HashSet<Integer>(1);
		filter.add(c);
		return filter;
	}
	
	/**
	 * @param parentShell
	 */
	public ChildObjectListDialog(Shell parentShell, long parentObject, Set<Integer> classFilter)
	{
		super(parentShell);
		setShellStyle(getShellStyle() | SWT.RESIZE);
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
		newShell.setText(Messages.ChildObjectListDialog_SelectSubordinate);
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
		
		GenericObject object = ((NXCSession)ConsoleSharedData.getSession()).findObjectById(parentObject);
		GenericObject[] sourceObjects = (object != null) ? object.getChildsAsArray() : new GenericObject[0];
		
		GridLayout layout = new GridLayout();
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		dialogArea.setLayout(layout);
		
		// Create filter area
		Composite filterArea = new Composite(dialogArea, SWT.NONE);
		layout = new GridLayout();
		layout.numColumns = 2;
		layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		filterArea.setLayout(layout);
		filterArea.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		
		Label filterLabel = new Label(filterArea, SWT.NONE);
		filterLabel.setText(Messages.ChildObjectListDialog_Filter);
		
		filterText = new Text(filterArea, SWT.BORDER);
		filterText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		filterText.addModifyListener(new ModifyListener() {
			@Override
			public void modifyText(ModifyEvent e)
			{
				filter.setFilterString(filterText.getText());
				objectList.refresh(false);
				GenericObject obj = filter.getLastMatch();
				if (obj != null)
				{
					objectList.setSelection(new StructuredSelection(obj), true);
					objectList.reveal(obj);
				}
			}
		});
		
		// Create object list
		objectList = new TableViewer(dialogArea, SWT.BORDER | SWT.FULL_SELECTION | SWT.MULTI);
		TableColumn tc = new TableColumn(objectList.getTable(), SWT.LEFT); 
		tc.setText(Messages.ChildObjectListDialog_Name);
		tc.setWidth(280);
		objectList.getTable().setHeaderVisible(false);
		objectList.setContentProvider(new ArrayContentProvider());
		objectList.setLabelProvider(new WorkbenchLabelProvider());
		objectList.setComparator(new ViewerComparator());
		filter = new ObjectListFilter(sourceObjects, classFilter);
		objectList.addFilter(filter);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		objectList.getControl().setLayoutData(gd);
		objectList.getTable().addControlListener(new ControlListener() {
			@Override
			public void controlMoved(ControlEvent e)
			{
			}

			@Override
			public void controlResized(ControlEvent e)
			{
				Table table = objectList.getTable();
				int w = table.getSize().x - table.getBorderWidth() * 2;
				ScrollBar sc = table.getVerticalBar();
				if ((sc != null) && sc.isVisible())
					w -= sc.getSize().x;
				table.getColumn(0).setWidth(w);
			}
		});
		
		if (object != null)
			objectList.setInput(sourceObjects);
		
		filterText.setFocus();
		
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
	@SuppressWarnings("rawtypes")
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

		settings.put("ChildObjectListDialog.cx", size.x); //$NON-NLS-1$
		settings.put("ChildObjectListDialog.cy", size.y); //$NON-NLS-1$
	}

	/**
	 * @return the selectedObjects
	 */
	public List<GenericObject> getSelectedObjects()
	{
		return selectedObjects;
	}
}
