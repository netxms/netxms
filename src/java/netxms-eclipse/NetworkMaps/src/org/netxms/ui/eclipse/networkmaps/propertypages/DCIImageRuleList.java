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
package org.netxms.ui.eclipse.networkmaps.propertypages;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
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
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.maps.configs.DCIImageConfiguration;
import org.netxms.client.maps.configs.DCIImageRule;
import org.netxms.client.maps.elements.NetworkMapDCIImage;
import org.netxms.ui.eclipse.networkmaps.Activator;
import org.netxms.ui.eclipse.networkmaps.Messages;
import org.netxms.ui.eclipse.networkmaps.dialogs.EditDCIImageRuleDialog;
import org.netxms.ui.eclipse.networkmaps.propertypages.helper.DCIImageRuleLabelProvider;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * DCI list editor for dashboard element
 */
public class DCIImageRuleList extends PropertyPage
{
   public static final int COLUMN_OPERATION = 0;
   public static final int COLUMN_COMMENT = 1;

   private static final String COLUMN_SETTINGS_PREFIX = "DCIImageRuleList.ImageRuleList"; //$NON-NLS-1$

   private NetworkMapDCIImage container;
   private DCIImageConfiguration config;
   private List<DCIImageRule> rules;
   private TableViewer RuleList;
   private Button addButton;
   private Button modifyButton;
   private Button deleteButton;
   private Button upButton;
   private Button downButton;

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      container = (NetworkMapDCIImage)getElement().getAdapter(NetworkMapDCIImage.class);
      config = container.getImageOptions();
      rules = config.getRulesAsList();
      if (rules == null)
         rules = new ArrayList<DCIImageRule>();

      Composite dialogArea = new Composite(parent, SWT.NONE);

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      Composite RuleArea = new Composite(dialogArea, SWT.NONE);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalSpan = 2;
      RuleArea.setLayoutData(gd);
      layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.INNER_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.numColumns = 2;
      RuleArea.setLayout(layout);

      new Label(RuleArea, SWT.NONE).setText(Messages.get().DCIImageRuleList_Rules);

      RuleList = new TableViewer(RuleArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalSpan = 2;
      RuleList.getControl().setLayoutData(gd);
      setupRuleList();
      RuleList.setInput(rules.toArray());

      Composite leftButtons = new Composite(RuleArea, SWT.NONE);
      gd = new GridData();
      gd.horizontalAlignment = SWT.LEFT;
      leftButtons.setLayoutData(gd);
      RowLayout buttonsLayout = new RowLayout(SWT.HORIZONTAL);
      buttonsLayout.marginBottom = 0;
      buttonsLayout.marginLeft = 0;
      buttonsLayout.marginRight = 0;
      buttonsLayout.marginTop = 0;
      buttonsLayout.spacing = WidgetHelper.OUTER_SPACING;
      buttonsLayout.fill = true;
      buttonsLayout.pack = false;
      leftButtons.setLayout(buttonsLayout);

      upButton = new Button(leftButtons, SWT.PUSH);
      upButton.setText(Messages.get().DCIImageRuleList_Up);
      upButton.setEnabled(false);
      upButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            moveUp();
         }
      });

      downButton = new Button(leftButtons, SWT.PUSH);
      downButton.setText(Messages.get().DCIImageRuleList_Down);
      downButton.setEnabled(false);
      downButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            moveDown();
         }
      });

      Composite buttons = new Composite(RuleArea, SWT.NONE);
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      buttons.setLayoutData(gd);
      buttonsLayout = new RowLayout(SWT.HORIZONTAL);
      buttonsLayout.marginBottom = 0;
      buttonsLayout.marginLeft = 0;
      buttonsLayout.marginRight = 0;
      buttonsLayout.marginTop = 0;
      buttonsLayout.spacing = WidgetHelper.OUTER_SPACING;
      buttonsLayout.fill = true;
      buttonsLayout.pack = false;
      buttons.setLayout(buttonsLayout);

      addButton = new Button(buttons, SWT.PUSH);
      addButton.setText(Messages.get().DCIImageRuleList_Add);
      RowData rd = new RowData();
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
            addRule();
         }
      });

      modifyButton = new Button(buttons, SWT.PUSH);
      modifyButton.setText(Messages.get().DCIImageRuleList_Edit);
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      modifyButton.setLayoutData(rd);
      modifyButton.setEnabled(false);
      modifyButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            editRule();
         }
      });

      deleteButton = new Button(buttons, SWT.PUSH);
      deleteButton.setText(Messages.get().DCIImageRuleList_Delete);
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      deleteButton.setLayoutData(rd);
      deleteButton.setEnabled(false);
      deleteButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            deleteRules();
         }
      });

      /*** Selection change listener for Rules list ***/
      RuleList.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = (IStructuredSelection)event.getSelection();
            int index = rules.indexOf(selection.getFirstElement());
            upButton.setEnabled((selection.size() == 1) && (index > 0));
            downButton.setEnabled((selection.size() == 1) && (index >= 0) && (index < rules.size() - 1));
            modifyButton.setEnabled(selection.size() == 1);
            deleteButton.setEnabled(selection.size() > 0);
         }
      });

      RuleList.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editRule();
         }
      });

      return dialogArea;
   }

   /**
    * Delete selected Rules
    */
   private void deleteRules()
   {
      final IStructuredSelection selection = (IStructuredSelection)RuleList.getSelection();
      if (!selection.isEmpty())
      {
         Iterator<?> it = selection.iterator();
         while(it.hasNext())
         {
            rules.remove(it.next());
         }
         RuleList.setInput(rules.toArray());
      }
   }

   /**
    * Edit selected Rule
    */
   private void editRule()
   {
      final IStructuredSelection selection = (IStructuredSelection)RuleList.getSelection();
      if (selection.size() == 1)
      {
         final DCIImageRule rule = (DCIImageRule)selection.getFirstElement();
         EditDCIImageRuleDialog dlg = new EditDCIImageRuleDialog(getShell(), rule);
         if (dlg.open() == Window.OK)
         {
            RuleList.update(rule, null);
         }
      }
   }

   /**
    * Add new Rule
    */
   private void addRule()
   {
      
      DCIImageRule rule = new DCIImageRule();
      EditDCIImageRuleDialog dlg = new EditDCIImageRuleDialog(getShell(), rule);
      if (dlg.open() == Window.OK)
      {
         rules.add(rule);
         RuleList.setInput(rules.toArray());
         RuleList.setSelection(new StructuredSelection(rule));
      }
   }

   /**
    * Move currently selected Rule up
    */
   private void moveUp()
   {
      final IStructuredSelection selection = (IStructuredSelection)RuleList.getSelection();
      if (selection.size() == 1)
      {
         final DCIImageRule rule = (DCIImageRule)selection.getFirstElement();

         int index = rules.indexOf(rule);
         if (index > 0)
         {
            Collections.swap(rules, index - 1, index);
            RuleList.setInput(rules.toArray());
            RuleList.setSelection(new StructuredSelection(rule));
         }
      }
   }

   /**
    * Move currently selected Rule down
    */
   private void moveDown()
   {
      final IStructuredSelection selection = (IStructuredSelection)RuleList.getSelection();
      if (selection.size() == 1)
      {
         final DCIImageRule rule = (DCIImageRule)selection.getFirstElement();

         int index = rules.indexOf(rule);
         if ((index < rules.size() - 1) && (index >= 0))
         {
            Collections.swap(rules, index + 1, index);
            RuleList.setInput(rules.toArray());
            RuleList.setSelection(new StructuredSelection(rule));
         }
      }
   }

   /**
    * Setup Rule list control
    */
   private void setupRuleList()
   {
      Table table = RuleList.getTable();
      table.setLinesVisible(true);
      table.setHeaderVisible(true);

      TableColumn column = new TableColumn(table, SWT.LEFT);
      column.setText(Messages.get().DCIImageRuleList_Expression);
      column.setWidth(200);

      column = new TableColumn(table, SWT.LEFT);
      column.setText(Messages.get().DCIImageRuleList_Comment);
      column.setWidth(150);

      WidgetHelper.restoreColumnSettings(table, Activator.getDefault().getDialogSettings(), COLUMN_SETTINGS_PREFIX);

      RuleList.setContentProvider(new ArrayContentProvider());
      RuleList.setLabelProvider(new DCIImageRuleLabelProvider());
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.jface.preference.PreferencePage#performApply()
    */
   @Override
   protected void performApply()
   {
      saveSettings();
      applyChanges(true);
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
   @Override
   public boolean performOk()
   {
      saveSettings();
      applyChanges(false);
      return true;
   }

   /**
    * Apply changes
    * 
    * @param isApply true if update operation caused by "Apply" button
    */
   protected void applyChanges(final boolean isApply)
   {
      config.setDciRuleArray(rules.toArray(new DCIImageRule[rules.size()]));
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.jface.preference.PreferencePage#performCancel()
    */
   @Override
   public boolean performCancel()
   {
      saveSettings();
      return true;
   }

   /**
    * Save settings
    */
   private void saveSettings()
   {
      WidgetHelper.saveColumnSettings(RuleList.getTable(), Activator.getDefault().getDialogSettings(), COLUMN_SETTINGS_PREFIX);
   }
}
