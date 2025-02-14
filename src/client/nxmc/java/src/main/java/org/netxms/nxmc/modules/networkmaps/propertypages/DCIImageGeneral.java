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
package org.netxms.nxmc.modules.networkmaps.propertypages;

import java.util.UUID;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.base.NXCommon;
import org.netxms.client.maps.configs.DCIImageConfiguration;
import org.netxms.client.maps.configs.MapImageDataSource;
import org.netxms.client.maps.elements.NetworkMapDCIImage;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.widgets.DciSelector;
import org.netxms.nxmc.modules.imagelibrary.widgets.ImageSelector;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Network map DCI image property page
 */
public class DCIImageGeneral extends PropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(DCIImageGeneral.class);
   private NetworkMapDCIImage dciImage;
   private DCIImageConfiguration config;
   private DciSelector dci;
   private LabeledText column;
   private LabeledText instance;
   private ImageSelector image;
   private UUID selectedImage;

   /**
    * Create new page for given DCI image.
    *
    * @param dciImage DCI image to edit
    */
   public DCIImageGeneral(NetworkMapDCIImage dciImage)
   {
      super(LocalizationHelper.getI18n(DCIImageGeneral.class).tr("General"));
      this.dciImage = dciImage;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      config = dciImage.getImageOptions();
      MapImageDataSource dciConf = config.getDci();

      Composite dialogArea = new Composite(parent, SWT.NONE);

      GridLayout layout = new GridLayout();
      layout.numColumns = 1;
      layout.makeColumnsEqualWidth = true;
      dialogArea.setLayout(layout);

      dci = new DciSelector(dialogArea, SWT.NONE);
      dci.setLabel(i18n.tr("Metric"));
      if (dciConf != null)
      {
         dci.setDciId(dciConf.getNodeId(), dciConf.getDciId());
         dci.setDciObjectType(dciConf.getType());
      }
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      dci.setLayoutData(gd);      
      
      column = new LabeledText(dialogArea, SWT.NONE);
      column.setLabel(i18n.tr("Column"));
      if (dciConf != null)
         column.setText(dciConf.getColumn());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      column.setLayoutData(gd);

      instance = new LabeledText(dialogArea, SWT.NONE);
      instance.setLabel(i18n.tr("Instance"));
      if (dciConf != null)
         instance.setText(dciConf.getInstance());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      instance.setLayoutData(gd);

      image = new ImageSelector(dialogArea, SWT.NONE);
      image.setLabel(i18n.tr("Default image"));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      image.setLayoutData(gd);
      selectedImage = config.getDefaultImage();
      image.setImageGuid(selectedImage, true);

      return dialogArea;
   }

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
   {
      if (dci.getDciId() == 0)
      {
         MessageDialogHelper.openError(getShell(), i18n.tr("Error"), i18n.tr("Metric is not selected"));
         return false;
      }

      selectedImage = image.getImageGuid();
      if ((selectedImage == null) || selectedImage.equals(NXCommon.EMPTY_GUID))
      {
         MessageDialogHelper.openError(getShell(), i18n.tr("Error"), i18n.tr("Default image not selected"));
         return false;
      }

      MapImageDataSource dciConf = config.getDci();
      if (dciConf == null)
         dciConf = new MapImageDataSource();

      dciConf.setDciId(dci.getDciId());
      dciConf.setNodeId(dci.getNodeId());
      dciConf.setName(dci.getDciToolTipInfo());
      dciConf.setType(dci.getDciObjectType());
      dciConf.setColumn(column.getText());
      dciConf.setInstance(instance.getText());

      config.setDci(dciConf);
      config.setDefaultImage(selectedImage);
      dciImage.setImageOptions(config);

      return true;
   }
}
