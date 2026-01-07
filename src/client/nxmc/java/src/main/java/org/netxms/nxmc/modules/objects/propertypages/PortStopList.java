/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.PortStopEntry;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.Subnet;
import org.netxms.client.objects.Zone;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.dialogs.PortStopEntryDialog;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Port Stop List" property page for Zone, Container, Subnet, and Node objects
 */
public class PortStopList extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(PortStopList.class);

   private List<PortStopEntry> entries = new ArrayList<>();
   private TableViewer viewer;
   private Button addButton;
   private Button editButton;
   private Button deleteButton;
   private boolean isModified = false;

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public PortStopList(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(PortStopList.class).tr("Port Stop List"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "portStopList";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 40;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return (object instanceof Zone) || (object instanceof Container) ||
             (object instanceof Subnet) || (object instanceof Node);
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
      layout.numColumns = 2;
      dialogArea.setLayout(layout);

      viewer = new TableViewer(dialogArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      Table table = viewer.getTable();
      table.setHeaderVisible(true);
      table.setLinesVisible(true);

      TableColumn column = new TableColumn(table, SWT.LEFT);
      column.setText(i18n.tr("Port"));
      column.setWidth(100);

      column = new TableColumn(table, SWT.LEFT);
      column.setText(i18n.tr("Protocol"));
      column.setWidth(100);

      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new PortStopEntryLabelProvider());
      viewer.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            PortStopEntry p1 = (PortStopEntry)e1;
            PortStopEntry p2 = (PortStopEntry)e2;
            return Integer.compare(p1.getPort(), p2.getPort());
         }
      });

      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      viewer.getControl().setLayoutData(gd);

      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = viewer.getStructuredSelection();
            editButton.setEnabled(selection.size() == 1);
            deleteButton.setEnabled(!selection.isEmpty());
         }
      });

      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editEntry();
         }
      });

      Composite buttons = new Composite(dialogArea, SWT.NONE);
      GridLayout buttonsLayout = new GridLayout();
      buttonsLayout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      buttonsLayout.marginHeight = 0;
      buttons.setLayout(buttonsLayout);
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      buttons.setLayoutData(gd);

      addButton = new Button(buttons, SWT.PUSH);
      addButton.setText(i18n.tr("&Add..."));
      addButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            addEntry();
         }
      });
      gd = new GridData();
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      addButton.setLayoutData(gd);

      editButton = new Button(buttons, SWT.PUSH);
      editButton.setText(i18n.tr("&Edit..."));
      editButton.setEnabled(false);
      editButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            editEntry();
         }
      });
      gd = new GridData();
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      editButton.setLayoutData(gd);

      deleteButton = new Button(buttons, SWT.PUSH);
      deleteButton.setText(i18n.tr("&Delete"));
      deleteButton.setEnabled(false);
      deleteButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            deleteEntries();
         }
      });
      gd = new GridData();
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      deleteButton.setLayoutData(gd);

      entries.clear();
      entries.addAll(object.getPortStopList());
      viewer.setInput(entries.toArray());

      return dialogArea;
   }

   /**
    * Add new entry
    */
   private void addEntry()
   {
      PortStopEntryDialog dlg = new PortStopEntryDialog(getShell(), null);
      if (dlg.open() == Window.OK)
      {
         entries.add(dlg.getEntry());
         viewer.setInput(entries.toArray());
         isModified = true;
      }
   }

   /**
    * Edit selected entry
    */
   private void editEntry()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      PortStopEntry entry = (PortStopEntry)selection.getFirstElement();
      PortStopEntryDialog dlg = new PortStopEntryDialog(getShell(), entry);
      if (dlg.open() == Window.OK)
      {
         viewer.refresh();
         isModified = true;
      }
   }

   /**
    * Delete selected entries
    */
   private void deleteEntries()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      for(Object o : selection.toList())
      {
         entries.remove(o);
      }
      viewer.setInput(entries.toArray());
      isModified = true;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
   {
      if (!isModified)
         return true;

      if (isApply)
         setValid(false);

      NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
      md.setPortStopList(entries);
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating port stop list"), null, messageArea) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modifyObject(md);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update port stop list");
         }

         @Override
         protected void jobFinalize()
         {
            if (isApply)
               runInUIThread(() -> PortStopList.this.setValid(true));
         }
      }.start();
      return true;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
    */
   @Override
   protected void performDefaults()
   {
      super.performDefaults();
      entries.clear();
      viewer.setInput(entries.toArray());
      isModified = true;
   }

   /**
    * Label provider for port stop entries
    */
   private class PortStopEntryLabelProvider extends LabelProvider implements ITableLabelProvider
   {
      @Override
      public Image getColumnImage(Object element, int columnIndex)
      {
         return null;
      }

      @Override
      public String getColumnText(Object element, int columnIndex)
      {
         PortStopEntry entry = (PortStopEntry)element;
         switch(columnIndex)
         {
            case 0:
               return Integer.toString(entry.getPort());
            case 1:
               return entry.getProtocolString();
         }
         return "";
      }
   }
}
