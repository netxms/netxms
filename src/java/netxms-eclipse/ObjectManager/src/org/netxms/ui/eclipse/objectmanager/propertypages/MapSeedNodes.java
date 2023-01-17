/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
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
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.NetworkMap;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Map seed nodes property page
 */
public class MapSeedNodes extends PropertyPage
{
   public static final int COLUMN_NAME = 0;
   private AbstractObject object = null;
   private TableViewer viewer;
   private Button addButton;
   private Button deleteButton;
   private Set<AbstractObject> seedObjects;
   private boolean isModified = false;

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Composite dialogArea = new Composite(parent, SWT.NONE);
      object = (AbstractObject)getElement().getAdapter(AbstractObject.class);
      if (object == null || !(object instanceof NetworkMap))
         return dialogArea;

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      viewer = new TableViewer(dialogArea, SWT.BORDER);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new WorkbenchLabelProvider());

      NXCSession session = ConsoleSharedData.getSession();
      seedObjects = new HashSet<AbstractObject>(session.findMultipleObjects(((NetworkMap)object).getSeedObjects(), false));
      viewer.setInput(seedObjects);

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
            final ObjectSelectionDialog dlg = new ObjectSelectionDialog(getShell(), ObjectSelectionDialog.createNodeSelectionFilter(false));
            dlg.enableMultiSelection(true);
            if (dlg.open() == Window.OK)
            {
               List<AbstractObject> selected = dlg.getSelectedObjects();
               if (selected.size() > 0)
               {
                  seedObjects.addAll(selected);
                  viewer.setInput(seedObjects);
                  isModified = true;
               }                  
            }
         }
      });

      deleteButton = new Button(buttons, SWT.PUSH);
      deleteButton.setText("&Delete");
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      deleteButton.setLayoutData(rd);
      deleteButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            IStructuredSelection selection = viewer.getStructuredSelection();
            if (selection.size() > 0)
            {
               seedObjects.removeAll(selection.toList());
               viewer.setInput(seedObjects);
               isModified = true;
            }
         }
      });      

      return dialogArea;
   }
   
   /**
    * Apply changes
    * @param isApply true if update operation caused by "Apply" button
    */
   protected void applyChanges(final boolean isApply)
   {
      if (!isModified)
         return;     // Nothing to apply

      if (isApply)
         setValid(false);

      final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
      List<Long> seedObjectIds = new ArrayList<Long>();
      for(AbstractObject o : seedObjects)
         seedObjectIds.add(o.getObjectId());      
      md.setSeedObjectIds(seedObjectIds);

      final NXCSession session = ConsoleSharedData.getSession();
      new ConsoleJob("Update network map seed nodes", null, Activator.PLUGIN_ID) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.modifyObject(md);
            isModified = false;
         }

         @Override
         protected String getErrorMessage()
         {
            return "Network map seed node update failed";
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
                     MapSeedNodes.this.setValid(true);
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
