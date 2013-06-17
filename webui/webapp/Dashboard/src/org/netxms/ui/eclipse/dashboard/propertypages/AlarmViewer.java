/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
import org.netxms.ui.eclipse.dashboard.widgets.internal.AlarmViewerConfig;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Alarm viewer element properties
 */
public class AlarmViewer extends PropertyPage
{
	private AlarmViewerConfig config;
	private ObjectSelector objectSelector;
	private LabeledText title;
	private Button[] checkSeverity;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		config = (AlarmViewerConfig)getElement().getAdapter(AlarmViewerConfig.class);
		
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		dialogArea.setLayout(layout);
		
		objectSelector = new ObjectSelector(dialogArea, SWT.NONE, true);
		objectSelector.setLabel(Messages.AlarmViewer_RootObject);
		objectSelector.setObjectClass(AbstractObject.class);
		objectSelector.setObjectId(config.getObjectId());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		objectSelector.setLayoutData(gd);
		
		title = new LabeledText(dialogArea, SWT.NONE);
		title.setLabel(Messages.AlarmViewer_Title);
		title.setText(config.getTitle());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		title.setLayoutData(gd);
		
		Group severityGroup = new Group(dialogArea, SWT.NONE);
		severityGroup.setText("Severity Filter");
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
		
		int severityFilter = 0;
		for(int i = 0; i < checkSeverity.length; i++)
			if (checkSeverity[i].getSelection())
				severityFilter |= (1 << i);
		config.setSeverityFilter(severityFilter);
		
		return true;
	}
}
