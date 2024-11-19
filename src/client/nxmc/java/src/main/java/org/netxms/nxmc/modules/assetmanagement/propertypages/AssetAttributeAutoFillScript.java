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
package org.netxms.nxmc.modules.assetmanagement.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.asset.AssetAttribute;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.nxsl.widgets.ScriptEditor;
import org.xnap.commons.i18n.I18n;

/**
 * "Auto Fill Script" property page for asset attribute
 */
public class AssetAttributeAutoFillScript extends PropertyPage
{
   final I18n i18n = LocalizationHelper.getI18n(AssetAttributeAutoFillScript.class);

   AssetAttribute attribute;
   ScriptEditor script;

   public AssetAttributeAutoFillScript(AssetAttribute attribute)
   {
      super(LocalizationHelper.getI18n(AssetAttributeAutoFillScript.class).tr("Auto Fill Script"));
      this.attribute = attribute;
      noDefaultAndApplyButton();
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Composite dialogArea = new Composite(parent, SWT.NONE);
      dialogArea.setLayout(new FillLayout());

      script = new ScriptEditor(dialogArea, SWT.BORDER, SWT.H_SCROLL | SWT.V_SCROLL,  
            i18n.tr("Variables:\n\t$object\tobject associated with this asset\n\t$node\tnode associated with this asset or null\n\t$asset\tcurrent asset object\n\t$name\tupdated property name\n\t$value\tcurrent property value\n\t$enumValues\tarray with possible enum values\n\nReturn value: new value for asset property"));
      script.setText(attribute.getAutofillScript());
      return dialogArea;
   }

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
   {
      attribute.setAutofillScript(script.getText());
      return true;
   }
}
