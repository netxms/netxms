/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.dashboards.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.config.AlarmViewerConfig;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig;
import org.netxms.nxmc.modules.dashboards.widgets.TitleConfigurator;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.xnap.commons.i18n.I18n;

/**
 * Alarm viewer element properties
 */
public class AlarmViewer extends DashboardElementPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(AlarmViewer.class);
   private static final String[] STATE_NAME = 
         { 
            LocalizationHelper.getI18n(AlarmViewer.class).tr("Outstanding"), 
            LocalizationHelper.getI18n(AlarmViewer.class).tr("Acknowledged"), 
            LocalizationHelper.getI18n(AlarmViewer.class).tr("Resolved") 
         };

	private AlarmViewerConfig config;
	private ObjectSelector objectSelector;
   private TitleConfigurator title;
	private Button[] checkSeverity;
   private Button[] checkState;
   private Button checkEnableLocalSound;

   /**
    * Create page.
    *
    * @param elementConfig element configuration
    */
   public AlarmViewer(DashboardElementConfig elementConfig)
   {
      super(LocalizationHelper.getI18n(AlarmViewer.class).tr("Alarm Viewer"), elementConfig);
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#getId()
    */
   @Override
   public String getId()
   {
      return "alarm-viewer";
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#isVisible()
    */
   @Override
   public boolean isVisible()
   {
      return elementConfig instanceof AlarmViewerConfig;
   }

   /**
    * @see org.netxms.nxmc.modules.dashboards.propertypages.DashboardElementPropertyPage#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 0;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
      config = (AlarmViewerConfig)elementConfig;

		Composite dialogArea = new Composite(parent, SWT.NONE);

		GridLayout layout = new GridLayout();
		dialogArea.setLayout(layout);
		
      title = new TitleConfigurator(dialogArea, config);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      title.setLayoutData(gd);

      objectSelector = new ObjectSelector(dialogArea, SWT.NONE, true, true);
      objectSelector.setLabel(i18n.tr("Root object"));
		objectSelector.setObjectClass(AbstractObject.class);
		objectSelector.setObjectId(config.getObjectId());
      gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		objectSelector.setLayoutData(gd);

		Group severityGroup = new Group(dialogArea, SWT.NONE);
      severityGroup.setText(i18n.tr("Severity filter"));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		severityGroup.setLayoutData(gd);

		layout = new GridLayout();
		layout.numColumns = 5;
		layout.makeColumnsEqualWidth = true;
		severityGroup.setLayout(layout);

		checkSeverity = new Button[5];
		for(int severity = 4; severity >= 0; severity--)
		{
			checkSeverity[severity] = new Button(severityGroup, SWT.CHECK);
			checkSeverity[severity].setText(StatusDisplayInfo.getStatusText(severity));
			checkSeverity[severity].setSelection((config.getSeverityFilter() & (1 << severity)) != 0);
		}

      Group stateGroup = new Group(dialogArea, SWT.NONE);
      stateGroup.setText(i18n.tr("State filter"));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      stateGroup.setLayoutData(gd);

      layout = new GridLayout();
      layout.numColumns = 3;
      layout.makeColumnsEqualWidth = true;
      stateGroup.setLayout(layout);

      checkState = new Button[3];
      for(int i = 0; i < 3; i++)
      {
         checkState[i] = new Button(stateGroup, SWT.CHECK);
         checkState[i].setText(STATE_NAME[i]);
         checkState[i].setSelection((config.getStateFilter() & (1 << i)) != 0);
      }

		checkEnableLocalSound  = new Button(dialogArea, SWT.CHECK);
      checkEnableLocalSound.setText(i18n.tr("Play alarm sounds when active"));
		checkEnableLocalSound.setSelection(config.getIsLocalSoundEnabled());

		return dialogArea;
	}

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
	{
		config.setObjectId(objectSelector.getObjectId());
      title.updateConfiguration(config);

		int severityFilter = 0;
		for(int i = 0; i < checkSeverity.length; i++)
			if (checkSeverity[i].getSelection())
				severityFilter |= (1 << i);
		config.setSeverityFilter(severityFilter);

      int stateFilter = 0;
      for(int i = 0; i < checkState.length; i++)
         if (checkState[i].getSelection())
            stateFilter |= (1 << i);
      config.setStateFilter(stateFilter);

		config.setIsLocalSoundEnabled(checkEnableLocalSound.getSelection());
		return true;
	}
}
