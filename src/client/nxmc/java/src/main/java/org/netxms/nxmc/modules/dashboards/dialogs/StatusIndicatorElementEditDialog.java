/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
package org.netxms.nxmc.modules.dashboards.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.config.StatusIndicatorConfig;
import org.netxms.nxmc.modules.dashboards.config.StatusIndicatorConfig.StatusIndicatorElementConfig;
import org.netxms.nxmc.modules.datacollection.widgets.DciSelector;
import org.netxms.nxmc.modules.datacollection.widgets.TemplateDciSelector;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for editing status indicator element properties
 */
public class StatusIndicatorElementEditDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(StatusIndicatorElementEditDialog.class);

   private StatusIndicatorElementConfig element;
   private LabeledText label;
   private Combo typeSelector;
   private Composite typeSpecificControl;
   private ObjectSelector objectSelector;
   private DciSelector dciSelector;
   private TemplateDciSelector templateDciSelector;
   private LabeledText tagEditor;
   private String cachedDciName;

   public StatusIndicatorElementEditDialog(Shell parentShell, StatusIndicatorElementConfig element)
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
      newShell.setText(element == null ? i18n.tr("Create Element") : i18n.tr("Edit Element"));
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
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      dialogArea.setLayout(layout);

      if (element == null)
         element = new StatusIndicatorElementConfig();

      label = new LabeledText(dialogArea, SWT.NONE);
      label.setLabel("Label");
      label.setText(element.getLabel());
      GridData gd = new GridData(SWT.FILL, SWT.FILL, true, false);
      gd.widthHint = 400;
      label.setLayoutData(gd);

      typeSelector = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, i18n.tr("Type"), WidgetHelper.DEFAULT_LAYOUT_DATA);
      typeSelector.add(i18n.tr("Object"));
      typeSelector.add(i18n.tr("DCI"));
      typeSelector.add(i18n.tr("DCI Template"));
      typeSelector.add(i18n.tr("Script"));
      typeSelector.select(element.getType());
      typeSelector.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));
      typeSelector.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            createTypeSpecificControls();
         }
      });

      typeSpecificControl = new Composite(dialogArea, SWT.NONE);
      typeSpecificControl.setLayout(new FillLayout());
      typeSpecificControl.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));

      createTypeSpecificControls();

      return dialogArea;
   }

   /**
    * Create type specific controls
    */
   private void createTypeSpecificControls()
   {
      for(Control c : typeSpecificControl.getChildren())
         c.dispose();

      switch(typeSelector.getSelectionIndex())
      {
         case StatusIndicatorConfig.ELEMENT_TYPE_DCI:
            dciSelector = new DciSelector(typeSpecificControl, SWT.NONE);
            dciSelector.setLabel(i18n.tr("Data collection item"));
            dciSelector.setDciId(element.getObjectId(), element.getDciId());
            break;
         case StatusIndicatorConfig.ELEMENT_TYPE_DCI_TEMPLATE:
            templateDciSelector = new TemplateDciSelector(typeSpecificControl, SWT.NONE);
            templateDciSelector.setLabel(i18n.tr("DCI name"));
            templateDciSelector.setText(element.getDciName());
            break;
         case StatusIndicatorConfig.ELEMENT_TYPE_OBJECT:
            objectSelector = new ObjectSelector(typeSpecificControl, SWT.NONE, false);
            objectSelector.setLabel(i18n.tr("Object"));
            objectSelector.setObjectClass(AbstractObject.class);
            objectSelector.setObjectId(element.getObjectId());
            break;
         case StatusIndicatorConfig.ELEMENT_TYPE_SCRIPT:
            tagEditor = new LabeledText(typeSpecificControl, SWT.NONE);
            tagEditor.setLabel(i18n.tr("Tag"));
            tagEditor.setText(element.getTag());
            break;
      }

      if (getShell().isVisible())
      {
         typeSpecificControl.getParent().layout(true, true);
         getShell().pack();
      }
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      element.setLabel(label.getText().trim());
      element.setType(typeSelector.getSelectionIndex());
      switch(element.getType())
      {
         case StatusIndicatorConfig.ELEMENT_TYPE_DCI:
            element.setObjectId(dciSelector.getNodeId());
            element.setDciId(dciSelector.getDciId());
            cachedDciName = dciSelector.getDciName();
            break;
         case StatusIndicatorConfig.ELEMENT_TYPE_DCI_TEMPLATE:
            element.setDciName(templateDciSelector.getText());
            break;
         case StatusIndicatorConfig.ELEMENT_TYPE_OBJECT:
            element.setObjectId(objectSelector.getObjectId());
            break;
         case StatusIndicatorConfig.ELEMENT_TYPE_SCRIPT:
            element.setTag(tagEditor.getText().trim());
            break;
      }
      super.okPressed();
   }

   /**
    * Get element.
    *
    * @return the element
    */
   public StatusIndicatorElementConfig getElement()
   {
      return element;
   }

   /**
    * @return the cachedDciName
    */
   public String getCachedDciName()
   {
      return cachedDciName;
   }
}
