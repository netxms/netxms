package org.netxms.ui.eclipse.objectview.widgets.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.PhysicalLink;
import org.netxms.ui.eclipse.objectview.widgets.PhysicalLinkWidget;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Physical link comporator view
 */
public class PhysicalLinkComparator extends ViewerComparator
{
   private PhysicalLinkLabelProvider labelProvider;

   /*
    * Comparator constructor
    */
   public PhysicalLinkComparator(PhysicalLinkLabelProvider labelProvider)
   {
      this.labelProvider = labelProvider;
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      PhysicalLink link1 = (PhysicalLink)e1;
      PhysicalLink link2 = (PhysicalLink)e2;
      final int column = (Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID"); //$NON-NLS-1$

      int result;
      switch(column)
      {
         case PhysicalLinkWidget.PHYSICAL_LINK_ID:
            result = Long.compare(link1.getId(), link2.getId());
            break;
         case PhysicalLinkWidget.DESCRIPTOIN:
            result = link1.getDescription().compareToIgnoreCase(link2.getDescription());
            break;
         case PhysicalLinkWidget.LEFT_OBJECT:
         case PhysicalLinkWidget.RIGHT_OBJECT:
            result = labelProvider.getObjectText(link1, column == PhysicalLinkWidget.LEFT_OBJECT).compareToIgnoreCase(labelProvider.getObjectText(link2, column == PhysicalLinkWidget.LEFT_OBJECT));
            break;
         case PhysicalLinkWidget.LEFT_PORT:
         case PhysicalLinkWidget.RIGHT_PORT:
            result = labelProvider.getPortText(link1, column == PhysicalLinkWidget.LEFT_OBJECT).compareToIgnoreCase(labelProvider.getPortText(link2, column == PhysicalLinkWidget.LEFT_OBJECT));
            break;
         default:
            result = 0;
            break;
      }
      
      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
   }
   
}
