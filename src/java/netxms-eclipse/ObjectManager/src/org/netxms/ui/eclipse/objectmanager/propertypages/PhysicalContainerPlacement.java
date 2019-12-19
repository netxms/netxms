/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectmanager.propertypages;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.base.Logger;
import org.netxms.base.NXCommon;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RackOrientation;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Chassis;
import org.netxms.client.objects.ElementForPhysicalPlacment;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.configs.ChassisPlacement;
import org.netxms.ui.eclipse.imagelibrary.widgets.ImageSelector;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.objectmanager.Messages;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledSpinner;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * "Rack" property page for NetXMS object
 */
public class PhysicalContainerPlacement extends PropertyPage
{
   private final static String[] ORIENTATION = { "Fill", "Front", "Rear" };
   private final static String[] CHASSIS_ORIENTATION = { "Front", "Rear" };
   private final static String[] VERTICAL_UNITS = { "units", "mm" };
   private final static String[] HORIZONTAL_UNITS = { "h peach", "mm" };
   
   private Composite dialogArea;
	private ElementForPhysicalPlacment object;
	
	private Composite rackElements;
	private ObjectSelector objectSelector;
	private ImageSelector rackImageFrontSelector;
   private ImageSelector rackImageRearSelector;
	private LabeledSpinner rackHeight;
	private LabeledSpinner rackPosition;
	private Combo rackOrientation;
	
	private Composite chassisElements;
	private ImageSelector chassisImageSelector;
	private LabeledText elementHeight;
	private Combo elementHeightUnits;
   private LabeledText elementWidth;
   private Combo elementWidthUnits;
   private LabeledText elementPositionHeight;
   private Combo elementPositionHeightUnits;
   private LabeledText elementPositionWidth;
   private Combo elementPositionWidthUnits;
   private Combo elementOrientation;
	
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		dialogArea = new Composite(parent, SWT.NONE);
		
		object = (ElementForPhysicalPlacment)getElement().getAdapter(ElementForPhysicalPlacment.class);

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      Set<Class<? extends AbstractObject>> filter = new HashSet<Class<? extends AbstractObject>>();
      filter.add(Rack.class);
      if (!(object instanceof Chassis))
         filter.add(Chassis.class);
      objectSelector = new ObjectSelector(dialogArea, SWT.NONE, true);
      objectSelector.setLabel("Rack or chassis");      
      objectSelector.setObjectClass(filter);
      objectSelector.setObjectId(object.getPhysicalContainerId());
		GridData gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		objectSelector.setLayoutData(gd);
		objectSelector.addModifyListener(new ModifyListener() {
         
         @Override
         public void modifyText(ModifyEvent e)
         {
            AbstractObject obj = objectSelector.getObject();
            if (obj != null)
               if (obj instanceof Rack)
                  createRackFields();
               else
                  createChassisFields();    
            dialogArea.layout();
         }
      });
		
		AbstractObject obj = objectSelector.getObject(); 
		if (obj != null)
		   if (obj instanceof Rack)
		      createRackFields();
		   else
		      createChassisFields();
      
		return dialogArea;
	}
	
	/**
	 * Create chassis fields and dispose rack fields
	 */
	private void createChassisFields()
   {
      if (rackElements != null)
      {
         rackElements.dispose();
         rackElements = null;
      }
      
      ChassisPlacement placement = object.getChassisPlacement();
      if(placement == null)
         placement = new ChassisPlacement();
      
      chassisElements = new Composite(dialogArea, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.numColumns = 2;
      chassisElements.setLayout(layout);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      chassisElements.setLayoutData(gd);
      
      chassisImageSelector = new ImageSelector(chassisElements, SWT.NONE);
      chassisImageSelector.setLabel("Chassis image");
      chassisImageSelector.setImageGuid(placement.getImage(), true);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 2;
      chassisImageSelector.setLayoutData(gd);
      
      elementHeight = new LabeledText(chassisElements, SWT.NONE);
      elementHeight.setLabel("Element height");
      elementHeight.setText(Integer.toString(placement.getHeight()));
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      elementHeight.setLayoutData(gd);

      elementHeightUnits = new Combo(chassisElements, SWT.NONE);
      elementHeightUnits.setItems(VERTICAL_UNITS);
      elementHeightUnits.select(placement.getHeightUnits());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      elementHeightUnits.setLayoutData(gd);
      
      elementWidth = new LabeledText(chassisElements, SWT.NONE);
      elementWidth.setLabel("Element width");
      elementWidth.setText(Integer.toString(placement.getWidth()));
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      elementWidth.setLayoutData(gd);

      elementWidthUnits = new Combo(chassisElements, SWT.NONE);
      elementWidthUnits.setItems(HORIZONTAL_UNITS);
      elementWidthUnits.select(placement.getWidthUnits());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      elementWidthUnits.setLayoutData(gd);
      
      elementPositionHeight = new LabeledText(chassisElements, SWT.NONE);
      elementPositionHeight.setLabel("Position hight");
      elementPositionHeight.setText(Integer.toString(placement.getPositionHight()));
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      elementPositionHeight.setLayoutData(gd);

      elementPositionHeightUnits = new Combo(chassisElements, SWT.NONE);
      elementPositionHeightUnits.setItems(VERTICAL_UNITS);
      elementPositionHeightUnits.select(placement.getPositionHightUnits());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      elementPositionHeightUnits.setLayoutData(gd);
      
      elementPositionWidth = new LabeledText(chassisElements, SWT.NONE);
      elementPositionWidth.setLabel("Position width");
      elementPositionWidth.setText(Integer.toString(placement.getPositionWidth()));
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      elementPositionWidth.setLayoutData(gd);

      elementPositionWidthUnits = new Combo(chassisElements, SWT.NONE);
      elementPositionWidthUnits.setItems(HORIZONTAL_UNITS);
      elementPositionWidthUnits.select(placement.getPositionWidthUnits());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      elementPositionWidthUnits.setLayoutData(gd);
      
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      elementOrientation = WidgetHelper.createLabeledCombo(chassisElements, SWT.READ_ONLY, "Orientation", gd);
      elementOrientation.setItems(CHASSIS_ORIENTATION);
      elementOrientation.select(placement.getOritentaiton());
   }

	/**
	 * Create rack fields and dispose chassis elements
	 */
   private void createRackFields()
	{
      if (chassisElements != null)
      {
         chassisElements.dispose();
         chassisElements = null;
      }
         
	   rackElements = new Composite(dialogArea, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.numColumns = 3;
      rackElements.setLayout(layout);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      rackElements.setLayoutData(gd);
	         
	   rackImageFrontSelector = new ImageSelector(rackElements, SWT.NONE);
      rackImageFrontSelector.setLabel("Rack front image");
      rackImageFrontSelector.setImageGuid(object.getFrontRackImage(), true);
      rackImageFrontSelector.setEnabled(object.getRackOrientation() == RackOrientation.FRONT ||
                                        object.getRackOrientation() == RackOrientation.FILL);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 3;
      rackImageFrontSelector.setLayoutData(gd);
      
      rackImageRearSelector = new ImageSelector(rackElements, SWT.NONE);
      rackImageRearSelector.setLabel("Rack rear image");
      rackImageRearSelector.setImageGuid(object.getRearRackImage(), true);
      rackImageRearSelector.setEnabled(object.getRackOrientation() == RackOrientation.REAR ||
                                       object.getRackOrientation() == RackOrientation.FILL);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 3;
      rackImageRearSelector.setLayoutData(gd);
      
      rackPosition = new LabeledSpinner(rackElements, SWT.NONE);
      rackPosition.setLabel(Messages.get().RackPlacement_Position);
      rackPosition.setRange(1, 50);
      rackPosition.setSelection(object.getRackPosition());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      rackPosition.setLayoutData(gd);
      
      rackHeight = new LabeledSpinner(rackElements, SWT.NONE);
      rackHeight.setLabel(Messages.get().RackPlacement_Height);
      rackHeight.setRange(1, 50);
      rackHeight.setSelection(object.getRackHeight());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      rackHeight.setLayoutData(gd);
      
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      rackOrientation = WidgetHelper.createLabeledCombo(rackElements, SWT.READ_ONLY, "Orientation", gd);
      rackOrientation.setItems(ORIENTATION);
      rackOrientation.select(object.getRackOrientation().getValue());
      rackOrientation.addSelectionListener(new SelectionListener() {         
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            if (RackOrientation.getByValue(rackOrientation.getSelectionIndex()) == RackOrientation.FRONT)
            {
               rackImageRearSelector.setEnabled(false);
               rackImageFrontSelector.setEnabled(true);
            }
            else if (RackOrientation.getByValue(rackOrientation.getSelectionIndex()) == RackOrientation.REAR)
            {
               rackImageRearSelector.setEnabled(true);
               rackImageFrontSelector.setEnabled(false);
            }
            else
            {
               rackImageRearSelector.setEnabled(true);
               rackImageFrontSelector.setEnabled(true);
            }
         }         
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      });
	}

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
		if (isApply)
			setValid(false);
		
		final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());

      AbstractObject obj = objectSelector.getObject(); 
      if (obj != null)
      {
         md.setPhysicalContainer(objectSelector.getObjectId());
         if (obj instanceof Rack)
         {
            md.setRackPlacement(rackImageFrontSelector.getImageGuid(), rackImageRearSelector.getImageGuid(), (short)rackPosition.getSelection(), (short)rackHeight.getSelection(),
                  RackOrientation.getByValue(rackOrientation.getSelectionIndex()));
         }
         else
         {
            ChassisPlacement placement = new ChassisPlacement(chassisImageSelector.getImageGuid(), Integer.parseInt(elementHeight.getText()), 
                  elementHeightUnits.getSelectionIndex(), Integer.parseInt(elementWidth.getText()), elementWidthUnits.getSelectionIndex(), 
                  Integer.parseInt(elementPositionHeight.getText()), elementPositionHeightUnits.getSelectionIndex(), 
                  Integer.parseInt(elementPositionWidth.getText()), elementPositionWidthUnits.getSelectionIndex(), elementOrientation.getSelectionIndex());
            String placementConfig = null;
            try
            {
               placementConfig = placement.createXml();
            }
            catch(Exception e)
            {
               Logger.debug("PhysicalContainerPlacement.applyChanges", "Cannot convert ChassisPlacement to XML: ", e);
            }
            md.setChassisPlacement(placementConfig);
         }
      }
      else 
      {

         md.setPhysicalContainer(0);
         md.setRackPlacement(NXCommon.EMPTY_GUID, NXCommon.EMPTY_GUID, (short)0, (short)0, RackOrientation.getByValue(0));
         md.setChassisPlacement(null);
      }
		
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob(String.format(Messages.get().RackPlacement_UpdatingRackPlacement, object.getObjectName()), null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
			   session.modifyObject(md);
			}

			@Override
			protected String getErrorMessage()
			{
				return "Failed to update rack/chassis placement";
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
				{
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
							PhysicalContainerPlacement.this.setValid(true);
						}
					});
				}
			}
		}.start();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		applyChanges(false);
		return true;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performApply()
	 */
	@Override
	protected void performApply()
	{
		applyChanges(true);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
	 */
	@Override
	protected void performDefaults()
	{
		super.performDefaults();
		objectSelector.setObjectId(0);
		rackImageFrontSelector.setImageGuid(NXCommon.EMPTY_GUID, true);
      rackImageRearSelector.setImageGuid(NXCommon.EMPTY_GUID, true);
		rackPosition.setSelection(1);
		rackHeight.setSelection(1);
		rackOrientation.select(0);
		
		chassisImageSelector.setImageGuid(NXCommon.EMPTY_GUID, true);      
      elementHeight.setText("0");
      elementHeightUnits.setText(VERTICAL_UNITS[0]);
      elementWidth.setText("0");
      elementWidthUnits.setText(HORIZONTAL_UNITS[0]);
      elementPositionHeight.setText("0");
      elementPositionHeightUnits.setText(VERTICAL_UNITS[0]);
      elementPositionWidth.setText("0");
      elementPositionWidthUnits.setText(HORIZONTAL_UNITS[0]);
      elementOrientation.setText(CHASSIS_ORIENTATION[0]);
	}
}
