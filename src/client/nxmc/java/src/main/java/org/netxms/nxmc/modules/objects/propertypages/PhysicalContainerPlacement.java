/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Raden Solutions
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
package org.netxms.nxmc.modules.objects.propertypages;

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
import org.eclipse.swt.widgets.Group;
import org.netxms.base.NXCommon;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RackOrientation;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Chassis;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.configs.ChassisPlacement;
import org.netxms.client.objects.interfaces.HardwareEntity;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.imagelibrary.widgets.ImageSelector;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * "Rack" property page for NetXMS object
 */
public class PhysicalContainerPlacement extends ObjectPropertyPage
{
   private static final Logger logger = LoggerFactory.getLogger(PhysicalContainerPlacement.class);
   private I18n i18n = LocalizationHelper.getI18n(PhysicalContainerPlacement.class);

   private final static String[] ORIENTATION = { "Fill", "Front", "Rear" };
   private final static String[] CHASSIS_ORIENTATION = { "Front", "Rear" };
   private final static String[] VERTICAL_UNITS = { "RU", "mm" };
   private final static String[] HORIZONTAL_UNITS = { "HP", "mm" };

   private Composite dialogArea;
   private HardwareEntity hardwareEntity;
	
	private Composite rackElements;
	private ObjectSelector objectSelector;
	private ImageSelector rackImageFrontSelector;
   private ImageSelector rackImageRearSelector;
	private LabeledSpinner rackHeight;
	private LabeledSpinner rackPosition;
	private Combo rackOrientation;
	
	private Composite chassisElements;
	private ImageSelector chassisImageSelector;
	private LabeledSpinner chassisHeight;
	private Combo chassisHeightUnits;
   private LabeledSpinner chassisWidth;
   private Combo chassisWidthUnits;
   private LabeledSpinner chassisVerticalPosition;
   private Combo chassisVerticalPositionUnits;
   private LabeledSpinner chassisHorizontalPosition;
   private Combo chassisHorizontalPositionUnits;
   private Combo chassisOrientation;
	
   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public PhysicalContainerPlacement(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(PhysicalContainerPlacement.class).tr("Rack or Chassis"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "physicalCOntainerPlacement";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return (object instanceof HardwareEntity);
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		dialogArea = new Composite(parent, SWT.NONE);
		
      hardwareEntity = (HardwareEntity)object;

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);
      
      objectSelector = new ObjectSelector(dialogArea, SWT.NONE, true);
      objectSelector.setLabel("Rack or chassis");      
      objectSelector.setObjectId(hardwareEntity.getPhysicalContainerId());

      Set<Class<? extends AbstractObject>> filter = new HashSet<Class<? extends AbstractObject>>();
      filter.add(Rack.class);
      if (!(object instanceof Chassis))
      {
         filter.add(Chassis.class);
         objectSelector.setClassFilter(ObjectSelectionDialog.createRackOrChassisSelectionFilter());
      }
      else
      {
         objectSelector.setClassFilter(ObjectSelectionDialog.createRackSelectionFilter());
      }
      objectSelector.setObjectClass(filter);

		GridData gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		objectSelector.setLayoutData(gd);
		objectSelector.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            updateFieldControls();
         }
      });
		updateFieldControls();
      
		return dialogArea;
	}
	
	/**
	 * Update rack or chassis field controls
	 */
	private void updateFieldControls()
	{
      AbstractObject selectedObject = objectSelector.getObject(); 
      if (selectedObject != null)
      {
         if (selectedObject instanceof Rack)
            createRackFieldControls();
         else
            createChassisFieldControls();
      }
      else if (object instanceof Chassis)
      {
         createRackFieldControls();
      }
      else if (rackElements != null)
      {
         rackElements.dispose();
         rackElements = null;
      }
      else if (chassisElements != null)
      {
         chassisElements.dispose();
         chassisElements = null;
      }
      dialogArea.getParent().layout(true, true);
	}
	
	/**
	 * Create chassis fields and dispose rack fields
	 */
	private void createChassisFieldControls()
   {
	   if (chassisElements != null)
	      return;

      if (rackElements != null)
      {
         rackElements.dispose();
         rackElements = null;
      }
      
      ChassisPlacement placement = hardwareEntity.getChassisPlacement();
      if(placement == null)
         placement = new ChassisPlacement();
      
      chassisElements = new Composite(dialogArea, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.numColumns = 2;
      layout.makeColumnsEqualWidth = true;
      chassisElements.setLayout(layout);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      chassisElements.setLayoutData(gd);
      
      chassisImageSelector = new ImageSelector(chassisElements, SWT.NONE);
      chassisImageSelector.setLabel("Chassis image");
      chassisImageSelector.setImageGuid(placement.getImage(), true);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 2;
      chassisImageSelector.setLayoutData(gd);
      
      Group sizeGroup = new Group(chassisElements, SWT.NONE);
      sizeGroup.setText("Size");
      layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
      layout.numColumns = 2;
      sizeGroup.setLayout(layout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      sizeGroup.setLayoutData(gd);
      
      chassisHeight = new LabeledSpinner(sizeGroup, SWT.NONE);
      chassisHeight.setRange(0, 10000);
      chassisHeight.setLabel("Height");
      chassisHeight.setText(Integer.toString(placement.getHeight()));
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.BOTTOM;
      chassisHeight.setLayoutData(gd);

      chassisHeightUnits = new Combo(sizeGroup, SWT.READ_ONLY);
      chassisHeightUnits.setItems(VERTICAL_UNITS);
      chassisHeightUnits.select(placement.getHeightUnits());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.BOTTOM;
      chassisHeightUnits.setLayoutData(gd);
      
      chassisWidth = new LabeledSpinner(sizeGroup, SWT.NONE);
      chassisWidth.setRange(0, 10000);
      chassisWidth.setLabel("Width");
      chassisWidth.setText(Integer.toString(placement.getWidth()));
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.BOTTOM;
      chassisWidth.setLayoutData(gd);

      chassisWidthUnits = new Combo(sizeGroup, SWT.READ_ONLY);
      chassisWidthUnits.setItems(HORIZONTAL_UNITS);
      chassisWidthUnits.select(placement.getWidthUnits());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.BOTTOM;
      chassisWidthUnits.setLayoutData(gd);
      
      Group positionGroup = new Group(chassisElements, SWT.NONE);
      positionGroup.setText("Position");
      layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
      layout.numColumns = 2;
      positionGroup.setLayout(layout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      positionGroup.setLayoutData(gd);
      
      chassisVerticalPosition = new LabeledSpinner(positionGroup, SWT.NONE);
      chassisVerticalPosition.setRange(0, 10000);
      chassisVerticalPosition.setLabel("Vertical");
      chassisVerticalPosition.setText(Integer.toString(placement.getPositionHeight()));
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.BOTTOM;
      chassisVerticalPosition.setLayoutData(gd);

      chassisVerticalPositionUnits = new Combo(positionGroup, SWT.READ_ONLY);
      chassisVerticalPositionUnits.setItems(VERTICAL_UNITS);
      chassisVerticalPositionUnits.select(placement.getPositionHeightUnits());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.BOTTOM;
      chassisVerticalPositionUnits.setLayoutData(gd);
      
      chassisHorizontalPosition = new LabeledSpinner(positionGroup, SWT.NONE);
      chassisHorizontalPosition.setRange(0, 10000);
      chassisHorizontalPosition.setLabel("Horizontal");
      chassisHorizontalPosition.setText(Integer.toString(placement.getPositionWidth()));
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.BOTTOM;
      chassisHorizontalPosition.setLayoutData(gd);

      chassisHorizontalPositionUnits = new Combo(positionGroup, SWT.READ_ONLY);
      chassisHorizontalPositionUnits.setItems(HORIZONTAL_UNITS);
      chassisHorizontalPositionUnits.select(placement.getPositionWidthUnits());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.BOTTOM;
      chassisHorizontalPositionUnits.setLayoutData(gd);
      
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 2;
      chassisOrientation = WidgetHelper.createLabeledCombo(chassisElements, SWT.READ_ONLY, "Orientation", gd);
      chassisOrientation.setItems(CHASSIS_ORIENTATION);
      chassisOrientation.select(placement.getOritentaiton() - 1);
   }

	/**
	 * Create rack fields and dispose chassis elements
	 */
   private void createRackFieldControls()
	{
      if (rackElements != null)
         return;

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
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      rackElements.setLayoutData(gd);

	   rackImageFrontSelector = new ImageSelector(rackElements, SWT.NONE);
      rackImageFrontSelector.setLabel("Rack front image");
      rackImageFrontSelector.setImageGuid(hardwareEntity.getFrontRackImage(), true);
      rackImageFrontSelector.setEnabled(hardwareEntity.getRackOrientation() == RackOrientation.FRONT || hardwareEntity.getRackOrientation() == RackOrientation.FILL);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 3;
      rackImageFrontSelector.setLayoutData(gd);

      rackImageRearSelector = new ImageSelector(rackElements, SWT.NONE);
      rackImageRearSelector.setLabel("Rack rear image");
      rackImageRearSelector.setImageGuid(hardwareEntity.getRearRackImage(), true);
      rackImageRearSelector.setEnabled(hardwareEntity.getRackOrientation() == RackOrientation.REAR || hardwareEntity.getRackOrientation() == RackOrientation.FILL);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 3;
      rackImageRearSelector.setLayoutData(gd);

      rackPosition = new LabeledSpinner(rackElements, SWT.NONE);
      rackPosition.setLabel(i18n.tr("Position"));
      rackPosition.setRange(1, 50);
      rackPosition.setSelection(hardwareEntity.getRackPosition());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      rackPosition.setLayoutData(gd);

      rackHeight = new LabeledSpinner(rackElements, SWT.NONE);
      rackHeight.setLabel(i18n.tr("Height"));
      rackHeight.setRange(1, 50);
      rackHeight.setSelection(hardwareEntity.getRackHeight());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      rackHeight.setLayoutData(gd);

      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      rackOrientation = WidgetHelper.createLabeledCombo(rackElements, SWT.READ_ONLY, "Orientation", gd);
      rackOrientation.setItems(ORIENTATION);
      rackOrientation.select(hardwareEntity.getRackOrientation().getValue());
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
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
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
            md.setChassisPlacement("");
         }
         else
         {
            md.setRackPlacement(NXCommon.EMPTY_GUID, NXCommon.EMPTY_GUID, (short)0, (short)0, RackOrientation.getByValue(0));
            ChassisPlacement placement = new ChassisPlacement(chassisImageSelector.getImageGuid(), Integer.parseInt(chassisHeight.getText()), 
                  chassisHeightUnits.getSelectionIndex(), Integer.parseInt(chassisWidth.getText()), chassisWidthUnits.getSelectionIndex(), 
                  Integer.parseInt(chassisVerticalPosition.getText()), chassisVerticalPositionUnits.getSelectionIndex(), 
                  Integer.parseInt(chassisHorizontalPosition.getText()), chassisHorizontalPositionUnits.getSelectionIndex(), chassisOrientation.getSelectionIndex()+1);
            String placementConfig = null;
            try
            {
               placementConfig = placement.createXml();
            }
            catch(Exception e)
            {
               logger.debug("Cannot create XML document from ChassisPlacement object", e);
            }
            md.setChassisPlacement(placementConfig);
         }
      }
      else if (object instanceof Chassis)
      {
         md.setRackPlacement(rackImageFrontSelector.getImageGuid(), rackImageRearSelector.getImageGuid(), (short)rackPosition.getSelection(), (short)rackHeight.getSelection(),
               RackOrientation.getByValue(rackOrientation.getSelectionIndex()));         
      }
      else  
      {

         md.setPhysicalContainer(0);
         md.setRackPlacement(NXCommon.EMPTY_GUID, NXCommon.EMPTY_GUID, (short)0, (short)0, RackOrientation.getByValue(0));
         md.setChassisPlacement("");
      }
		
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating rack placement for object {0}", object.getObjectName()), null, messageArea) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
			   session.modifyObject(md);
			}

			@Override
			protected String getErrorMessage()
			{
            return "Cannot update rack/chassis placement";
			}

			@Override
			protected void jobFinalize()
			{
				if (isApply)
               runInUIThread(() -> PhysicalContainerPlacement.this.setValid(true));
			}
		}.start();
		return true;
	}

   /**
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
      chassisHeight.setText("0");
      chassisHeightUnits.setText(VERTICAL_UNITS[0]);
      chassisWidth.setText("0");
      chassisWidthUnits.setText(HORIZONTAL_UNITS[0]);
      chassisVerticalPosition.setText("0");
      chassisVerticalPositionUnits.setText(VERTICAL_UNITS[0]);
      chassisHorizontalPosition.setText("0");
      chassisHorizontalPositionUnits.setText(HORIZONTAL_UNITS[0]);
      chassisOrientation.setText(CHASSIS_ORIENTATION[0]);
	}
}
