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
package org.netxms.nxmc.modules.objects.actions;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewPlacement;
import org.netxms.nxmc.base.widgets.MessageAreaHolder;

/**
 * Base class for actions intended to be run on objects.
 *
 * @param <T> class to execute action on
 */
public abstract class ObjectAction<T extends AbstractObject> extends Action
{
   private Class<T> objectClass;
   private ViewPlacement viewPlacement;
   private ISelectionProvider selectionProvider;

   /**
    * Create action for creating interface DCIs.
    *
    * @param text action's text
    * @param perspective owning view
    * @param selectionProvider associated selection provider
    */
   public ObjectAction(final Class<T> objectClass, String text, ViewPlacement viewPlacement, ISelectionProvider selectionProvider)
   {
      super(text);
      this.viewPlacement = viewPlacement;
      this.selectionProvider = selectionProvider;
      this.objectClass = objectClass;
   }

   /**
    * Check if action is valid for given selection of objects. Only called if multiple objects are selected. Default implementation
    * returns true if all elements in selection are instances of class passed to constructor.
    *
    * @param selection selection to test
    * @return true if action is valid for given selection
    */
   public boolean isValidForSelection(IStructuredSelection selection)
   {
      if (selection.isEmpty())
         return false;

      for(Object e : selection.toList())
         if (!objectClass.isInstance(e))
            return false;

      return true;
   }

   /**
    * @see org.eclipse.jface.action.Action#run()
    */
   @Override
   @SuppressWarnings("unchecked")
   public final void run()
   {     
      final List<T> objects = new ArrayList<>();
      for(Object o : ((IStructuredSelection)selectionProvider.getSelection()).toList())
      {         
         if (objectClass.isInstance(o))
            objects.add((T)o);
      }
      if (!objects.isEmpty())
      {
         run(objects);
      }
   }

   /**
    * Execute action on provided objects
    * 
    * @param objects object list
    */
   protected abstract void run(List<T> objects);

   /**
    * Get message area that can be used by this action.
    *
    * @return message area
    */
   protected MessageAreaHolder getMessageArea()
   {
      return viewPlacement.getMessageAreaHolder();
   }

   /**
    * Get shell that can be used as parent for dialogs, etc.
    *
    * @return shell
    */
   protected Shell getShell()
   {
      return viewPlacement.getWindow().getShell();
   }

   /**
    * Open view using view placement information provided during construction.
    *
    * @param view view to open
    */
   protected void openView(View view)
   {
      viewPlacement.openView(view);
   }
}
