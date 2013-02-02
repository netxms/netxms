/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.networkmaps.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Dialog for creating new network map object
 */
public class CreateNetworkMapDialog extends Dialog
{
	private Text textName;
	private Combo mapType;
	private ObjectSelector seedObjectSelector;
	private String name;
	private int type;
	private long seedObject;
	
	/**
	 * @param parentShell
	 */
	public CreateNetworkMapDialog(Shell parentShell)
	{
		super(parentShell);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText("Create Network Map");
	}

	/* (non-Javadoc)
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
		
      textName = WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER, SWT.DEFAULT, "Name", "",
                                                WidgetHelper.DEFAULT_LAYOUT_DATA);
      textName.getShell().setMinimumSize(300, 0);
      
      mapType = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, "Map type", WidgetHelper.DEFAULT_LAYOUT_DATA);
      mapType.add("Custom");
      mapType.add("Layer 2 Topology");
      mapType.add("IP Topology");
      mapType.select(0);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      mapType.getParent().setLayoutData(gd);
      mapType.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
		      seedObjectSelector.setEnabled(mapType.getSelectionIndex() > 0);
			}

			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
      });
      
      seedObjectSelector = new ObjectSelector(dialogArea, SWT.NONE, true);
      seedObjectSelector.setLabel("Seed node");
      seedObjectSelector.setObjectClass(Node.class);
      seedObjectSelector.setEnabled(false);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      seedObjectSelector.setLayoutData(gd);
      
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		name = textName.getText().trim();
		if (name.isEmpty())
		{
			MessageDialog.openWarning(getShell(), "Warning", "Please enter non-empty object name");
			return;
		}
		
		type = mapType.getSelectionIndex();
		if (type > 0)
		{
			seedObject = seedObjectSelector.getObjectId();
			if (seedObject == 0)
			{
				MessageDialog.openWarning(getShell(), "Warning", "Please select seed node");
				return;
			}
		}

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
	 * @return the type
	 */
	public int getType()
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
}
