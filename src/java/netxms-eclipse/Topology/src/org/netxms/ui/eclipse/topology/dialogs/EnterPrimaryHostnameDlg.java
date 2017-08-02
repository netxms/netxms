/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.topology.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.ui.eclipse.objectbrowser.widgets.ZoneSelector;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.topology.Messages;

/**
 * Dialog for requesting primary hostname from user
 */
public class EnterPrimaryHostnameDlg extends Dialog
{
   private Text hostnameText;
   private ZoneSelector zoneSelector;
   private String hostname;
   private long zoneId;
   private boolean zoningEnabled;

   /**
    * Constructor
    * 
    * @param parent Parent shell
    */
   public EnterPrimaryHostnameDlg(Shell parent)
   {
      super(parent);
      zoningEnabled = ConsoleSharedData.getSession().isZoningEnabled();
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText("Enter Hostname");
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      dialogArea.setLayout(layout);

      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      hostnameText = WidgetHelper.createLabeledText(dialogArea, SWT.BORDER, 300, "Enter hostname", "", gd);

      if (zoningEnabled)
      {
         zoneSelector = new ZoneSelector(dialogArea, SWT.NONE, false);
         zoneSelector.setLabel(Messages.get().EnterIpAddressDlg_Zone);
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         gd.widthHint = 300;
         zoneSelector.setLayoutData(gd);
      }
      
      return dialogArea;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      hostname = hostnameText.getText();
      zoneId = zoningEnabled ? zoneSelector.getZoneId() : 0;
      super.okPressed();
   }

   /**
    * @return the zoneObjectId
    */
   public long getZoneId()
   {
      return zoneId;
   }

   /**
    * @return the hostname
    */
   public String getHostname()
   {
      return hostname;
   }
}
