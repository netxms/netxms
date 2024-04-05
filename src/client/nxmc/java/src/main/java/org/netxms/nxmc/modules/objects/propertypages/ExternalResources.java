/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.propertypages;

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
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Event;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.ObjectUrl;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.dialogs.ObjectUrlEditDialog;
import org.netxms.nxmc.modules.objects.propertypages.helpers.UrlListLabelProvider;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "External Resources" property page
 */
public class ExternalResources extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(ExternalResources.class);

	public static final int COLUMN_NAME = 0;
	public static final int COLUMN_VALUE = 1;

	private SortableTableViewer viewer;
   private Button moveUpButton;
   private Button moveDownButton;
	private Button addButton;
	private Button editButton;
	private Button deleteButton;
	private List<ObjectUrl> urls = null;
	private boolean modified = false;

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public ExternalResources(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(ExternalResources.class).tr("External Resources"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "externalResources";
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 2;
      dialogArea.setLayout(layout);
      
      final String[] columnNames = { i18n.tr("URL"), i18n.tr("Description") };
      final int[] columnWidths = { 300, 300 };
      viewer = new SortableTableViewer(dialogArea, columnNames, columnWidths, 0, SWT.UP, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
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
      moveUpButton.setText(i18n.tr("&Up"));
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      moveUpButton.setLayoutData(rd);
      moveUpButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            moveSelection(true);
         }
      });
      
      moveDownButton = new Button(buttonsLeft, SWT.PUSH);
      moveDownButton.setText(i18n.tr("Dow&n"));
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      moveDownButton.setLayoutData(rd);
      moveDownButton.addSelectionListener(new SelectionAdapter() {
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
      addButton.setText(i18n.tr("&Add..."));
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      addButton.setLayoutData(rd);
      addButton.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
			   addUrl();
			}
      });

      editButton = new Button(buttonsRight, SWT.PUSH);
      editButton.setText(i18n.tr("&Edit..."));
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      editButton.setLayoutData(rd);
      editButton.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
			   editUrl();
			}
      });
		
      deleteButton = new Button(buttonsRight, SWT.PUSH);
      deleteButton.setText(i18n.tr("&Delete"));
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      deleteButton.setLayoutData(rd);
      deleteButton.addSelectionListener(new SelectionAdapter() {
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
            IStructuredSelection selection = viewer.getStructuredSelection();
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
      IStructuredSelection selection = viewer.getStructuredSelection();
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
      IStructuredSelection selection = viewer.getStructuredSelection();
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
      IStructuredSelection selection = viewer.getStructuredSelection();
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
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
	{
		if (!modified)
         return true; // Nothing to apply

		if (isApply)
			setValid(false);

		final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
		md.setUrls(new ArrayList<ObjectUrl>(urls));
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating external resource list"), null, messageArea) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
				modified = false;
			}

			@Override
			protected String getErrorMessage()
			{
            return "Cannot update object's external resource list";
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
      return true;
	}
}
