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
package org.netxms.ui.eclipse.datacollection.dialogs.helpers;

import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.ComboBoxCellEditor;
import org.eclipse.jface.viewers.EditingSupport;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.TextCellEditor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Text;

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

   @Override
   protected CellEditor getCellEditor(Object element)
   {
      BulkUpdateElementUI el = (BulkUpdateElementUI)element;
      CellEditor editor = null;
      if (el.isText())
      {
         editor = new TextCellEditor(viewer.getTable());
      }
      else
      {
         editor = new ComboBoxCellEditor(viewer.getTable(), el.getPossibleValues(), SWT.READ_ONLY);
      }
      
      if (el.getVerifyListener() != null)
         ((Text)editor.getControl()).addVerifyListener(el.getVerifyListener());
      
      return editor;
   }

   @Override
   protected boolean canEdit(Object element)
   {
      return ((BulkUpdateElementUI)element).isEditable();
   }

   @Override
   protected Object getValue(Object element)
   {
      BulkUpdateElementUI el = (BulkUpdateElementUI)element;

      if (el.isText())
      {
         return el.getTextValue();
      }
      else
      {
         return el.getSelectionValue() + 1;         
      }
   }

   @Override
   protected void setValue(Object element, Object value)
   {
      BulkUpdateElementUI el = (BulkUpdateElementUI)element;
      el.setValue(el.isText() ? value : ((Integer)value) - 1);
      getViewer().update(el, null);
   }

}
