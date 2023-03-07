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
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.asset.AssetManagementAttribute;
import org.netxms.client.constants.AMDataType;
import org.netxms.client.constants.AMSystemType;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.serverconfig.views.helpers.AssetManagementAttributeLabelProvider;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Asset management attribute general editor page
 */
public class AssetAttributeGeneral extends PropertyPage
{
   final static I18n i18n = LocalizationHelper.getI18n(AssetAttributeGeneral.class);
   
   AssetManagementAttribute attr = null;
   boolean createNew = false;

   LabeledText textName;
   LabeledText textDisplayName;
   Combo comboDataType = null;
   Button buttonMandatory;
   Button buttonUnique;
   LabeledSpinner spinnerRangeMin;
   LabeledSpinner spinnerRangeMax;
   Combo comboSystemType;   
   

   /**
    * Page constructor
    * 
    * @param attr attribute to edit
    */
   public AssetAttributeGeneral(AssetManagementAttribute attr, boolean createNew)
   {
      super("General");
      this.attr = attr;
      this.createNew = createNew;
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
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.numColumns = 2;    
      dialogArea.setLayout(layout);
      
      textName = new LabeledText(dialogArea, SWT.NONE);
      textName.setLabel("Name");
      GridData gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.horizontalSpan = 2;
      textName.setLayoutData(gd);
      textName.setText(attr.getName());
      textName.setEditable(createNew);
      
      textDisplayName = new LabeledText(dialogArea, SWT.NONE);
      textDisplayName.setLabel("Display name");
      gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.horizontalSpan = 2;
      textDisplayName.setLayoutData(gd);
      textDisplayName.setText(attr.getDisplayName());
      
      comboDataType = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, i18n.tr("Data type"), new GridData(SWT.FILL, SWT.CENTER, true, false));
      for (String s : AssetManagementAttributeLabelProvider.DATA_TYPES)
      {
         comboDataType.add(s);
      }
      comboDataType.select(attr.getDataType().getValue());
      
      comboDataType.addModifyListener(new ModifyListener() {
         
         @Override
         public void modifyText(ModifyEvent e)
         {
            AMDataType seleciton = AMDataType.getByValue(comboDataType.getSelectionIndex());
            boolean haveLimits = seleciton == AMDataType.STRING || seleciton == AMDataType.NUMBER || seleciton == AMDataType.INTEGER;
            spinnerRangeMin.setEnabled(haveLimits);
            spinnerRangeMax.setEnabled(haveLimits);       
            spinnerRangeMax.setLabel((attr.getDataType() == AMDataType.STRING) ? i18n.tr("Maximum lenghts") : i18n.tr("Minimum value"));
            spinnerRangeMin.setLabel((attr.getDataType() == AMDataType.STRING) ? i18n.tr("Minimum lenghts") : i18n.tr("Minimum value"));
            //TODO: add enable disable of enum editor callback?
         }
      });

      comboSystemType = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, i18n.tr("System type"), new GridData(SWT.FILL, SWT.CENTER, true, false));
      for (String s : AssetManagementAttributeLabelProvider.SYSTEM_TYPE)
      {
         comboSystemType.add(s);
      }
      comboSystemType.select(attr.getSystemType().getValue());

      spinnerRangeMin = new LabeledSpinner(dialogArea, SWT.NONE);
      spinnerRangeMin.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      spinnerRangeMin.setLabel((attr.getDataType() == AMDataType.STRING) ? i18n.tr("Minimum lenghts") : i18n.tr("Minimum value"));
      spinnerRangeMin.setSelection(attr.getRangeMin());
      
      spinnerRangeMax = new LabeledSpinner(dialogArea, SWT.NONE);
      spinnerRangeMax.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      spinnerRangeMax.setLabel((attr.getDataType() == AMDataType.STRING) ? i18n.tr("Maximum lenghts") : i18n.tr("Minimum value"));
      spinnerRangeMax.setSelection(attr.getRangeMax());
      
      boolean haveLimits = (attr.getDataType() == AMDataType.STRING || attr.getDataType() == AMDataType.NUMBER || attr.getDataType() == AMDataType.INTEGER);
      spinnerRangeMin.setEnabled(haveLimits);
      spinnerRangeMax.setEnabled(haveLimits);
      
      Composite checkContainer = new Composite(dialogArea, SWT.NONE);
      checkContainer.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      checkContainer.setLayout(new GridLayout());
      
      buttonMandatory = new Button(checkContainer, SWT.CHECK);
      buttonMandatory.setText(i18n.tr("Is mandatory"));
      buttonMandatory.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      buttonMandatory.setSelection(attr.isMandatory());
      
      buttonUnique = new Button(checkContainer, SWT.CHECK);
      buttonUnique.setText(i18n.tr("Is unique"));
      buttonUnique.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      buttonUnique.setSelection(attr.isUnique());
      
      return dialogArea;
   }

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
   {
      if (spinnerRangeMin.getSelection() > spinnerRangeMax.getSelection())
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Minumun can't be lesst than maximum"));
         return false;
      }
      
      if (createNew)
      {
         String newName = textName.getText();
         if (newName.isEmpty())
         {            
            MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Name can't be empty"));
            return false;
         }
         if (!Registry.getSession().isAssetAttributeUnique(newName))
         { 
            MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Name should be unique"));
            return false;            
         }
         attr.setName(textName.getText());
      }
      attr.setDisplayName(textDisplayName.getText());
      attr.setDataType(AMDataType.getByValue(comboDataType.getSelectionIndex()));
      attr.setSystemType(AMSystemType.getByValue(comboSystemType.getSelectionIndex()));
      attr.setRangeMin(spinnerRangeMin.getSelection());
      attr.setRangeMax(spinnerRangeMax.getSelection());
      attr.setMandatory(buttonMandatory.getSelection());
      attr.setUnique(buttonUnique.getSelection());
      
      return true;
   }
}
