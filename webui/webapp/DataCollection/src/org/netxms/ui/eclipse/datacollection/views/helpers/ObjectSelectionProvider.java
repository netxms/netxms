/**
 * 
 */
package org.netxms.ui.eclipse.datacollection.views.helpers;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.StructuredSelection;
import org.netxms.client.NXCSession;
import org.netxms.client.TableRow;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Selection provider that transforms summary table row selection into object selection
 */
public class ObjectSelectionProvider implements ISelectionProvider
{
   private ISelectionProvider parent;
   private NXCSession session;
   
   /**
    * @param parent
    */
   public ObjectSelectionProvider(ISelectionProvider parent)
   {
      this.parent = parent;
      session = ConsoleSharedData.getSession();
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ISelectionProvider#addSelectionChangedListener(org.eclipse.jface.viewers.ISelectionChangedListener)
    */
   @Override
   public void addSelectionChangedListener(ISelectionChangedListener listener)
   {
      parent.addSelectionChangedListener(listener);
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ISelectionProvider#getSelection()
    */
   @Override
   public ISelection getSelection()
   {
      IStructuredSelection selection = (IStructuredSelection)parent.getSelection();
      if ((selection == null) || selection.isEmpty())
         return selection;
      
      List<AbstractObject> objects = new ArrayList<AbstractObject>(selection.size());
      for(Object o : selection.toList())
      {
         if (!(o instanceof TableRow))
            continue;
         
         AbstractObject object = session.findObjectById(((TableRow)o).getObjectId());
         if (object != null)
            objects.add(object);
      }
      
      return new StructuredSelection(objects);
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ISelectionProvider#removeSelectionChangedListener(org.eclipse.jface.viewers.ISelectionChangedListener)
    */
   @Override
   public void removeSelectionChangedListener(ISelectionChangedListener listener)
   {
      parent.removeSelectionChangedListener(listener);
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ISelectionProvider#setSelection(org.eclipse.jface.viewers.ISelection)
    */
   @Override
   public void setSelection(ISelection selection)
   {
   }
}
