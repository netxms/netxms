/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
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
import org.eclipse.swt.widgets.Event;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.configs.RackPassiveElementConfig;
import org.netxms.client.objects.configs.RackPassiveElementConfigEntry;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.objectmanager.dialogs.RackPassiveElementEditDialog;
import org.netxms.ui.eclipse.objectmanager.propertypages.helpers.RackPassiveElementComparator;
import org.netxms.ui.eclipse.objectmanager.propertypages.helpers.RackPassiveElementLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * "Rack Attributes" property page for rack object
 */
public class RackPassiveElements extends PropertyPage
{
   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_TYPE = 1;
   public static final int COLUMN_POSITION = 2;
   public static final int COLUMN_ORIENTATION = 3;
   
   private Rack rack = null;
   private SortableTableViewer viewer;
   private Button addButton;
   private Button editButton;
   private Button deleteButton;
   private RackPassiveElementConfig passiveElementConfig;
   private boolean isModified = false;

   /* (non-Javadoc)
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Composite dialogArea = new Composite(parent, SWT.NONE);
      
      rack = (Rack)getElement().getAdapter(Rack.class);
      if (rack == null)  // Paranoid check
         return dialogArea;
      
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      dialogArea.setLayout(layout);
      
      final String[] columnNames = { "Name", "Type", "Position", "Orientation" };
      final int[] columnWidths = { 150, 100, 70, 30 };
      viewer = new SortableTableViewer(dialogArea, columnNames, columnWidths, 0, SWT.UP,
                                       SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new RackPassiveElementLabelProvider());
      viewer.setComparator(new RackPassiveElementComparator());
      
      passiveElementConfig = new RackPassiveElementConfig(rack.getPassiveElementConfig());
      viewer.setInput(passiveElementConfig.getEntryList());

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
      addButton.setText("&Add...");
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
            RackPassiveElementEditDialog dlg = new RackPassiveElementEditDialog(getShell(), null);
            if (dlg.open() == Window.OK)
            {
               passiveElementConfig.addEntry(dlg.getPassiveElement());
               viewer.setInput(passiveElementConfig.getEntryList());
               isModified = true;
            }
         }
      });
      
      editButton = new Button(buttons, SWT.PUSH);
      editButton.setText("&Edit...");
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
            IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
            if (selection.size() == 1)
            {
               RackPassiveElementEditDialog dlg = new RackPassiveElementEditDialog(getShell(), (RackPassiveElementConfigEntry)selection.getFirstElement());
               if (dlg.open() == Window.OK)
               {
                  viewer.setInput(passiveElementConfig.getEntryList());
                  isModified = true;
               }
            }
         }
      });
      
      deleteButton = new Button(buttons, SWT.PUSH);
      deleteButton.setText("&Delete...");
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
            IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
            passiveElementConfig.getEntryList().removeAll(selection.toList());
            viewer.setInput(passiveElementConfig.getEntryList());
            isModified = true;
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
            editButton.setEnabled(selection.size() == 1);
            deleteButton.setEnabled(selection.size() > 0);
         }
      });
      
      return dialogArea;
   }

   /**
    * Apply changes
    * 
    * @param isApply true if update operation caused by "Apply" button
    */
   protected void applyChanges(final boolean isApply)
   {
      if (!isModified)
         return;     // Nothing to apply
      
      if (isApply)
         setValid(false);
      
      final NXCObjectModificationData md = new NXCObjectModificationData(rack.getObjectId());
      try
      {
         md.setPassiveElementsConfig(passiveElementConfig.createXml());
      }
      catch(Exception e)
      {
         MessageDialogHelper.openError(getShell(), "Rack attribute error", "Unable to save rack attribute configuration" + e.getMessage());
      }
      final NXCSession session = ConsoleSharedData.getSession();
      new ConsoleJob("Update rack attributes", null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.modifyObject(md);
            isModified = false;
         }

         @Override
         protected String getErrorMessage()
         {
            return "Rack attribute update error";
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
                     isModified = true;
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
