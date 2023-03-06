package org.netxms.nxmc.base.actions;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.netxms.nxmc.base.views.Perspective;
/**
 * @author zev 
 *
 * @param <T> class to exewcute action on
 */
public abstract class ObjectAction<T> extends Action
{
   private ISelectionProvider selectionProvider;
   private Class<T> objectClass;
   protected Perspective perspective;

   /**
    * Create action for creating interface DCIs.
    *
    * @param text action's text
    * @param perspective owning view
    * @param selectionProvider associated selection provider
    */
   public ObjectAction(final Class<T> objectClass, String text, Perspective perspective, ISelectionProvider selectionProvider)
   {
      super(text);
      this.perspective = perspective;
      this.selectionProvider = selectionProvider;
      this.objectClass = objectClass;
   }

   /**
    * @see org.eclipse.jface.action.Action#run()
    */
   @Override
   @SuppressWarnings("unchecked")
   public void run()
   {     
      final List<T> objects = new ArrayList<>();
      for(Object o : ((IStructuredSelection)selectionProvider.getSelection()).toList())
      {         
         if (objectClass.isInstance(o))
            objects.add((T)o);
      }
      if (objects.isEmpty())
         return;

      runInternal(objects);
   }
   
   /**
    * Execute action on provided objects
    * 
    * @param objects object list
    */
   protected abstract void runInternal(List<T> objects);   
}
