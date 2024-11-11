/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.users.propertypages;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.jface.viewers.ViewerFilter;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.configs.CustomAttribute;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.base.widgets.FilterText;
import org.netxms.nxmc.base.widgets.MessageAreaHolder;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.users.dialogs.CustomAttributeEditDialog;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Custom Attributes" property page for user object
 */
public class CustomAttributes extends PropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(CustomAttributes.class);

   private NXCSession session = Registry.getSession();
	private AbstractUserObject object;
   private Map<String, String> attributes;
   private String filterText = "";
   private FilterText filter;
   private SortableTableViewer viewer;

   /**
    * Default constructor
    */
   public CustomAttributes(AbstractUserObject object, MessageAreaHolder messageArea)
   {
      super(LocalizationHelper.getI18n(CustomAttributes.class).tr("Custom Attributes"), messageArea);
      this.object = object;
      this.attributes = new HashMap<>(object.getCustomAttributes());
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
      Composite dialogArea = new Composite(parent, SWT.NONE);

      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      Composite listArea = new Composite(dialogArea, SWT.BORDER);
      listArea.setBackground(parent.getDisplay().getSystemColor(SWT.COLOR_LIST_BACKGROUND));

      layout = new GridLayout();
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      layout.verticalSpacing = 0;
      listArea.setLayout(layout);

      filter = new FilterText(listArea, SWT.NONE, null, false, false);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      filter.setLayoutData(gd);
      filter.setBackground(parent.getDisplay().getSystemColor(SWT.COLOR_LIST_BACKGROUND));
      filter.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            filterText = filter.getText();
            viewer.refresh();
         }
      });

      final String[] columnNames = { i18n.tr("Name"), i18n.tr("Value") };
      final int[] columnWidths = { 150, 250 };
      viewer = new SortableTableViewer(listArea, columnNames, columnWidths, 0, SWT.UP, SWT.MULTI | SWT.FULL_SELECTION);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 400;
      viewer.getControl().setLayoutData(gd);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new AttributeListLabelProvider());
      viewer.setComparator(new ViewerComparator() {
         @SuppressWarnings("unchecked")
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            return ((Entry<String, String>)e1).getKey().compareTo(((Entry<String, String>)e2).getKey());
         }
      });
      viewer.addFilter(new ViewerFilter() {
         @SuppressWarnings("unchecked")
         @Override
         public boolean select(Viewer viewer, Object parentElement, Object element)
         {
            return filterText.isEmpty() ? true : ((Entry<String, String>)element).getKey().toLowerCase().contains(filterText);
         }
      });
      viewer.setInput(attributes.entrySet());

      Composite buttons = new Composite(dialogArea, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttonLayout.marginRight = 0;
      buttons.setLayout(buttonLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      buttons.setLayoutData(gd);

      Button addButton = new Button(buttons, SWT.PUSH);
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

      Button editButton = new Button(buttons, SWT.PUSH);
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

      Button deleteButton = new Button(buttons, SWT.PUSH);
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
      final CustomAttributeEditDialog dlg = new CustomAttributeEditDialog(getShell(), null, null);
      if (dlg.open() != Window.OK)
         return;

      if (attributes.containsKey(dlg.getName()))
      {
         MessageDialogHelper.openWarning(CustomAttributes.this.getShell(), i18n.tr("Warning"), String.format(i18n.tr("Attribute named %s already exists"), dlg.getName()));
      }
      else
      {
         attributes.put(dlg.getName(), dlg.getValue());
         viewer.refresh();
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

      Entry<String, String> element = (Entry<String, String>)selection.getFirstElement();
      final CustomAttributeEditDialog dlg = new CustomAttributeEditDialog(getShell(), element.getKey(), element.getValue());
      if (dlg.open() != Window.OK)
         return;

      if (element.getKey().equals(dlg.getName()))
      {
         attributes.put(dlg.getName(), dlg.getValue());
      }
      else if (attributes.containsKey(dlg.getName()))
      {
         MessageDialogHelper.openWarning(CustomAttributes.this.getShell(), i18n.tr("Warning"), String.format(i18n.tr("Attribute named %s already exists"), dlg.getName()));
      }
      else
      {
         attributes.remove(element.getKey());
         attributes.put(dlg.getName(), dlg.getValue());
      }
      viewer.refresh();
   }

   /**
    * Delete selected attributes
    */
   @SuppressWarnings("unchecked")
   private void deleteAttributes()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      Set<String> keys = new HashSet<>();
      Iterator<Entry<String, CustomAttribute>> it = selection.iterator();
      while(it.hasNext())
         keys.add(it.next().getKey());
      for(String k : keys)
         attributes.remove(k);
      viewer.refresh();
   }

	/**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
	@Override
	protected boolean applyChanges(final boolean isApply)
	{
      if (isApply)
      {
         setMessage(null);
         setValid(false);
      }

      object.setCustomAttributes(attributes);

      new Job(i18n.tr("Updating user database"), null, getMessageArea(isApply)) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				session.modifyUserDBObject(object, AbstractUserObject.MODIFY_ACCESS_RIGHTS);
			}

			@Override
			protected String getErrorMessage()
			{
				return i18n.tr("Cannot update user object");
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

   /**
    * Label provider
    */
   private static class AttributeListLabelProvider extends LabelProvider implements ITableLabelProvider
   {
      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
       */
      @Override
      public Image getColumnImage(Object element, int columnIndex)
      {
         return null;
      }

      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
       */
      @SuppressWarnings("unchecked")
      @Override
      public String getColumnText(Object element, int columnIndex)
      {
         return (columnIndex == 0) ? ((Entry<String, String>)element).getKey() : ((Entry<String, String>)element).getValue();
      }
   }
}
