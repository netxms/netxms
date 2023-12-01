/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.nxsl.widgets;

import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Composite;
import org.netxms.nxmc.base.widgets.AbstractSelector;
import org.netxms.nxmc.base.widgets.helpers.SelectorConfigurator;
import org.netxms.nxmc.modules.nxsl.dialogs.SelectScriptDialog;

/**
 * Script selector
 */
public class ScriptSelector extends AbstractSelector
{
   private String scriptName = ""; //$NON-NLS-1$

   /**
    * @param parent
    * @param style
    * @param useHyperlink
    */
   public ScriptSelector(Composite parent, int style, boolean useHyperlink, boolean showLabel)
   {
      super(parent, style, new SelectorConfigurator()
            .setUseHyperlink(useHyperlink)
            .setShowLabel(showLabel));
   }

   /** 
    * @see org.netxms.ui.eclipse.widgets.AbstractSelector#selectionButtonHandler()
    */
   @Override
   protected void selectionButtonHandler()
   {
      SelectScriptDialog dlg = new SelectScriptDialog(getShell());
      if (dlg.open() == Window.OK)
      {
         scriptName = dlg.getScript().getName();
         setText(scriptName);
         fireModifyListeners();
      }
   }

   /**
    * Get name of selected script.
    *
    * @return name of selected script or empty string if script was not selected
    */
   public final String getScriptName()
   {
      return scriptName;
   }

   /**
    * Set script name.
    *
    * @param scriptName script name, empty string, or null
    */
   public final void setScriptName(String scriptName)
   {
      this.scriptName = (scriptName != null) ? scriptName : "";
      setText(this.scriptName);
   }
}
