/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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
package org.netxms.nxmc.base.views;

import org.eclipse.jface.window.Window;
import org.netxms.nxmc.base.widgets.MessageAreaHolder;
import org.netxms.nxmc.base.windows.PopOutViewWindow;

/**
 * View placement - window and perspective combination. Perspective can be null for pop-out views.
 */
public class ViewPlacement
{
   private Perspective perspective;
   private Window window;

   /**
    * Create view placement in given perspective.
    *
    * @param perspective perspective to place view in
    */
   public ViewPlacement(Perspective perspective)
   {
      this.perspective = perspective;
      this.window = perspective.getWindow();
   }

   /**
    * Create view placement in given pop-out window.
    *
    * @param window pop-out window
    */
   public ViewPlacement(Window window)
   {
      this.perspective = null;
      this.window = window;
   }

   /**
    * Create view placement identical to that of given view.
    *
    * @param view view to get placement information from
    */
   public ViewPlacement(View view)
   {
      this.perspective = view.getPerspective();
      this.window = view.getWindow();
   }

   /**
    * Open view according to this placement
    *
    * @param view view to open
    */
   public void openView(View view)
   {
      if (perspective != null)
         perspective.addMainView(view, true);
      else
         PopOutViewWindow.open(view);
   }

   /**
    * Get perspective.
    *
    * @return perspective or null
    */
   public Perspective getPerspective()
   {
      return perspective;
   }

   /**
    * Get window.
    *
    * @return window holding view
    */
   public Window getWindow()
   {
      return window;
   }

   /**
    * Check if view location describes pop-out view.
    *
    * @return true if view location describes pop-out view
    */
   public boolean isPopOutView()
   {
      return perspective == null;
   }

   /**
    * Get message area holder.
    *
    * @return message area holder
    */
   public MessageAreaHolder getMessageAreaHolder()
   {
      return (perspective != null) ? perspective.getMessageArea() : (MessageAreaHolder)window;
   }
}
