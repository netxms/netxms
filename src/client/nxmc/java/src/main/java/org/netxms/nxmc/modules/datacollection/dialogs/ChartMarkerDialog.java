/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.datacollection.dialogs;

import java.util.Date;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.preference.ColorSelector;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.nxmc.base.widgets.DateTimeSelector;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.charts.api.ChartMarker;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for editing properties (label, position, color) of a chart marker.
 */
public class ChartMarkerDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(ChartMarkerDialog.class);

   private ChartMarker marker;
   private LabeledText labelInput;
   private DateTimeSelector timeSelector;
   private ColorSelector colorSelector;

   /**
    * @param parentShell parent shell
    * @param marker marker to edit (modified in place on OK)
    */
   public ChartMarkerDialog(Shell parentShell, ChartMarker marker)
   {
      super(parentShell);
      this.marker = marker;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Marker Properties"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      dialogArea.setLayout(layout);

      labelInput = new LabeledText(dialogArea, SWT.NONE);
      labelInput.setLabel(i18n.tr("Label"));
      labelInput.setText((marker.getLabel() != null) ? marker.getLabel() : "");
      GridData gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.widthHint = 300;
      labelInput.setLayoutData(gd);

      Label timeLabel = new Label(dialogArea, SWT.NONE);
      timeLabel.setText(i18n.tr("Position"));
      timeSelector = new DateTimeSelector(dialogArea, SWT.NONE);
      timeSelector.setValue(new Date(marker.getTimestamp()));
      timeSelector.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      Label colorLabel = new Label(dialogArea, SWT.NONE);
      colorLabel.setText(i18n.tr("Color"));
      colorSelector = new ColorSelector(dialogArea);
      colorSelector.setColorValue(marker.getColor());

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      marker.setLabel(labelInput.getText().trim());
      marker.setTimestamp(timeSelector.getValue().getTime());
      RGB color = colorSelector.getColorValue();
      if (color != null)
         marker.setColor(color);
      super.okPressed();
   }
}
