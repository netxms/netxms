/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectmanager.propertypages;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Event;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.ObjectUrl;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.objectmanager.Messages;
import org.netxms.ui.eclipse.objectmanager.dialogs.ObjectUrlEditDialog;
import org.netxms.ui.eclipse.objectmanager.propertypages.helpers.UrlListLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * "External Resources" property page
 */
public class ExternalResources extends PropertyPage
{
	public static final int COLUMN_NAME = 0;
	public static final int COLUMN_VALUE = 1;
	
	private AbstractObject object = null;
	private SortableTableViewer viewer;
   private Button moveUpButton;
   private Button moveDownButton;
	private Button addButton;
	private Button editButton;
	private Button deleteButton;
	private List<ObjectUrl> urls = null;
	private boolean modified = false;

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		object = (AbstractObject)getElement().getAdapter(AbstractObject.class);
		if (object == null)	// Paranoid check
			return dialogArea;
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 2;
      dialogArea.setLayout(layout);
      
      final String[] columnNames = { "URL", "Description" };
      final int[] columnWidths = { 300, 300 };
      viewer = new SortableTableViewer(dialogArea, columnNames, columnWidths, 0, SWT.UP,
                                       SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new UrlListLabelProvider());
      
      urls = new ArrayList<ObjectUrl>(object.getUrls());
      viewer.setInput(urls);
      
      GridData gridData = new GridData();
      gridData.verticalAlignment = GridData.FILL;
      gridData.grabExcessVerticalSpace = true;
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.horizontalSpan = 2;
      gridData.heightHint = 0;
      viewer.getControl().setLayoutData(gridData);

      Composite buttonsLeft = new Composite(dialogArea, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttonLayout.marginRight = 0;
      buttonsLeft.setLayout(buttonLayout);
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.LEFT;
      buttonsLeft.setLayoutData(gridData);
      
      moveUpButton = new Button(buttonsLeft, SWT.PUSH);
      moveUpButton.setText("&Up");
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      moveUpButton.setLayoutData(rd);
      moveUpButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            moveSelection(true);
         }
      });
      
      moveDownButton = new Button(buttonsLeft, SWT.PUSH);
      moveDownButton.setText("&Down");
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      moveDownButton.setLayoutData(rd);
      moveDownButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            moveSelection(false);
         }
      });
      
      Composite buttonsRight = new Composite(dialogArea, SWT.NONE);
      buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttonLayout.marginRight = 0;
      buttonsRight.setLayout(buttonLayout);
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.RIGHT;
      buttonsRight.setLayoutData(gridData);

      addButton = new Button(buttonsRight, SWT.PUSH);
      addButton.setText(Messages.get().CustomAttributes_Add);
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      addButton.setLayoutData(rd);
      addButton.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
			   addUrl();
			}
      });
		
      editButton = new Button(buttonsRight, SWT.PUSH);
      editButton.setText(Messages.get().CustomAttributes_Modify);
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      editButton.setLayoutData(rd);
      editButton.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
			   editUrl();
			}
      });
		
      deleteButton = new Button(buttonsRight, SWT.PUSH);
      deleteButton.setText(Messages.get().CustomAttributes_Delete);
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      deleteButton.setLayoutData(rd);
      deleteButton.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
			   deleteUrl();
			}
      });
		
      viewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				editButton.notifyListeners(SWT.Selection, new Event());
			}
      });
      
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
				moveUpButton.setEnabled(selection.size() == 1);
            moveDownButton.setEnabled(selection.size() == 1);
            editButton.setEnabled(selection.size() == 1);
				deleteButton.setEnabled(selection.size() > 0);
			}
		});
      
		return dialogArea;
	}
	
	/**
	 * Add new URL
	 */
	private void addUrl()
	{
	   ObjectUrlEditDialog dlg = new ObjectUrlEditDialog(getShell(), null, null);
	   if (dlg.open() != Window.OK)
	      return;
	   
	   urls.add(new ObjectUrl(urls.size(), dlg.getUrl(), dlg.getDescription()));
	   viewer.refresh();
	   modified = true;
	}
	
	/**
	 * Edit selected URL 
	 */
	private void editUrl()
	{
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
	   if (selection.size() != 1)
	      return;

	   ObjectUrl url = (ObjectUrl)selection.getFirstElement();
      ObjectUrlEditDialog dlg = new ObjectUrlEditDialog(getShell(), url.getUrl(), url.getDescription());
      if (dlg.open() != Window.OK)
         return;

      int index = urls.indexOf(url);
      urls.set(index, new ObjectUrl(url.getId(), dlg.getUrl(), dlg.getDescription()));
      viewer.refresh();
      viewer.setSelection(new StructuredSelection(urls.get(index)));
      modified = true;
	}
	
	/**
	 * Delete selected URLs
	 */
	private void deleteUrl()
	{
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.isEmpty())
         return;
      
	   for(Object o : selection.toList())
	      urls.remove(o);
	   for(int i = 0; i < urls.size(); i++)
	   {
	      ObjectUrl u = urls.get(i);
	      urls.set(i, new ObjectUrl(i, u.getUrl(), u.getDescription()));
	   }
      viewer.refresh();
      modified = true;
	}
	
	/**
	 * Move selection
	 * 
	 * @param up
	 */
	private void moveSelection(boolean up)
	{
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.size() != 1)
         return;

      ObjectUrl url = (ObjectUrl)selection.getFirstElement();
      int index = urls.indexOf(url);
      if (((index == 0) && up) || ((index == urls.size() - 1) && !up))
         return;
      
      int swapIndex = up ? index - 1 : index + 1;
      ObjectUrl swapUrl = urls.get(swapIndex);
      
      urls.set(index, new ObjectUrl(index, swapUrl.getUrl(), swapUrl.getDescription()));
      urls.set(swapIndex, new ObjectUrl(index, url.getUrl(), url.getDescription()));
      
      viewer.refresh();
      viewer.setSelection(new StructuredSelection(urls.get(swapIndex)));
      modified = true;
	}

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
		if (!modified)
			return;		// Nothing to apply
		
		if (isApply)
			setValid(false);
		
		final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
		md.setUrls(new ArrayList<ObjectUrl>(urls));
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob(Messages.get().CustomAttributes_JobName, null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
				modified = false;
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot update object's URL list";
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
				{
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
							ExternalResources.this.setValid(true);
						}
					});
				}
			}
		}.start();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performApply()
	 */
	@Override
	protected void performApply()
	{
		applyChanges(true);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		applyChanges(false);
		return true;
	}
}
