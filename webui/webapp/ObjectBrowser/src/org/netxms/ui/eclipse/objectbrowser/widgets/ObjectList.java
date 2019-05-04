/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Raden Solutions
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
package org.netxms.ui.eclipse.objectbrowser.widgets;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ILabelProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
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
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.objectbrowser.Messages;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.tools.ObjectLabelComparator;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Object list widget
 */
public class ObjectList extends Composite
{      
   private TableViewer viewer;
   private HashMap<Long, AbstractObject> objects;
   private Button addButton;
   private Button deleteButton;

   /**
    * Create new object list widget
    *  
    * @param parent parent composite
    * @param style object list holding composite style
    * @param title List title (if set to null, no title will be displayed)
    * @param initialContent initial object list (can be null)
    * @param classFilter class filter for object selection dialog (can be null)
    * @param modifyListener modify listener (can be null)
    */
   public ObjectList(Composite parent, int style, String title, Collection<AbstractObject> initialContent,
         final Class<? extends AbstractObject> classFilter, final Runnable modifyListener)
   {
      super(parent, style);

      objects = new HashMap<Long, AbstractObject>();
      if (initialContent != null)
      {
         for(AbstractObject o : initialContent)
            objects.put(o.getObjectId(), o);
      }
      
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.INNER_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      setLayout(layout);
      
      if (title != null)
      {
         Label titleLabel = new Label(this, SWT.NONE);
         titleLabel.setText(title);
      }
      
      viewer = new TableViewer(this, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.getTable().setSortDirection(SWT.UP);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new WorkbenchLabelProvider());
      viewer.setComparator(new ObjectLabelComparator((ILabelProvider)viewer.getLabelProvider()));
      viewer.setInput(objects.values().toArray());
      
      GridData gd = new GridData();
      gd.verticalAlignment = GridData.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = GridData.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.heightHint = 0;
      viewer.getControl().setLayoutData(gd);
      
      Composite buttons = new Composite(this, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttons.setLayout(buttonLayout);
      gd = new GridData();
      gd.verticalIndent = WidgetHelper.OUTER_SPACING - WidgetHelper.INNER_SPACING;
      gd.horizontalAlignment = SWT.RIGHT;
      buttons.setLayoutData(gd);

      addButton = new Button(buttons, SWT.PUSH);
      addButton.setText(Messages.get().ObjectList_Add);
      addButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            ObjectSelectionDialog dlg = new ObjectSelectionDialog(getShell(), null, ObjectSelectionDialog.createNodeSelectionFilter(true));
            if (dlg.open() == Window.OK)
            {
               for (AbstractObject o :dlg.getSelectedObjects(classFilter))
                  objects.put(o.getObjectId(), o);
               viewer.setInput(objects.values().toArray());
               if (modifyListener != null)
                  modifyListener.run();
            }
         }
      });
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      addButton.setLayoutData(rd);
      
      deleteButton = new Button(buttons, SWT.PUSH);
      deleteButton.setText(Messages.get().ObjectList_Delete);
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
            for(Object o : selection.toList())
               objects.remove(((AbstractObject)o).getObjectId());
            viewer.setInput(objects.values().toArray());
            if (modifyListener != null)
               modifyListener.run();
         }
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      deleteButton.setLayoutData(rd);      
   }
   
   /**
    * Clear list
    */
   public void clear()
   {
      objects.clear();
      viewer.setInput(new AbstractObject[0]);
   }
   
   /**
    * Get selected object identifiers
    * 
    * @return selected object identifiers
    */
   public Long[] getObjectIdentifiers()
   {
      return objects.keySet().toArray(new Long[objects.size()]);
   }

   /**
    * Get selected objects
    * 
    * @return selected objects
    */
   public List<AbstractObject> getObjects()
   {
      return new ArrayList<AbstractObject>(objects.values());
   }
}
