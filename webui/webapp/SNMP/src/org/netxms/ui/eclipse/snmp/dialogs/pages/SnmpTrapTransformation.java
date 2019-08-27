/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.snmp.dialogs.pages;

import org.eclipse.jface.preference.PreferencePage;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.snmp.SnmpTrap;
import org.netxms.ui.eclipse.nxsl.widgets.ScriptEditor;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "Transformation" property page for SNMP trap configuration
 */
public class SnmpTrapTransformation extends PreferencePage
{
   private SnmpTrap trap;
   private ScriptEditor scriptEditor;

   /**
    * Create page
    * 
    * @param trap SNMP trap object to edit
    */
   public SnmpTrapTransformation(SnmpTrap trap)
   {
      super("Transformation");
      noDefaultAndApplyButton();
      this.trap = trap;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Composite dialogArea = new Composite(parent, SWT.NONE);
      
      GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      dialogArea.setLayout(layout);
      
      scriptEditor = new ScriptEditor(dialogArea, SWT.BORDER, SWT.NONE);
      scriptEditor.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      scriptEditor.setText(trap.getTransformationScript());

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
   @Override
   public boolean performOk()
   {
      if (isControlCreated())
      {
         trap.setTransformationScript(scriptEditor.getText());
      }
      return super.performOk();
   }
}
