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

import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import java.util.Map.Entry;
import org.eclipse.core.runtime.IProgressMonitor;
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
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.configs.CustomAttribute;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.dialogs.AttributeEditDialog;
import org.netxms.nxmc.modules.objects.propertypages.helpers.AttrListLabelProvider;
import org.netxms.nxmc.tools.ElementLabelComparator;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Custom Attributes" property page
 */
public class CustomAttributes extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(CustomAttributes.class);

	public static final int COLUMN_NAME = 0;
	public static final int COLUMN_VALUE = 1;
	public static final int COLUMN_INHERITABLE = 2;
	public static final int COLUMN_INHERETED_FROM = 3;

	private SortableTableViewer viewer;
	private Button addButton;
	private Button editButton;
	private Button deleteButton;
	private Map<String, CustomAttribute> attributes = null;
	private boolean isModified = false;

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public CustomAttributes(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(CustomAttributes.class).tr("Custom Attributes"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "customAttributes";
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
      dialogArea.setLayout(layout);

      final String[] columnNames = { i18n.tr("Name"), i18n.tr("Value"), i18n.tr("Inheritable"), i18n.tr("Origin") };
      final int[] columnWidths = { 150, 250, 80, 250 };
      viewer = new SortableTableViewer(dialogArea, columnNames, columnWidths, 0, SWT.UP, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      AttrListLabelProvider labelProvider = new AttrListLabelProvider(object);
      viewer.setLabelProvider(labelProvider);
      viewer.setComparator(new ElementLabelComparator(labelProvider));

      attributes = new HashMap<String, CustomAttribute>(object.getCustomAttributes());
      viewer.setInput(attributes.entrySet());

      if (!PreferenceStore.getInstance().getAsBoolean("CustomAttributes.ShowHidden", false))
      {
	      viewer.addFilter(new ViewerFilter() {
				@SuppressWarnings("unchecked")
				@Override
				public boolean select(Viewer viewer, Object parentElement, Object element)
				{
					return !((Entry<String, String>)element).getKey().startsWith(".");
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
      addButton.setText(i18n.tr("&Add..."));
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
      editButton.setText(i18n.tr("&Modify..."));
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
      deleteButton.setText(i18n.tr("&Delete"));
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
         MessageDialogHelper.openWarning(CustomAttributes.this.getShell(), i18n.tr("Warning"), String.format(i18n.tr("Attribute named %s already exists"), dlg.getName()));
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
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
	{
		if (!isModified)
         return true; // Nothing to apply

		if (isApply)
			setValid(false);

		final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
		md.setCustomAttributes(attributes);
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating custom attributes"), null, messageArea) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
				isModified = false;
			}

			@Override
			protected String getErrorMessage()
			{
				return i18n.tr("Cannot update object's custom attributes");
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
               runInUIThread(() -> CustomAttributes.this.setValid(true));
			}
		}.start();
		return true;
	}
}
