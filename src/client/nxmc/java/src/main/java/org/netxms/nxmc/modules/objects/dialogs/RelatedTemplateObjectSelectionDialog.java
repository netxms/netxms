/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Reden Solutions
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
package org.netxms.nxmc.modules.objects.dialogs;

import java.util.Set;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog used to select related object(s) of given template for removal 
 */
public class RelatedTemplateObjectSelectionDialog extends RelatedObjectSelectionDialog
{
   private final I18n i18n = LocalizationHelper.getI18n(RelatedTemplateObjectSelectionDialog.class);
   
   private boolean removeDci;
   private Button radioRemove; 
   private Button radioUnbind;
   
   /**
    * Constructor
    * 
    * @param parentShell parent shell
    * @param seedObject object used for populating related object list
    * @param relationType relation type
    * @param classFilter class filter for object list
    */
   public RelatedTemplateObjectSelectionDialog(Shell parentShell, long seedObject, RelationType relationType,
         Set<Integer> classFilter)
   {
      super(parentShell, seedObject, relationType, classFilter);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.dialogs.RelatedObjectSelectionDialog#additionalRemovalOptions(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void additionalRemovalOptions(Composite parent)
   {
      Composite selectionArea = new Composite(parent, SWT.NONE);
      
      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.horizontalSpacing = 0;
      selectionArea.setLayout(layout);
      
      final Label label = new Label(selectionArea, SWT.WRAP);
      label.setText(i18n.tr("You are about to remove data collection template from a node. Please select how to deal with DCIs related to this template:"));
      GridData gd = new GridData();
      gd.widthHint = 300;
      label.setLayoutData(gd);
      
      radioRemove = new Button(selectionArea, SWT.RADIO);
      radioRemove.setText(i18n.tr("&Remove DCIs from node"));
      radioRemove.setSelection(true);
      
      radioUnbind = new Button(selectionArea, SWT.RADIO);
      radioUnbind.setText(i18n.tr("&Unbind DCIs from template"));
      radioUnbind.setSelection(false);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.dialogs.RelatedObjectSelectionDialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      removeDci = radioRemove.getSelection();
      super.okPressed();
   }

   /**
    * If DCIs should be removed on template removal
    * 
    * @return if DCIs should be removed on template removal
    */
   public boolean isRemoveDci()
   {
      return removeDci;
   }

}
