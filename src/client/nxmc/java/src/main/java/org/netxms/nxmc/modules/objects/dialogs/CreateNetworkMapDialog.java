/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.maps.MapType;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.NetworkMap;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for creating new network map object
 */
public class CreateNetworkMapDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(CreateNetworkMapDialog.class);

   private ObjectSelector templateMapSelector;
	private Text textName;
   private Text textAlias;
	private Combo mapType;
	private ObjectSelector seedObjectSelector;
	private String name;
   private String alias;
   private MapType type;
	private long seedObject;
   private long templateMapId;

	/**
	 * @param parentShell
	 */
	public CreateNetworkMapDialog(Shell parentShell)
	{
		super(parentShell);
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
      newShell.setText(i18n.tr("Create Network Map"));
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);

		GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      dialogArea.setLayout(layout);

      textName = WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER, SWT.DEFAULT, i18n.tr("Name"), "", WidgetHelper.DEFAULT_LAYOUT_DATA);
      textName.getShell().setMinimumSize(300, 0);

      textAlias = WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER, SWT.DEFAULT, i18n.tr("Alias"), "", WidgetHelper.DEFAULT_LAYOUT_DATA);
      textAlias.getShell().setMinimumSize(300, 0);

      mapType = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, i18n.tr("Map type"), WidgetHelper.DEFAULT_LAYOUT_DATA);
      mapType.add(i18n.tr("Custom"));
      mapType.add(i18n.tr("Layer 2 topology"));
      mapType.add(i18n.tr("IP topology"));
      mapType.add(i18n.tr("Internal communication topology"));
      mapType.add(i18n.tr("OSPF topology"));
      mapType.add(i18n.tr("Hybrid topology"));
      mapType.select(0);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      mapType.getParent().setLayoutData(gd);
      mapType.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
		      seedObjectSelector.setEnabled(mapType.getSelectionIndex() > 0 && mapType.getSelectionIndex() != 3);
			}
      });

      seedObjectSelector = new ObjectSelector(dialogArea, SWT.NONE, true);
      seedObjectSelector.setLabel(i18n.tr("Seed node"));
      seedObjectSelector.setObjectClass(AbstractObject.class);
      seedObjectSelector.setClassFilter(ObjectSelectionDialog.createNodeSelectionFilter(false));
      seedObjectSelector.setEnabled(false);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      seedObjectSelector.setLayoutData(gd);

      templateMapSelector = new ObjectSelector(dialogArea, SWT.NONE, true);
      templateMapSelector.setLabel(i18n.tr("Template network map"));
      templateMapSelector.setObjectClass(NetworkMap.class);
      templateMapSelector.setClassFilter(ObjectSelectionDialog.createNetworkMapSelectionFilter());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      templateMapSelector.setLayoutData(gd);

		return dialogArea;
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
		name = textName.getText().trim();
      alias = textAlias.getText().trim();
		if (name.isEmpty())
		{
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Object name cannot be empty"));
			return;
		}

      type = MapType.getByValue(mapType.getSelectionIndex());
      if ((type != MapType.CUSTOM) && (type != MapType.INTERNAL_TOPOLOGY))
		{
			seedObject = seedObjectSelector.getObjectId();
			if (seedObject == 0)
			{
            MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Seed node is required for selected map type"));
				return;
			}
		}
      
      templateMapId = templateMapSelector.getObjectId();

		super.okPressed();
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

   /**
    * @return the alias
    */
   public String getAlias()
   {
      return alias;
   }

	/**
	 * @return the type
	 */
   public MapType getType()
	{
		return type;
	}

	/**
	 * @return the seedObject
	 */
	public long getSeedObject()
	{
		return seedObject;
	}

   /**
    * Get ID of template map.
    *
    * @return ID of template map or 0 if not set
    */
   public long getTemplateMapId()
   {
      return templateMapId;
   }
}
