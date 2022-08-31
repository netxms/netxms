/**
 * NetXMS - open source network management system
 * Copyright (C) 2020 Raden Solutions
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
package org.netxms.nxmc.modules.datacollection.dialogs.helpers;

import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.ComboBoxCellEditor;
import org.eclipse.jface.viewers.EditingSupport;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.TextCellEditor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Text;
import org.netxms.nxmc.base.widgets.EditableComboBoxCellEditor;

/**
 * Edit support class for bulk update value field
 */
public class BulkValueEditSupport extends EditingSupport
{
   TableViewer viewer;

   /**
    * Constructor 
    * 
    * @param viewer viewer
    * @param isCustomIntervalOrigin true if all DCIs have custom retention time
    * @param isCustomRetentionOrigin true if all DCIs have custom interval
    */
   public BulkValueEditSupport(TableViewer viewer)
   {
      super(viewer);
      this.viewer = viewer;
   }

   /**
    * @see org.eclipse.jface.viewers.EditingSupport#getCellEditor(java.lang.Object)
    */
   @Override
   protected CellEditor getCellEditor(Object element)
   {
      BulkDciUpdateElementUI el = (BulkDciUpdateElementUI)element;
      CellEditor editor = null;
      if (el.isText())
      {
         editor = new TextCellEditor(viewer.getTable());
      }
      else if (el.isEditableDropdown())
      {
         editor = new EditableComboBoxCellEditor(viewer.getTable(), el.getPossibleValues());
      }
      else
      {
         editor = new ComboBoxCellEditor(viewer.getTable(), el.getPossibleValues(), SWT.READ_ONLY);
      }

      if (el.getVerifyListener() != null)
         ((Text)editor.getControl()).addVerifyListener(el.getVerifyListener());

      return editor;
   }

   /**
    * @see org.eclipse.jface.viewers.EditingSupport#canEdit(java.lang.Object)
    */
   @Override
   protected boolean canEdit(Object element)
   {
      return ((BulkDciUpdateElementUI)element).isEditable();
   }

   /**
    * @see org.eclipse.jface.viewers.EditingSupport#getValue(java.lang.Object)
    */
   @Override
   protected Object getValue(Object element)
   {
      BulkDciUpdateElementUI el = (BulkDciUpdateElementUI)element;
      return el.getValue();
   }

   /**
    * @see org.eclipse.jface.viewers.EditingSupport#setValue(java.lang.Object, java.lang.Object)
    */
   @Override
   protected void setValue(Object element, Object value)
   {
      BulkDciUpdateElementUI el = (BulkDciUpdateElementUI)element;
      el.setValue(value);
      getViewer().update(el, null);
   }
}
