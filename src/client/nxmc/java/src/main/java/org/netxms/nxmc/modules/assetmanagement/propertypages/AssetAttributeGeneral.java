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
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.asset.AssetAttribute;
import org.netxms.client.constants.AMDataType;
import org.netxms.client.constants.AMSystemType;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.assetmanagement.views.helpers.AssetAttributeListLabelProvider;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Asset management attribute general editor page
 */
public class AssetAttributeGeneral extends PropertyPage
{
   final I18n i18n = LocalizationHelper.getI18n(AssetAttributeGeneral.class);

   private AssetAttribute attribute = null;
   private boolean createNew = false;
   private LabeledText textName;
   private LabeledText textDisplayName;
   private Combo comboDataType = null;
   private Button checkMandatory;
   private Button checkUnique;
   private Button checkHidden;
   private Button useLimits;
   private LabeledSpinner spinnerRangeMin;
   private LabeledSpinner spinnerRangeMax;
   private Combo comboSystemType;

   /**
    * Page constructor
    * 
    * @param attribute attribute to edit
    */
   public AssetAttributeGeneral(AssetAttribute attribute, boolean createNew)
   {
      super("General");
      this.attribute = attribute;
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
      textName.setText(attribute.getName());
      textName.setEditable(createNew);

      textDisplayName = new LabeledText(dialogArea, SWT.NONE);
      textDisplayName.setLabel("Display name");
      gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.horizontalSpan = 2;
      textDisplayName.setLayoutData(gd);
      textDisplayName.setText(attribute.getDisplayName());

      comboDataType = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, i18n.tr("Data type"), new GridData(SWT.FILL, SWT.CENTER, true, false));
      for (String s : AssetAttributeListLabelProvider.DATA_TYPES)
      {
         comboDataType.add(s);
      }
      comboDataType.select(attribute.getDataType().getValue());

      comboDataType.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            AMDataType seleciton = AMDataType.getByValue(comboDataType.getSelectionIndex());
            boolean haveLimits = seleciton == AMDataType.STRING || seleciton == AMDataType.NUMBER || seleciton == AMDataType.INTEGER;
            useLimits.setEnabled(haveLimits);
            haveLimits &= useLimits.getSelection();
            spinnerRangeMax.setEnabled(haveLimits);       
            spinnerRangeMax.setLabel((seleciton == AMDataType.STRING) ? i18n.tr("Maximum length") : i18n.tr("Maximum value"));
            spinnerRangeMax.setRange((seleciton == AMDataType.STRING) ? 0 : Integer.MIN_VALUE, (attribute.getDataType() == AMDataType.STRING) ? 255 : Integer.MAX_VALUE);
            spinnerRangeMin.setEnabled(haveLimits);
            spinnerRangeMin.setLabel((seleciton == AMDataType.STRING) ? i18n.tr("Minimum length") : i18n.tr("Minimum value"));
            spinnerRangeMin.setRange((seleciton == AMDataType.STRING) ? 0 : Integer.MIN_VALUE, (attribute.getDataType() == AMDataType.STRING) ? 255 : Integer.MAX_VALUE);
         }
      });

      comboSystemType = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, i18n.tr("System type"), new GridData(SWT.FILL, SWT.CENTER, true, false));
      for (String s : AssetAttributeListLabelProvider.SYSTEM_TYPE)
      {
         comboSystemType.add(s);
      }
      comboSystemType.select(attribute.getSystemType().getValue());

      boolean haveLimits = (attribute.getDataType() == AMDataType.STRING || attribute.getDataType() == AMDataType.NUMBER || attribute.getDataType() == AMDataType.INTEGER);
      useLimits = new Button(dialogArea, SWT.CHECK);
      useLimits.setText(i18n.tr("Use limits"));
      gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.horizontalSpan = 2;
      useLimits.setLayoutData(gd);
      useLimits.setSelection(attribute.getRangeMax() != 0 || attribute.getRangeMin() != 0);
      useLimits.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            spinnerRangeMax.setEnabled(useLimits.getSelection()); 
            spinnerRangeMin.setEnabled(useLimits.getSelection());
         }
      });
      useLimits.setEnabled(haveLimits);
      haveLimits = haveLimits && useLimits.getSelection();

      spinnerRangeMin = new LabeledSpinner(dialogArea, SWT.NONE);
      spinnerRangeMin.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      spinnerRangeMin.setLabel((attribute.getDataType() == AMDataType.STRING) ? i18n.tr("Minimum length") : i18n.tr("Minimum value"));
      spinnerRangeMin.setRange((attribute.getDataType() == AMDataType.STRING) ? 0 : Integer.MIN_VALUE, (attribute.getDataType() == AMDataType.STRING) ? 255 : Integer.MAX_VALUE);
      spinnerRangeMin.setSelection(attribute.getRangeMin());

      spinnerRangeMax = new LabeledSpinner(dialogArea, SWT.NONE);
      spinnerRangeMax.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      spinnerRangeMax.setLabel((attribute.getDataType() == AMDataType.STRING) ? i18n.tr("Maximum length") : i18n.tr("Minimum value"));
      spinnerRangeMax.setRange((attribute.getDataType() == AMDataType.STRING) ? 0 : Integer.MIN_VALUE, (attribute.getDataType() == AMDataType.STRING) ? 255 : Integer.MAX_VALUE);
      spinnerRangeMax.setSelection(attribute.getRangeMax());

      spinnerRangeMin.setEnabled(haveLimits);
      spinnerRangeMax.setEnabled(haveLimits);

      Composite optionsContainer = new Composite(dialogArea, SWT.NONE);
      optionsContainer.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      optionsContainer.setLayout(new GridLayout());

      checkMandatory = new Button(optionsContainer, SWT.CHECK);
      checkMandatory.setText(i18n.tr("&Mandatory"));
      checkMandatory.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      checkMandatory.setSelection(attribute.isMandatory());
      
      checkUnique = new Button(optionsContainer, SWT.CHECK);
      checkUnique.setText(i18n.tr("&Unique"));
      checkUnique.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      checkUnique.setSelection(attribute.isUnique());
      
      checkHidden = new Button(optionsContainer, SWT.CHECK);
      checkHidden.setText(i18n.tr("&Hidden"));
      checkHidden.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      checkHidden.setSelection(attribute.isUnique());

      return dialogArea;
   }

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
   {
      if (useLimits.getSelection() && (spinnerRangeMin.getSelection() > spinnerRangeMax.getSelection()))
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
         if (!newName.matches("[A-Za-z$_][A-Za-z0-9$_]*"))
         { 
            MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Name should be NXSL compliant"));
            return false;            
         }
         attribute.setName(textName.getText());
      }
      attribute.setDisplayName(textDisplayName.getText());
      attribute.setDataType(AMDataType.getByValue(comboDataType.getSelectionIndex()));
      attribute.setSystemType(AMSystemType.getByValue(comboSystemType.getSelectionIndex()));
      if (useLimits.getSelection())
      {
         attribute.setRangeMin(spinnerRangeMin.getSelection());
         attribute.setRangeMax(spinnerRangeMax.getSelection());
      }
      else
      {
         attribute.setRangeMin(0);
         attribute.setRangeMax(0);         
      }
      attribute.setMandatory(checkMandatory.getSelection());
      attribute.setUnique(checkUnique.getSelection());
      attribute.setHidden(checkHidden.getSelection());

      return true;
   }
}
