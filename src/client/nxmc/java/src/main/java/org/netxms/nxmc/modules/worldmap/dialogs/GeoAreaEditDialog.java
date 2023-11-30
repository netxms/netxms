/**
 * NetXMS - open source network management system
 * Copyright (C) 2020-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.worldmap.dialogs;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.base.GeoLocation;
import org.netxms.base.GeoLocationFormatException;
import org.netxms.client.GeoArea;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for editing object category
 */
public class GeoAreaEditDialog extends Dialog
{
   private static final Logger logger = LoggerFactory.getLogger(GeoAreaEditDialog.class);

   private final I18n i18n = LocalizationHelper.getI18n(GeoAreaEditDialog.class);

   private GeoArea area;
   private LabeledText name;
   private LabeledText comments;
   private LabeledText border;

   /**
    * Create new dialog.
    *
    * @param parentShell parent shell
    * @param category category object to edit or null if new category to be created
    */
   public GeoAreaEditDialog(Shell parentShell, GeoArea area)
   {
      super(parentShell);
      this.area = area;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText((area != null) ? i18n.tr("Edit Geographical Area") : i18n.tr("Create Geographical Area"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      dialogArea.setLayout(layout);

      name = new LabeledText(dialogArea, SWT.NONE);
      name.setLabel("Name");
      name.getTextControl().setTextLimit(127);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 600;
      name.setLayoutData(gd);

      comments = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.MULTI);
      comments.setLabel("Comments");
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.heightHint = 150;
      comments.setLayoutData(gd);

      border = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.MULTI);
      border.setLabel("Border points (one point per line)");
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.heightHint = 250;
      border.setLayoutData(gd);

      if (area != null)
      {
         name.setText(area.getName());
         comments.setText(area.getComments());
         border.setText(borderPointsToText(area.getBorder()));
      }

      return dialogArea;
   }

   /**
    * Get border points as text
    *
    * @param borderPoints points defining area border
    * @return list of border points as text
    */
   private static String borderPointsToText(List<GeoLocation> borderPoints)
   {
      StringBuilder sb = new StringBuilder();
      for(GeoLocation p : borderPoints)
      {
         sb.append(p.getLatitudeAsString());
         sb.append(' ');
         sb.append(p.getLongitudeAsString());
         sb.append("\n");
      }
      return sb.toString();
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      if (name.getText().trim().isEmpty())
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Area name cannot be empty!"));
         return;
      }

      List<GeoLocation> borderPoints = new ArrayList<GeoLocation>();
      for(String line : border.getText().split("\n"))
      {
         try
         {
            GeoLocation p = GeoLocation.parseGeoLocation(line.trim());
            borderPoints.add(p);
         }
         catch(GeoLocationFormatException e)
         {
            logger.warn("Exception while parsing area border points", e);
            MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), String.format(i18n.tr("Cannot parse area border point %s"), line.trim()));
            return;
         }
      }

      area = new GeoArea((area != null) ? area.getId() : 0, name.getText().trim(), comments.getText(), borderPoints);
      super.okPressed();
   }

   /**
    * @return the category
    */
   public GeoArea getArea()
   {
      return area;
   }
}
