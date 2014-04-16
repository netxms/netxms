/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
import org.netxms.client.maps.configs.SingleDciConfig;
import org.netxms.client.maps.elements.NetworkMapDCIImage;
import org.netxms.ui.eclipse.datacollection.widgets.DciSelector;
import org.netxms.ui.eclipse.imagelibrary.widgets.ImageSelector;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * DCI container general property page
 *
 */
public class GeneralDCIImagePropertyPage extends PropertyPage
{
	
   private DciSelector dci;
   private LabeledText instanceColumn;
   private LabeledText dataColumn;
   private NetworkMapDCIImage container;
   private DCIImageConfiguration config;
   private ImageSelector image;
   private UUID selectedImage;

   @Override
   protected Control createContents(Composite parent)
   {
      container = (NetworkMapDCIImage)getElement().getAdapter(NetworkMapDCIImage.class);
      config = container.getImageOptions();
      SingleDciConfig dciConf = config.getDci();
      
      Composite dialogArea = new Composite(parent, SWT.NONE);
      
      GridLayout layout = new GridLayout();
      layout.numColumns = 1;
      layout.makeColumnsEqualWidth = true;
      dialogArea.setLayout(layout);
      
      dci = new DciSelector(dialogArea, SWT.NONE, false);
      dci.setLabel("Data source");
      if(dciConf != null)
      {
         dci.setDciId(dciConf.getNodeId(), dciConf.getDciId());
         dci.setDciObjectType(dciConf.getType());
      }
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      dci.setLayoutData(gd);      
      
      instanceColumn = new LabeledText(dialogArea, SWT.NONE);
      instanceColumn.setLabel("Column");
      if(dciConf !=  null)
         instanceColumn.setText(dciConf.getColumn());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      instanceColumn.setLayoutData(gd);

      dataColumn = new LabeledText(dialogArea, SWT.NONE);
      dataColumn.setLabel("Instance");
      if(dciConf !=  null)
         dataColumn.setText(dciConf.getInstance());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      dataColumn.setLayoutData(gd);
      
      image = new ImageSelector(dialogArea, SWT.NONE);
      image.setLabel("Default image");
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      image.setLayoutData(gd);
      try
      {
         selectedImage = UUID.fromString(config.getDefaultImage());
         image.setImageGuid(selectedImage, true);
      }
      catch (Exception e) {
      }
      
      return dialogArea;
   }
   
   /**
    * Apply changes
    * 
    * @param isApply true if update operation caused by "Apply" button
    */
   private boolean applyChanges(final boolean isApply)
   {
      if(dci.getDciId() == 0)
      {
         MessageDialogHelper.openError(getShell(), "Required field is empty", "DCI ID should be selected");
         return false;
      }
      
      selectedImage = image.getImageGuid();
      
      if(selectedImage ==null || selectedImage == NXCommon.EMPTY_GUID)
      {
         MessageDialogHelper.openError(getShell(), "Required field is empty", "Default image should be selected");
         return false;
      }
      
      SingleDciConfig dciConf = config.getDci();
      if(dciConf == null)
         dciConf = new SingleDciConfig();      
      
      dciConf.setDciId(dci.getDciId());
      dciConf.setNodeId(dci.getNodeId());
      dciConf.setName(dci.getDciName());
      dciConf.setType(dci.getDciObjectType());
      dciConf.setColumn(instanceColumn.getText());
      dciConf.setInstance(dataColumn.getText());    
      
      config.setDci(dciConf);      
      config.setDefaultImage(selectedImage.toString());
      
      return true;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
   @Override
   public boolean performOk()
   {
      return applyChanges(false);
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.preference.PreferencePage#performApply()
    */
   @Override
   protected void performApply()
   {
      applyChanges(true);
   }
}
