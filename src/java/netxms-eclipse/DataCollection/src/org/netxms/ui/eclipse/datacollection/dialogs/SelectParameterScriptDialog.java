/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.datacollection.dialogs;

import org.eclipse.swt.widgets.Shell;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.ui.eclipse.nxsl.dialogs.SelectScriptDialog;

/**
 * Script selection dialog for use in data collection configuration
 */
public class SelectParameterScriptDialog extends SelectScriptDialog implements IParameterSelectionDialog
{
   /**
    * @param parentShell
    */
   public SelectParameterScriptDialog(Shell parentShell)
   {
      super(parentShell);
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.datacollection.dialogs.IParameterSelectionDialog#getParameterName()
    */
   @Override
   public String getParameterName()
   {
      return getScript().getName();
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.datacollection.dialogs.IParameterSelectionDialog#getParameterDescription()
    */
   @Override
   public String getParameterDescription()
   {
      return getScript().getName();
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.datacollection.dialogs.IParameterSelectionDialog#getParameterDataType()
    */
   @Override
   public int getParameterDataType()
   {
      return DataCollectionItem.DT_STRING;
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.datacollection.dialogs.IParameterSelectionDialog#getInstanceColumn()
    */
   @Override
   public String getInstanceColumn()
   {
      return null;
   }
}
