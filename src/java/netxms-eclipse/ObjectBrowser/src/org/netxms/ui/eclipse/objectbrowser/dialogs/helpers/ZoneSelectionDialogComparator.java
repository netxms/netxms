package org.netxms.ui.eclipse.objectbrowser.dialogs.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.netxms.client.objects.AbstractObject;

/**
 * Zone comparator
 */
public class ZoneSelectionDialogComparator extends ViewerComparator
{
   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      int result = ((AbstractObject)e1).getObjectName().compareToIgnoreCase(((AbstractObject)e1).getObjectName());
      return result;
   }

}
