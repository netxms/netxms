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
package org.netxms.nxmc.modules.serverconfig.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.asset.AssetManagementAttribute;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.base.widgets.KeyValueSetEditor;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Asset management attribute enum configuration
 */
public class AssetAttributeEnums extends PropertyPage
{
   final static I18n i18n = LocalizationHelper.getI18n(AssetAttributeEnums.class);
   
   AssetManagementAttribute attr = null;
   KeyValueSetEditor enumEditor;

   public AssetAttributeEnums(AssetManagementAttribute attr)
   {
      super("Enum configuration");
      this.attr = attr;
      noDefaultAndApplyButton();
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {    
      Composite dialogArea = new Composite(parent, SWT.NONE);

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      dialogArea.setLayout(layout);
      
      enumEditor = new KeyValueSetEditor(dialogArea, SWT.NONE, i18n.tr("Name"));
      GridData gd = new GridData(SWT.FILL, SWT.FILL, true, true);
      gd.horizontalSpan = 2;
      gd.heightHint = 150;
      gd.verticalIndent = WidgetHelper.OUTER_SPACING * 2;
      enumEditor.setLayoutData(gd);
      enumEditor.addAll(attr.getEnumMapping());
      //TODO: enumEditor.setEnabled(attr.getDataType() == AMDataType.ENUM);
      
      return dialogArea;
   }

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
   {
      attr.setEnumMapping(enumEditor.getContent());
      return true;
   }

}
