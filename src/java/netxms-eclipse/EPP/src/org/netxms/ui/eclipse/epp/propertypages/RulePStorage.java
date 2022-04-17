/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.epp.propertypages;

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
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.ui.eclipse.epp.widgets.RuleEditor;
import org.netxms.ui.eclipse.tools.ElementLabelComparator;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.KeyValueSetEditor;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;
import org.netxms.ui.eclipse.console.dialogs.KeyValuePairEditDialog;

/**
 * "Events" property page for EPP rule
 */
public class RulePStorage extends PropertyPage
{
	private RuleEditor editor;
	private EventProcessingPolicyRule rule;
	private KeyValueSetEditor keysToSetEditor;
   private SortableTableViewer keysToDeleteViewer;
   private Button addToDeleteListButton;
   private Button editDeleteListButton;
   private Button removeFromDeleteListButton;
   private List<String> keysToDelete = new ArrayList<String>(0);

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		editor = (RuleEditor)getElement().getAdapter(RuleEditor.class);
		rule = editor.getRule();
		keysToDelete.addAll(rule.getPStorageDelete());

		Composite dialogArea = new Composite(parent, SWT.NONE);
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.INNER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);
      final int vInd = WidgetHelper.OUTER_SPACING - WidgetHelper.INNER_SPACING;
      
      Label label = new Label(dialogArea, SWT.NONE);
      label.setText("Set persistent storage values");
      GridData gd = new GridData();
      gd.verticalIndent = vInd;
      label.setLayoutData(gd);
      
      keysToSetEditor = new KeyValueSetEditor(dialogArea, SWT.NONE);
      keysToSetEditor.addAll(rule.getPStorageSet());
      gd = new GridData();
      gd.verticalIndent = vInd;
      gd.verticalAlignment = GridData.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = GridData.FILL;
      gd.grabExcessHorizontalSpace = true;
      keysToSetEditor.setLayoutData(gd);

      label = new Label(dialogArea, SWT.NONE);
      label.setText("Delete persistent storage entries");
      gd = new GridData();
      gd.verticalIndent = vInd;
      label.setLayoutData(gd);

      final String[] deleteColumnNames = { "Key" };
      final int[] deleteColumnWidths = { 150 };
      keysToDeleteViewer = new SortableTableViewer(dialogArea, deleteColumnNames, deleteColumnWidths, 0, SWT.UP, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      keysToDeleteViewer.setContentProvider(new ArrayContentProvider());
      keysToDeleteViewer.setComparator(new ElementLabelComparator((ILabelProvider)keysToDeleteViewer.getLabelProvider()));
      keysToDeleteViewer.setInput(keysToDelete.toArray());
      keysToDeleteViewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            int size = ((IStructuredSelection)keysToDeleteViewer.getSelection()).size();
            editDeleteListButton.setEnabled(size == 1);
            removeFromDeleteListButton.setEnabled(size > 0);
         }
      });

      gd = new GridData();
      gd.verticalAlignment = GridData.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = GridData.FILL;
      gd.grabExcessHorizontalSpace = true;
      keysToDeleteViewer.getControl().setLayoutData(gd);
      
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
      addToDeleteListButton.setText("Add");
      addToDeleteListButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            addPStorageDeleteAction();
         }
      });
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      addToDeleteListButton.setLayoutData(rd);

      editDeleteListButton = new Button(buttonsDeleteValue, SWT.PUSH);
      editDeleteListButton.setText("Edit");
      editDeleteListButton.addSelectionListener(new SelectionListener() {
        @Override
        public void widgetDefaultSelected(SelectionEvent e)
        {
           widgetSelected(e);
        }

        @Override
        public void widgetSelected(SelectionEvent e)
        {
           editPStorageDeleteAction();
        }
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      editDeleteListButton.setLayoutData(rd);
      editDeleteListButton.setEnabled(false);
      
      removeFromDeleteListButton = new Button(buttonsDeleteValue, SWT.PUSH);
      removeFromDeleteListButton.setText("Delete");
      removeFromDeleteListButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            deletePStorageDeleteAction();
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
	private void addPStorageDeleteAction()
	{
	   KeyValuePairEditDialog dlg = new KeyValuePairEditDialog(getShell(), null, null, false, false);
		if (dlg.open() == Window.OK)
		{
		   keysToDelete.add(dlg.getAtributeName());
		   keysToDeleteViewer.setInput(keysToDelete.toArray());
		}
	}
   
   /**
    * Edit delete value
    */
   private void editPStorageDeleteAction()
   {
      IStructuredSelection selection = (IStructuredSelection)keysToDeleteViewer.getSelection();
      if (selection.size() != 1)
         return;
      
      String attr = (String)selection.getFirstElement();
      KeyValuePairEditDialog dlg = new KeyValuePairEditDialog(getShell(), attr, null, false, false);
      if (dlg.open() == Window.OK)
      {         
         keysToDelete.set(keysToDelete.indexOf(attr), dlg.getAtributeName());
         keysToDeleteViewer.setInput(keysToDelete.toArray());    
      }
   }
   
   /**
    * Delete attribute(s) from list
    */
   private void deletePStorageDeleteAction()
   {
      IStructuredSelection selection = (IStructuredSelection)keysToDeleteViewer.getSelection();
      Iterator<?> it = selection.iterator();
      if (it.hasNext())
      {
         while(it.hasNext())
         {
            String e = (String)it.next();
            keysToDelete.remove(e);
         }
         keysToDeleteViewer.setInput(keysToDelete.toArray());
      }
   }
	
	/**
	 * Update rule object
	 */
	private void doApply()
	{
		rule.setPStorageSet(keysToSetEditor.getContent());
      rule.setPStorageDelete(keysToDelete);
		editor.setModified(true);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performApply()
	 */
	@Override
	protected void performApply()
	{
		doApply();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		doApply();
		return super.performOk();
	}
}
