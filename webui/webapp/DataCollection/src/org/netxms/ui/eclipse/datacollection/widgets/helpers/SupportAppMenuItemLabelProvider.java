package org.netxms.ui.eclipse.datacollection.widgets.helpers;

import org.eclipse.jface.viewers.IColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.netxms.ui.eclipse.datacollection.widgets.SupportAppPolicyEditor;

/**
 * Label provider for support application menu list
 */
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
            return ((AppMenuItem)element).getIcon();
      }
      return null;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.IColorProvider#getForeground(java.lang.Object)
    */
   @Override
   public Color getForeground(Object element)
   {
      // TODO Auto-generated method stub
      return null;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.IColorProvider#getBackground(java.lang.Object)
    */
   @Override
   public Color getBackground(Object element)
   {
      // TODO Auto-generated method stub
      return null;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      switch(columnIndex)
      {
         case SupportAppPolicyEditor.NAME:
            return ((AppMenuItem)element).getName();
         case SupportAppPolicyEditor.DISCRIPTION:
            return ((AppMenuItem)element).getDescription();
         case SupportAppPolicyEditor.COMMAND:
            return ((AppMenuItem)element).getCommand();
      }
      return null;
   }
}
