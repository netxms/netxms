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
package org.netxms.ui.eclipse.dashboard.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.dashboard.widgets.TitleConfigurator;
import org.netxms.ui.eclipse.dashboard.widgets.internal.AlarmViewerConfig;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;

/**
 * Alarm viewer element properties
 */
public class AlarmViewer extends PropertyPage
{
   private static final String[] STATE_NAME = { "Outstanding", "Acknowledged", "Resolved" };

	private AlarmViewerConfig config;
	private ObjectSelector objectSelector;
   private TitleConfigurator title;
	private Button[] checkSeverity;
   private Button[] checkState;
   private Button checkEnableLocalSound;

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		config = (AlarmViewerConfig)getElement().getAdapter(AlarmViewerConfig.class);

		Composite dialogArea = new Composite(parent, SWT.NONE);

		GridLayout layout = new GridLayout();
		dialogArea.setLayout(layout);
		
      title = new TitleConfigurator(dialogArea, config);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      title.setLayoutData(gd);

      objectSelector = new ObjectSelector(dialogArea, SWT.NONE, true, true);
      objectSelector.setLabel(Messages.get().AlarmViewer_RootObject);
		objectSelector.setObjectClass(AbstractObject.class);
		objectSelector.setObjectId(config.getObjectId());
      gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		objectSelector.setLayoutData(gd);

		Group severityGroup = new Group(dialogArea, SWT.NONE);
		severityGroup.setText(Messages.get().AlarmViewer_SeverityFilter);
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
      stateGroup.setText("State filter");
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
		checkEnableLocalSound.setText("Play alarm sounds when active");
		checkEnableLocalSound.setSelection(config.getIsLocalSoundEnabled());

		return dialogArea;
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
	@Override
	public boolean performOk()
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
