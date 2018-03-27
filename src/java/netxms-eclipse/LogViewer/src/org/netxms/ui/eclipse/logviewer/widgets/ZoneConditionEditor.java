/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2018 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.logviewer.widgets;

import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.eclipse.ui.forms.widgets.ImageHyperlink;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.constants.ColumnFilterType;
import org.netxms.client.log.ColumnFilter;
import org.netxms.client.objects.Zone;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.logviewer.Messages;
import org.netxms.ui.eclipse.objectbrowser.dialogs.ZoneSelectionDialog;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Condition editor for object columns
 */
public class ZoneConditionEditor extends ConditionEditor
{
	private static final String[] OPERATIONS = { Messages.get().ObjectConditionEditor_Is, Messages.get().ObjectConditionEditor_IsNot };
	private static final String EMPTY_SELECTION_TEXT = Messages.get().ObjectConditionEditor_None;
	
	private WorkbenchLabelProvider labelProvider;
	private long zoneUin = 0;
	private CLabel objectName;
	
	/**
	 * @param parent
	 * @param toolkit
	 * @param parentElement
	 */
	public ZoneConditionEditor(Composite parent, FormToolkit toolkit)
	{
		super(parent, toolkit);

		labelProvider = new WorkbenchLabelProvider();
		addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				labelProvider.dispose();
			}
		});
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.logviewer.widgets.ConditionEditor#getOperations()
	 */
	@Override
	protected String[] getOperations()
	{
		return OPERATIONS;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.logviewer.widgets.ConditionEditor#createContent(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected void createContent(Composite parent, ColumnFilter initialFilter)
	{
		Composite group = new Composite(this, SWT.NONE);
		GridLayout layout = new GridLayout();
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.horizontalSpacing = WidgetHelper.INNER_SPACING;
		layout.numColumns = 2;
		group.setLayout(layout);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		group.setLayoutData(gd);

		objectName = new CLabel(group, SWT.NONE);
		toolkit.adapt(objectName);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.TOP;
		objectName.setLayoutData(gd);
		objectName.setText(EMPTY_SELECTION_TEXT);
		
		final ImageHyperlink selectionLink = toolkit.createImageHyperlink(group, SWT.NONE);
		selectionLink.setImage(SharedIcons.IMG_FIND);
		selectionLink.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				selectObject();
			}
		});

      if (initialFilter != null)
      {
         if (initialFilter.getType() == ColumnFilterType.EQUALS)
         {
            setSelectedOperation(initialFilter.isNegated() ? 1 : 0);
            zoneUin = initialFilter.getNumericValue();
         }
         
         if (zoneUin != 0)
         {
            Zone zone = ConsoleSharedData.getSession().findZone(zoneUin);
            if (zone != null)
            {
               objectName.setText(zone.getObjectName());
               objectName.setImage(labelProvider.getImage(zone));
            }
            else
            {
               objectName.setText("[" + zoneUin + "]"); //$NON-NLS-1$ //$NON-NLS-2$
            }
         }
      }
	}
	
	/**
	 * Select object
	 */
	private void selectObject()
	{
		ZoneSelectionDialog dlg = new ZoneSelectionDialog(getShell());
		if (dlg.open() == Window.OK)
		{
			zoneUin = dlg.getZoneUIN();
			if (zoneUin != 0)
			{
				objectName.setText(dlg.getZoneName());
				objectName.setImage(labelProvider.getImage(ConsoleSharedData.getSession().findZone(zoneUin)));
			}
			else
			{
				objectName.setText(EMPTY_SELECTION_TEXT);
				objectName.setImage(null);
			}
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.logviewer.widgets.ConditionEditor#createFilter()
	 */
	@Override
	public ColumnFilter createFilter()
	{
		int op = getSelectedOperation();
		ColumnFilter filter = new ColumnFilter(ColumnFilterType.EQUALS, zoneUin);
		filter.setNegated(op == 1);
		return filter;
	}
}
