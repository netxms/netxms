/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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
package org.netxms.nxmc.tools;

import org.eclipse.jface.window.Window;

/**
 * State data for dialogs with multiple options
 */
public class DialogData
{
   private boolean okPressed;
   private boolean saveSelection;
   
   /**
    * @param dlgReturn code
    * @param saveSelection
    */
   public DialogData(int dlgReturn, boolean saveSelection)
   {
      if (dlgReturn == Window.OK)
         okPressed = true;
      else
         okPressed = false;
      
      this.saveSelection = saveSelection;
   }
   
   /**
    * @return was OK pressed
    */
   public boolean isOkPressed()
   {
      return okPressed;
   }
   
   /**
    * @return if selection should be saved
    */
   public boolean getSaveSelection()
   {
      return saveSelection;
   }
}
