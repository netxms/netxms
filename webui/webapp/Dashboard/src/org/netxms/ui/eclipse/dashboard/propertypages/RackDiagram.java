/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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
package org.netxms.ui.eclipse.dashboard.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.dashboard.propertypages.helpers.RackView;
import org.netxms.ui.eclipse.dashboard.widgets.internal.RackDiagramConfig;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ObjectSelectionDialog;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Rack diagram element properties
 */
public class RackDiagram extends PropertyPage
{
   private final static String[] RACK_VIEWS = { "Full", "Front", "Back" };
   
   private RackDiagramConfig config;
   private ObjectSelector objectSelector;
   private LabeledText title;
   private Button showTitle;
   private Combo view;

   /* (non-Javadoc)
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      config = (RackDiagramConfig)getElement().getAdapter(RackDiagramConfig.class);
      
      Composite dialogArea = new Composite(parent, SWT.NONE);
      
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      dialogArea.setLayout(layout);
      
      objectSelector = new ObjectSelector(dialogArea, SWT.NONE, false);
      objectSelector.setLabel("Rack");
      objectSelector.setClassFilter(ObjectSelectionDialog.createRackSelectionFilter());
      objectSelector.setObjectClass(AbstractObject.class);
      objectSelector.setObjectId(config.getObjectId());
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      objectSelector.setLayoutData(gd);
      
      title = new LabeledText(dialogArea, SWT.NONE);
      title.setLabel("Title");
      title.setText(config.getTitle());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      title.setLayoutData(gd);

      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      view = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, "View", gd);
      view.setItems(RACK_VIEWS); 
      view.setText(RACK_VIEWS[config.getView().getValue()]);
      
      showTitle = new Button(dialogArea, SWT.CHECK);
      showTitle.setText("Show title");
      showTitle.setSelection(config.isShowTitle());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      showTitle.setLayoutData(gd);
      
      return dialogArea;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
   @Override
   public boolean performOk()
   {
      config.setObjectId(objectSelector.getObjectId());
      config.setTitle(title.getText());
      config.setShowTitle(showTitle.getSelection());
      config.setView(RackView.getByValue(view.getSelectionIndex()));
      return true;
   }
}
