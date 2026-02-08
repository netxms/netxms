/**
 * NetXMS - open source network management system
 * Copyright (C) 2022-2025 Raden Solutions
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
package org.netxms.nxmc.modules.events.propertypages;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ILabelProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
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
import org.eclipse.swt.widgets.Label;
import org.netxms.nxmc.base.dialogs.KeyValuePairEditDialog;
import org.netxms.nxmc.base.widgets.KeyValueSetEditor;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.events.widgets.RuleEditor;
import org.netxms.nxmc.tools.ElementLabelComparator;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Custom Attributes" property page for EPP rule
 */
public class RuleCustomAttribute extends RuleBasePropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(RuleCustomAttribute.class);

   private KeyValueSetEditor namesToSetEditor;
   private SortableTableViewer namesToDeleteViewer;
   private Button addToDeleteListButton;
   private Button editDeleteListButton;
   private Button removeFromDeleteListButton;
   private List<String> namesToDelete = new ArrayList<String>(0);

   /**
    * Create property page.
    *
    * @param editor rule editor
    */
   public RuleCustomAttribute(RuleEditor editor)
   {
      super(editor, LocalizationHelper.getI18n(RuleCustomAttribute.class).tr("Custom Attributes"));
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      namesToDelete.addAll(rule.getCustomAttributeStorageDelete());

		Composite dialogArea = new Composite(parent, SWT.NONE);
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.INNER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);
      final int vInd = WidgetHelper.OUTER_SPACING - WidgetHelper.INNER_SPACING;

      Label label = new Label(dialogArea, SWT.NONE);
      label.setText(i18n.tr("Set custom attribute values"));
      GridData gd = new GridData();
      gd.verticalIndent = vInd;
      label.setLayoutData(gd);
      
      namesToSetEditor = new KeyValueSetEditor(dialogArea, SWT.NONE, i18n.tr("Name"), i18n.tr("Value"));
      namesToSetEditor.addAll(rule.getCustomAttributeStorageSet());
      gd = new GridData();
      gd.verticalIndent = vInd;
      gd.verticalAlignment = GridData.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = GridData.FILL;
      gd.grabExcessHorizontalSpace = true;
      namesToSetEditor.setLayoutData(gd);

      label = new Label(dialogArea, SWT.NONE);
      label.setText(i18n.tr("Delete custom attribute entries"));
      gd = new GridData();
      gd.verticalIndent = vInd;
      label.setLayoutData(gd);

      final String[] deleteColumnNames = { i18n.tr("Name") };
      final int[] deleteColumnWidths = { 150 };
      namesToDeleteViewer = new SortableTableViewer(dialogArea, deleteColumnNames, deleteColumnWidths, 0, SWT.UP, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      namesToDeleteViewer.setContentProvider(new ArrayContentProvider());
      namesToDeleteViewer.setComparator(new ElementLabelComparator((ILabelProvider)namesToDeleteViewer.getLabelProvider()));
      namesToDeleteViewer.setInput(namesToDelete.toArray());
      namesToDeleteViewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            int size = namesToDeleteViewer.getStructuredSelection().size();
            editDeleteListButton.setEnabled(size == 1);
            removeFromDeleteListButton.setEnabled(size > 0);
         }
      });

      gd = new GridData();
      gd.verticalAlignment = GridData.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = GridData.FILL;
      gd.grabExcessHorizontalSpace = true;
      namesToDeleteViewer.getControl().setLayoutData(gd);

      Composite buttonsDeleteValue = new Composite(dialogArea, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginLeft = 0;
      buttonLayout.marginRight = 0;
      buttonsDeleteValue.setLayout(buttonLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      buttonsDeleteValue.setLayoutData(gd);

      addToDeleteListButton = new Button(buttonsDeleteValue, SWT.PUSH);
      addToDeleteListButton.setText(i18n.tr("&Add..."));
      addToDeleteListButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            addCustomAttributeDeleteAction();
         }
      });
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      addToDeleteListButton.setLayoutData(rd);

      editDeleteListButton = new Button(buttonsDeleteValue, SWT.PUSH);
      editDeleteListButton.setText(i18n.tr("&Edit..."));
      editDeleteListButton.addSelectionListener(new SelectionAdapter() {
        @Override
        public void widgetSelected(SelectionEvent e)
        {
           editCustomAttributeDeleteAction();
        }
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      editDeleteListButton.setLayoutData(rd);
      editDeleteListButton.setEnabled(false);
      
      removeFromDeleteListButton = new Button(buttonsDeleteValue, SWT.PUSH);
      removeFromDeleteListButton.setText(i18n.tr("&Delete"));
      removeFromDeleteListButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            deleteCustomAttributeDeleteAction();
         }
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      removeFromDeleteListButton.setLayoutData(rd);
      removeFromDeleteListButton.setEnabled(false);

      return dialogArea;
   }

	/**
	 * Add new attribute
	 */
	private void addCustomAttributeDeleteAction()
	{
	   KeyValuePairEditDialog dlg = new KeyValuePairEditDialog(getShell(), null, null, false, false, i18n.tr("Name"), i18n.tr("Value"));
		if (dlg.open() == Window.OK)
		{
         namesToDelete.add(dlg.getKey());
		   namesToDeleteViewer.setInput(namesToDelete.toArray());
		}
	}

   /**
    * Edit delete value
    */
   private void editCustomAttributeDeleteAction()
   {
      IStructuredSelection selection = namesToDeleteViewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      String attr = (String)selection.getFirstElement();
      KeyValuePairEditDialog dlg = new KeyValuePairEditDialog(getShell(), attr, null, false, false, i18n.tr("Name"), i18n.tr("Value"));
      if (dlg.open() == Window.OK)
      {         
         namesToDelete.set(namesToDelete.indexOf(attr), dlg.getKey());
         namesToDeleteViewer.setInput(namesToDelete.toArray());    
      }
   }

   /**
    * Delete attribute(s) from list
    */
   private void deleteCustomAttributeDeleteAction()
   {
      IStructuredSelection selection = namesToDeleteViewer.getStructuredSelection();
      Iterator<?> it = selection.iterator();
      if (it.hasNext())
      {
         while(it.hasNext())
         {
            String e = (String)it.next();
            namesToDelete.remove(e);
         }
         namesToDeleteViewer.setInput(namesToDelete.toArray());
      }
   }

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
	{
      rule.setCustomAttributeStorageSet(namesToSetEditor.getContent());
      rule.setCustomAttributeStorageDelete(namesToDelete);
		editor.setModified(true);
      return true;
	}
}
