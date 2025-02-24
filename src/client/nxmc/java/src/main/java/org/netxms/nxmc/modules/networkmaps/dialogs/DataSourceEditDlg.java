/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.networkmaps.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.client.maps.LinkDataLocation;
import org.netxms.client.maps.configs.MapLinkDataSource;
import org.netxms.client.maps.configs.MapDataSource;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.LabeledCombo;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.widgets.DciSelector;
import org.netxms.nxmc.modules.networkmaps.views.helpers.LinkEditor;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Data source editing dialog for map line
 */
public class DataSourceEditDlg extends Dialog
{
   private I18n i18n = LocalizationHelper.getI18n(DataSourceEditDlg.class);
	private MapDataSource dci;
	private DciSelector dciSelector;
   private LabeledCombo locationSelector;
	private LabeledText instance;
	private LabeledText dataColumn;
	private LabeledText formatString;
   private LinkEditor linkEditor;
   private String dciDescription;
	
	/**
	 * @param parentShell
	 * @param dci
	 */
	public DataSourceEditDlg(Shell parentShell, MapDataSource dci, LinkEditor linkEditor)
	{
		super(parentShell);
		this.dci = dci;
		this.linkEditor = linkEditor;
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
      newShell.setText(i18n.tr("Edit Data Source"));
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
		layout.numColumns = 2;
		dialogArea.setLayout(layout);
		
      dciSelector = new DciSelector(dialogArea, SWT.NONE);
      dciSelector.setLabel(i18n.tr("Data collection item"));
		dciSelector.setDciId(dci.getNodeId(), dci.getDciId());
		dciSelector.setDcObjectType(dci.getType());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = 400;
		gd.horizontalSpan = 2;
		dciSelector.setLayoutData(gd);
      
		formatString = new LabeledText(dialogArea, SWT.NONE);
      formatString.setLabel(i18n.tr("Format string"));
		formatString.setText(dci.getFormatString());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      formatString.setLayoutData(gd);
      
		if (dci.getType() == MapDataSource.TABLE)
		{
			Group tableGroup = new Group(dialogArea, SWT.NONE);
         tableGroup.setText(i18n.tr("Table cell"));
			gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			gd.horizontalSpan = 2;
			tableGroup.setLayoutData(gd);

			layout = new GridLayout();
			tableGroup.setLayout(layout);

			dataColumn = new LabeledText(tableGroup, SWT.NONE);
         dataColumn.setLabel(i18n.tr("Data column"));
			dataColumn.setText(dci.getColumn());
			gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			dataColumn.setLayoutData(gd);

			instance = new LabeledText(tableGroup, SWT.NONE);
         instance.setLabel(i18n.tr("Instance"));
			instance.setText(dci.getInstance());
			gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			instance.setLayoutData(gd);
		}

		if (dci instanceof MapLinkDataSource)
		{
		   NXCSession session = Registry.getSession();
		   
   		locationSelector = new LabeledCombo(dialogArea, SWT.NONE);
   		locationSelector.setLabel(i18n.tr("Position on the link"));
         locationSelector.add(i18n.tr("Center"));
   		locationSelector.add(session.getObjectNameWithAlias(linkEditor.getElement1()));
   		locationSelector.add(session.getObjectNameWithAlias(linkEditor.getElement2()));
   		locationSelector.select(((MapLinkDataSource)dci).getLocation().getValue());
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         locationSelector.setLayoutData(gd);
		}
		
		return dialogArea;
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
	   if ((dci instanceof MapLinkDataSource))
	   {
   	   if ((dci.getNodeId() != dciSelector.getNodeId()) || (dci.getDciId() != dciSelector.getDciId()) ||
   	         !dci.getFormatString().equals(formatString.getText()) ||
   	         ((dci.getType() == MapDataSource.TABLE) && (!dci.getColumn().equals(dataColumn.getText().trim()) || !dci.getInstance().equals(instance.getText()))) ||
   	         ((MapLinkDataSource)dci).getLocation() != LinkDataLocation.getByValue(locationSelector.getSelectionIndex()))
   	   {
   	      ((MapLinkDataSource)dci).setSystem(false);
   	   }
	   }
	   
		dci.setNodeId(dciSelector.getNodeId());
		dci.setDciId(dciSelector.getDciId());
		dciDescription = dciSelector.getDciDescription();
		dci.setFormatString(formatString.getText());
		if (dci.getType() == MapDataSource.TABLE)
		{
			dci.setColumn(dataColumn.getText().trim());
			dci.setInstance(instance.getText());
		}	
      if (dci instanceof MapLinkDataSource)
      {
         ((MapLinkDataSource)dci).setLocation(LinkDataLocation.getByValue(locationSelector.getSelectionIndex()));
      }
		super.okPressed();
	}

   /**
    * Get selected DCI name
    * 
    * @return DCI name
    */
   public String getDciDescription()
   {
      return dciDescription;
   }
}
