/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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

import java.util.UUID;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.base.NXCommon;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RoomGridLabels;
import org.netxms.client.constants.RoomType;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Room;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledCombo;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.imagelibrary.widgets.ImageSelector;
import org.netxms.nxmc.modules.objects.widgets.helpers.RoomTypeLabels;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Room" property page for room object. Room outline, backdrop calibration and passive elements are edited in floor plan view.
 */
public class RoomProperties extends ObjectPropertyPage
{
   private I18n i18n = LocalizationHelper.getI18n(RoomProperties.class);

   private Room room;
   private LabeledCombo roomType;
   private LabeledSpinner height;
   private LabeledSpinner gridTileSize;
   private LabeledSpinner gridOriginX;
   private LabeledSpinner gridOriginY;
   private LabeledCombo gridLabels;
   private ImageSelector backgroundImage;

   /**
    * Create new page.
    *
    * @param object object to edit
    */
   public RoomProperties(AbstractObject object)
   {
      super(LocalizationHelper.getI18n(RoomProperties.class).tr("Room"), object);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "roomProperties";
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 1;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return (object instanceof Room);
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Composite dialogArea = new Composite(parent, SWT.NONE);

      room = (Room)object;

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.numColumns = 2;
      layout.makeColumnsEqualWidth = true;
      dialogArea.setLayout(layout);

      roomType = new LabeledCombo(dialogArea, SWT.NONE);
      roomType.setLabel(i18n.tr("Room type"));
      roomType.setContent(RoomTypeLabels.getAll());
      roomType.select(room.getRoomType().getValue());
      roomType.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      height = new LabeledSpinner(dialogArea, SWT.NONE);
      height.setLabel(i18n.tr("Height (mm, 0 = undeclared)"));
      height.setRange(0, 100000);
      height.setSelection(room.getHeight());
      height.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      gridTileSize = new LabeledSpinner(dialogArea, SWT.NONE);
      gridTileSize.setLabel(i18n.tr("Floor tile size (mm, 0 = no grid)"));
      gridTileSize.setRange(0, 10000);
      gridTileSize.setSelection(room.getGridTileSize());
      gridTileSize.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      gridLabels = new LabeledCombo(dialogArea, SWT.NONE);
      gridLabels.setLabel(i18n.tr("Floor tile labels"));
      gridLabels.add(i18n.tr("None"));
      gridLabels.add(i18n.tr("Letters and numbers (A1, B2, ...)"));
      gridLabels.add(i18n.tr("Numbers and numbers (1-1, 2-2, ...)"));
      gridLabels.select(room.getGridLabels().getValue());
      gridLabels.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      gridOriginX = new LabeledSpinner(dialogArea, SWT.NONE);
      gridOriginX.setLabel(i18n.tr("Tile grid origin X (mm)"));
      gridOriginX.setRange(-1000000, 1000000);
      gridOriginX.setSelection(room.getGridOriginX());
      gridOriginX.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      gridOriginY = new LabeledSpinner(dialogArea, SWT.NONE);
      gridOriginY.setLabel(i18n.tr("Tile grid origin Y (mm)"));
      gridOriginY.setRange(-1000000, 1000000);
      gridOriginY.setSelection(room.getGridOriginY());
      gridOriginY.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      backgroundImage = new ImageSelector(dialogArea, SWT.NONE);
      backgroundImage.setLabel(i18n.tr("Floor plan background image"));
      backgroundImage.setImageGuid(room.getBackgroundImage(), true);
      GridData gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.horizontalSpan = 2;
      backgroundImage.setLayoutData(gd);

      return dialogArea;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
   {
      if (isApply)
         setValid(false);

      final NXCObjectModificationData md = new NXCObjectModificationData(room.getObjectId());
      md.setRoomType(RoomType.getByValue(roomType.getSelectionIndex()));
      md.setRoomHeight(height.getSelection());
      md.setGridTileSize(gridTileSize.getSelection());
      md.setGridLabels(RoomGridLabels.getByValue(gridLabels.getSelectionIndex()));
      md.setGridOriginX(gridOriginX.getSelection());
      md.setGridOriginY(gridOriginY.getSelection());
      UUID image = backgroundImage.getImageGuid();
      md.setRoomBackgroundImage((image != null) ? image : NXCommon.EMPTY_GUID);

      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Updating properties of room {0}", room.getObjectName()), null, messageArea) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modifyObject(md);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update room properties");
         }

         @Override
         protected void jobFinalize()
         {
            if (isApply)
               runInUIThread(() -> RoomProperties.this.setValid(true));
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
      roomType.select(RoomType.OTHER.getValue());
      height.setSelection(0);
      gridTileSize.setSelection(0);
      gridLabels.select(RoomGridLabels.NONE.getValue());
      gridOriginX.setSelection(0);
      gridOriginY.setSelection(0);
      backgroundImage.setImageGuid(NXCommon.EMPTY_GUID, true);
   }
}
