/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.configs.PassiveRackElement;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.dialogs.RackPassiveElementEditDialog;
import org.netxms.nxmc.modules.objects.propertypages.helpers.RackPassiveElementComparator;
import org.netxms.nxmc.modules.objects.propertypages.helpers.RackPassiveElementLabelProvider;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * "Rack passive elements" property page for rack object
 */
public class RackPassiveElements extends ObjectPropertyPage
{
   public static final int COLUMN_NAME = 0;
   public static final int COLUMN_TYPE = 1;
   public static final int COLUMN_POSITION = 2;
   public static final int COLUMN_HEIGHT = 3;
   public static final int COLUMN_ORIENTATION = 4;

   private Rack rack = null;
   private SortableTableViewer viewer;
   private Button addButton;
   private Button editButton;
   private Button deleteButton;
   private List<PassiveRackElement> passiveElements;
   private boolean isModified = false;

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public RackPassiveElements(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(RackPassiveElements.class).tr("Passive Elements"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "rackPassiveElements";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 2;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return (object instanceof Rack);
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Composite dialogArea = new Composite(parent, SWT.NONE);

      rack = (Rack)object;
      
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      final String[] columnNames = { "Name", "Type", "Position", "Height", "Orientation" };
      final int[] columnWidths = { 150, 100, 70, 70, 80 };
      viewer = new SortableTableViewer(dialogArea, columnNames, columnWidths, 0, SWT.UP, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new RackPassiveElementLabelProvider());
      viewer.setComparator(new RackPassiveElementComparator());

      passiveElements = new ArrayList<PassiveRackElement>();
      for(PassiveRackElement element : rack.getPassiveElements())
         passiveElements.add(new PassiveRackElement(element));
      viewer.setInput(passiveElements);

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
      addButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            RackPassiveElementEditDialog dlg = new RackPassiveElementEditDialog(getShell(), null);
            if (dlg.open() == Window.OK)
            {
               passiveElements.add(dlg.getElement());
               viewer.setInput(passiveElements);
               isModified = true;
            }
         }
      });

      editButton = new Button(buttons, SWT.PUSH);
      editButton.setText("&Edit...");
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      editButton.setLayoutData(rd);
      editButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
            if (selection.size() == 1)
            {
               RackPassiveElementEditDialog dlg = new RackPassiveElementEditDialog(getShell(), (PassiveRackElement)selection.getFirstElement());
               if (dlg.open() == Window.OK)
               {
                  viewer.setInput(passiveElements);
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
      deleteButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
            passiveElements.removeAll(selection.toList());
            viewer.setInput(passiveElements);
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
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
   {
      if (!isModified)
         return true; // Nothing to apply

      if (isApply)
         setValid(false);

      final NXCObjectModificationData md = new NXCObjectModificationData(rack.getObjectId());
      md.setPassiveElements(passiveElements);
      final NXCSession session = Registry.getSession();
      new Job("Updating list of passive rack elements", null, messageArea) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modifyObject(md);
            isModified = false;
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot update list of passive rack elements";
         }

         @Override
         protected void jobFinalize()
         {
            if (isApply)
               runInUIThread(() -> RackPassiveElements.this.setValid(true));
         }
      }.start();
      return true;
   }
}
