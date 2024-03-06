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

import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import java.util.Map.Entry;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
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
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.configs.CustomAttribute;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.objectmanager.Messages;
import org.netxms.ui.eclipse.objectmanager.dialogs.AttributeEditDialog;
import org.netxms.ui.eclipse.objectmanager.propertypages.helpers.AttrListLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.ElementLabelComparator;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * "Custom Attributes" property page
 */
public class CustomAttributes extends PropertyPage
{
	public static final int COLUMN_NAME = 0;
	public static final int COLUMN_VALUE = 1;
	public static final int COLUMN_INHERITABLE = 2;
	public static final int COLUMN_INHERETED_FROM = 3;
	
	private AbstractObject object = null;
	private SortableTableViewer viewer;
	private Button addButton;
	private Button editButton;
	private Button deleteButton;
	private Map<String, CustomAttribute> attributes = null;
	private boolean isModified = false;

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
      dialogArea.setLayout(layout);
      
      final String[] columnNames = { Messages.get().CustomAttributes_Name, Messages.get().CustomAttributes_Value, "Inheritable", "Inherited From" };
      final int[] columnWidths = { 150, 250, 80, 250 };
      viewer = new SortableTableViewer(dialogArea, columnNames, columnWidths, 0, SWT.UP,
                                       SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      AttrListLabelProvider labelProvider = new AttrListLabelProvider(object);
      viewer.setLabelProvider(labelProvider);
      viewer.setComparator(new ElementLabelComparator(labelProvider));
      
      attributes = new HashMap<String, CustomAttribute>(object.getCustomAttributes());
      viewer.setInput(attributes.entrySet());
      
      if (!Platform.getPreferencesService().getBoolean("org.netxms.ui.eclipse.console", "SHOW_HIDDEN_ATTRIBUTES", false, null)) //$NON-NLS-1$ //$NON-NLS-2$
      {
	      viewer.addFilter(new ViewerFilter() {
				@SuppressWarnings("unchecked")
				@Override
				public boolean select(Viewer viewer, Object parentElement, Object element)
				{
					return !((Entry<String, String>)element).getKey().startsWith("."); //$NON-NLS-1$
				}
			});
      }
      
      GridData gridData = new GridData();
      gridData.verticalAlignment = GridData.FILL;
      gridData.grabExcessVerticalSpace = true;
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.heightHint = 0;
      viewer.getControl().setLayoutData(gridData);
      
      Composite buttons = new Composite(dialogArea, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttonLayout.marginRight = 0;
      buttons.setLayout(buttonLayout);
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.RIGHT;
      buttons.setLayoutData(gridData);

      addButton = new Button(buttons, SWT.PUSH);
      addButton.setText(Messages.get().CustomAttributes_Add);
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      addButton.setLayoutData(rd);
      addButton.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
            addAttribute();
			}
      });
		
      editButton = new Button(buttons, SWT.PUSH);
      editButton.setText(Messages.get().CustomAttributes_Modify);
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      editButton.setLayoutData(rd);
      editButton.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
            editAttribute();
			}
      });
		
      deleteButton = new Button(buttons, SWT.PUSH);
      deleteButton.setText(Messages.get().CustomAttributes_Delete);
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      deleteButton.setLayoutData(rd);
      deleteButton.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
            deleteAttributes();
			}
      });

      viewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
            editAttribute();
			}
      });
      
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
				editButton.setEnabled(selection.size() == 1);
				deleteButton.setEnabled(selection.size() > 0);
			}
		});
      
		return dialogArea;
	}

   /**
    * Add new attribute
    */
   private void addAttribute()
   {
      final AttributeEditDialog dlg = new AttributeEditDialog(CustomAttributes.this.getShell(), null, null, 0, 0);
      if (dlg.open() != Window.OK)
         return;

      if (attributes.containsKey(dlg.getName()))
      {
         MessageDialogHelper.openWarning(CustomAttributes.this.getShell(), Messages.get().CustomAttributes_Warning, String.format(Messages.get().CustomAttributes_WarningAlreadyExist, dlg.getName()));
      }
      else
      {
         attributes.put(dlg.getName(), new CustomAttribute(dlg.getValue(), dlg.getFlags(), 0));
         viewer.setInput(attributes.entrySet());
         CustomAttributes.this.isModified = true;
      }
   }

   /**
    * Edit selected attribute
    */
   @SuppressWarnings("unchecked")
   private void editAttribute()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      Entry<String, CustomAttribute> element = (Entry<String, CustomAttribute>)selection.getFirstElement();
      CustomAttribute attr = element.getValue();
      final AttributeEditDialog dlg = new AttributeEditDialog(CustomAttributes.this.getShell(), element.getKey(), attr.getValue(), attr.getFlags(), attr.getSourceObject());
      if (dlg.open() == Window.OK)
      {
         attributes.put(dlg.getName(), new CustomAttribute(dlg.getValue(), dlg.getFlags(), attr.getSourceObject()));
         viewer.setInput(attributes.entrySet());
         CustomAttributes.this.isModified = true;
      }
   }

   /**
    * Delete selected attributes
    */
   @SuppressWarnings("unchecked")
   private void deleteAttributes()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      Iterator<Entry<String, CustomAttribute>> it = selection.iterator();
      boolean modified = false;
      while(it.hasNext())
      {
         Entry<String, CustomAttribute> element = it.next();
         if (element.getValue().isInherited() && !element.getValue().isRedefined())
            continue;
         if (element.getValue().isRedefined())
         {
            CustomAttribute parentCa = null;
            AbstractObject parentObject = null;
            Set<Long> objectSet = new HashSet<Long>();
            for(AbstractObject obj : object.getParentsAsArray())
            {
               CustomAttribute ca = obj.getCustomAttribute(element.getKey());
               if (element.getValue().getSourceObject() == obj.getObjectId())
               {
                  if (ca != null && ca.isInheritable())
                  {
                     parentCa = ca;
                     parentObject = obj;
                  }
                  objectSet.add(obj.getObjectId());
               }
               else if (ca != null && ca.isInheritable())
               {
                  objectSet.add(obj.getObjectId());                  
               }
            }
            if (parentCa == null) //Search any parent with attribute
            {
               for(AbstractObject obj : object.getParentsAsArray())
               {
                  CustomAttribute ca = obj.getCustomAttribute(element.getKey());
                  if (ca != null && ca.isInheritable())
                  {
                     parentCa = ca;
                     parentObject = obj;
                     break;
                  }
               }
            }
               
               
            if (parentCa == null) // Parent attribute not found, do real delete
               attributes.remove(element.getKey());
            else
               attributes.put(element.getKey(), 
                     new CustomAttribute(parentCa.getValue(), 
                           objectSet.size() > 1 ? CustomAttribute.INHERITABLE | CustomAttribute.CONFLICT : CustomAttribute.INHERITABLE, 
                           parentCa.isInherited() && !parentCa.isRedefined() ? parentCa.getSourceObject() : parentObject.getObjectId()));
            modified = true;
         }
         else
         {
            attributes.remove(element.getKey());
            modified = true;
         }
      }
      if (modified)
      {
         viewer.setInput(attributes.entrySet());
         CustomAttributes.this.isModified = true;
      }
   }

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
		if (!isModified)
			return;		// Nothing to apply
		
		if (isApply)
			setValid(false);
		
		final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
		md.setCustomAttributes(attributes);
		final NXCSession session = ConsoleSharedData.getSession();
		new ConsoleJob(Messages.get().CustomAttributes_JobName, null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
				isModified = false;
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().CustomAttributes_JobError;
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
							CustomAttributes.this.setValid(true);
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
