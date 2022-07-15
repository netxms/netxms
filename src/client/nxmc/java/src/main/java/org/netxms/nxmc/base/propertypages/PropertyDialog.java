/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.nxmc.base.propertypages;

import org.eclipse.jface.preference.IPreferenceNode;
import org.eclipse.jface.preference.IPreferencePage;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Shell;

/**
 * Property dialog
 */
public class PropertyDialog extends PreferenceDialog
{
   private String title;

   /**
    * @param parentShell
    * @param manager
    */
   public PropertyDialog(Shell parentShell, PreferenceManager manager, String title)
   {
      super(parentShell, manager);
      this.title = title;
      setBlockOnOpen(true);
   }

   /**
    * @see org.eclipse.jface.preference.PreferenceDialog#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(title);
   }

   /**
    * @see org.eclipse.jface.window.Window#open()
    */
   @Override
   public int open()
   {
      int result = super.open();
      if (result == Window.OK)
         return Window.OK;

      // Check if any of the pages was changed
      for(Object n : getPreferenceManager().getElements(PreferenceManager.PRE_ORDER))
      {
         if (n instanceof IPreferenceNode)
         {
            IPreferencePage page = ((IPreferenceNode)n).getPage();
            if ((page != null) && (page instanceof PropertyPage) && ((PropertyPage)page).isChanged())
               return Window.OK;
         }
      }
      return result;
   }

   /**
    * Show property page by path.
    *
    * @param path path to property page (usually dot separated if not overriden by preference manager)
    */
   public void showPage(String path)
   {
      IPreferenceNode n = getPreferenceManager().find(path);
      if (n != null)
      {
         if (showPage(n))
         {
            getTreeViewer().setSelection(new StructuredSelection(n));
         }
      }
   }
}
