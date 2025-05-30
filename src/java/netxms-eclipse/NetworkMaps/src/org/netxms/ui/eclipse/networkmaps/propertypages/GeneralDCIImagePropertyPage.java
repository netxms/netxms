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
package org.netxms.ui.eclipse.networkmaps.propertypages;

import java.util.UUID;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.base.NXCommon;
import org.netxms.client.maps.configs.DCIImageConfiguration;
import org.netxms.client.maps.configs.MapImageDataSource;
import org.netxms.client.maps.elements.NetworkMapDCIImage;
import org.netxms.ui.eclipse.datacollection.widgets.DciSelector;
import org.netxms.ui.eclipse.imagelibrary.widgets.ImageSelector;
import org.netxms.ui.eclipse.networkmaps.Messages;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * DCI image general property page
 */
public class GeneralDCIImagePropertyPage extends PropertyPage
{
   private DciSelector dci;
   private LabeledText column;
   private LabeledText instance;
   private NetworkMapDCIImage container;
   private DCIImageConfiguration config;
   private ImageSelector image;
   private UUID selectedImage;

   @Override
   protected Control createContents(Composite parent)
   {
      container = (NetworkMapDCIImage)getElement().getAdapter(NetworkMapDCIImage.class);
      config = container.getImageOptions();
      MapImageDataSource dciConf = config.getDci();

      Composite dialogArea = new Composite(parent, SWT.NONE);

      GridLayout layout = new GridLayout();
      layout.numColumns = 1;
      layout.makeColumnsEqualWidth = true;
      dialogArea.setLayout(layout);

      dci = new DciSelector(dialogArea, SWT.NONE);
      dci.setLabel(Messages.get().GeneralDCIImagePropertyPage_DataSource);
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
      column.setLabel(Messages.get().GeneralDCIImagePropertyPage_Column);
      if (dciConf != null)
         column.setText(dciConf.getColumn());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      column.setLayoutData(gd);

      instance = new LabeledText(dialogArea, SWT.NONE);
      instance.setLabel(Messages.get().GeneralDCIImagePropertyPage_Instance);
      if (dciConf != null)
         instance.setText(dciConf.getInstance());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      instance.setLayoutData(gd);

      image = new ImageSelector(dialogArea, SWT.NONE);
      image.setLabel(Messages.get().GeneralDCIImagePropertyPage_DefaultImage);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      image.setLayoutData(gd);
      selectedImage = config.getDefaultImage();
      image.setImageGuid(selectedImage, true);
      
      return dialogArea;
   }
   
   /**
    * Apply changes
    * 
    * @param isApply true if update operation caused by "Apply" button
    */
   private boolean applyChanges(final boolean isApply)
   {
      if (dci.getDciId() == 0)
      {
         MessageDialogHelper.openError(getShell(), Messages.get().GeneralDCIImagePropertyPage_Error, Messages.get().GeneralDCIImagePropertyPage_DciNotSelected);
         return false;
      }
      
      selectedImage = image.getImageGuid();
      
      if ((selectedImage == null) || selectedImage.equals(NXCommon.EMPTY_GUID))
      {
         MessageDialogHelper.openError(getShell(), Messages.get().GeneralDCIImagePropertyPage_Error, Messages.get().GeneralDCIImagePropertyPage_DefImageNotSelected);
         return false;
      }
      
      MapImageDataSource dciConf = config.getDci();
      if(dciConf == null)
         dciConf = new MapImageDataSource();      
      
      dciConf.setDciId(dci.getDciId());
      dciConf.setNodeId(dci.getNodeId());
      dciConf.setName(dci.getDciToolTipInfo());
      dciConf.setType(dci.getDciObjectType());
      dciConf.setColumn(column.getText());
      dciConf.setInstance(instance.getText());    
      
      config.setDci(dciConf);      
      config.setDefaultImage(selectedImage);
      container.setImageOptions(config);
      
      return true;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
   @Override
   public boolean performOk()
   {
      return applyChanges(false);
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performApply()
    */
   @Override
   protected void performApply()
   {
      applyChanges(true);
   }
}
