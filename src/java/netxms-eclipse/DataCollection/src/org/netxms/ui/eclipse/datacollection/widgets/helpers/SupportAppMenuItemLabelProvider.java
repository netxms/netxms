package org.netxms.ui.eclipse.datacollection.widgets.helpers;

import org.eclipse.jface.viewers.IColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.netxms.ui.eclipse.datacollection.widgets.SupportAppPolicyEditor;

public class SupportAppMenuItemLabelProvider extends LabelProvider implements ITableLabelProvider, IColorProvider
{
   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      switch(columnIndex)
      {
         case SupportAppPolicyEditor.ICON:
            return ((GenericMenuItem)element).getIcon();
      }
      return null;
   }

   @Override
   public Color getForeground(Object element)
   {
      // TODO Auto-generated method stub
      return null;
   }

   @Override
   public Color getBackground(Object element)
   {
      // TODO Auto-generated method stub
      return null;
   }

   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      switch(columnIndex)
      {
         case SupportAppPolicyEditor.NAME:
            return ((GenericMenuItem)element).getName();
         case SupportAppPolicyEditor.DICPLAY_NAME:
            return ((GenericMenuItem)element).getDisplayName();
         case SupportAppPolicyEditor.COMMAND:
            return ((GenericMenuItem)element).getCommand();
      }
      return null;
   }

}
