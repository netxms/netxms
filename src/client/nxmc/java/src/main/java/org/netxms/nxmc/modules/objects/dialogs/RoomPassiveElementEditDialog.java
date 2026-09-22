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
package org.netxms.nxmc.modules.objects.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.constants.RoomElementType;
import org.netxms.client.objects.configs.RoomPassiveElement;
import org.netxms.nxmc.base.widgets.LabeledCombo;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.helpers.RoomElementTypeLabels;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Room passive element (column, wall, ramp, etc.) edit dialog. Changes are applied to the given element only when OK is
 * pressed. All coordinates and dimensions are in millimetres.
 */
public class RoomPassiveElementEditDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(RoomPassiveElementEditDialog.class);

   private RoomPassiveElement element;
   private LabeledText name;
   private LabeledCombo type;
   private LabeledSpinner x;
   private LabeledSpinner y;
   private LabeledSpinner rotation;
   private LabeledSpinner width;
   private LabeledSpinner depth;

   /**
    * Create dialog.
    *
    * @param parentShell parent shell
    * @param element element to edit
    */
   public RoomPassiveElementEditDialog(Shell parentShell, RoomPassiveElement element)
   {
      super(parentShell);
      this.element = element;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Edit Floor Plan Element"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.numColumns = 2;
      layout.makeColumnsEqualWidth = true;
      dialogArea.setLayout(layout);

      name = new LabeledText(dialogArea, SWT.NONE);
      name.setLabel(i18n.tr("Name"));
      name.setText(element.name);
      name.getTextControl().setTextLimit(255);
      GridData gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.horizontalSpan = 2;
      gd.widthHint = 320;
      name.setLayoutData(gd);

      type = new LabeledCombo(dialogArea, SWT.NONE);
      type.setLabel(i18n.tr("Type"));
      type.setContent(RoomElementTypeLabels.getAll());
      type.select(element.type.getValue());
      gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.horizontalSpan = 2;
      type.setLayoutData(gd);

      x = createSpinner(dialogArea, i18n.tr("X (mm)"), -1000000, 1000000, element.x);
      y = createSpinner(dialogArea, i18n.tr("Y (mm)"), -1000000, 1000000, element.y);
      width = createSpinner(dialogArea, i18n.tr("Width (mm)"), 0, 1000000, element.width);
      depth = createSpinner(dialogArea, i18n.tr("Depth (mm)"), 0, 1000000, element.depth);
      rotation = createSpinner(dialogArea, i18n.tr("Rotation (degrees, clockwise)"), 0, 359, element.rotation);

      return dialogArea;
   }

   /**
    * Create labelled spinner filling one grid cell
    */
   private static LabeledSpinner createSpinner(Composite parent, String label, int min, int max, int value)
   {
      LabeledSpinner spinner = new LabeledSpinner(parent, SWT.NONE);
      spinner.setLabel(label);
      spinner.setRange(min, max);
      spinner.setSelection(value);
      spinner.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      return spinner;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      element.name = name.getText().trim();
      element.type = RoomElementType.getByValue(type.getSelectionIndex());
      element.x = x.getSelection();
      element.y = y.getSelection();
      element.width = width.getSelection();
      element.depth = depth.getSelection();
      element.rotation = rotation.getSelection();
      super.okPressed();
   }
}
