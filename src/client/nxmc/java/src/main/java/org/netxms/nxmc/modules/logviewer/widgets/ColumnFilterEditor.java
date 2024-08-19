/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.logviewer.widgets;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.constants.ColumnFilterSetOperation;
import org.netxms.client.log.ColumnFilter;
import org.netxms.client.log.LogColumn;
import org.netxms.nxmc.base.widgets.DashboardComposite;
import org.netxms.nxmc.base.widgets.ImageHyperlink;
import org.netxms.nxmc.base.widgets.events.HyperlinkAdapter;
import org.netxms.nxmc.base.widgets.events.HyperlinkEvent;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.SharedIcons;
import org.xnap.commons.i18n.I18n;

/**
 * Widget for editing filter for single column
 */
public class ColumnFilterEditor extends DashboardComposite
{
   private final I18n i18n = LocalizationHelper.getI18n(ColumnFilterEditor.class);

	private LogColumn column;
	private FilterBuilder filterBuilder;
	private List<ConditionEditor> conditions = new ArrayList<ConditionEditor>();
	private ColumnFilterSetOperation booleanOperation = ColumnFilterSetOperation.OR;

	/**
	 * @param parent
	 * @param style
	 * @param columnElement 
	 * @param column 
	 */
   public ColumnFilterEditor(Composite parent, LogColumn column, ColumnFilterSetOperation operation, final Runnable deleteHandler)
	{
		super(parent, SWT.BORDER);

		this.column = column;
		this.booleanOperation = operation;

		GridLayout layout = new GridLayout();
		setLayout(layout);

      final Composite header = new Composite(this, SWT.NONE);
      header.setBackground(getBackground());
		layout = new GridLayout();
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 5;
		header.setLayout(layout);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		header.setLayoutData(gd);

      final Label title = new Label(header, SWT.NONE);
      title.setText(column.getDescription());
      title.setBackground(header.getBackground());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = false;
		title.setLayoutData(gd);

      final Composite buttons = new Composite(header, SWT.NONE);
      buttons.setBackground(header.getBackground());
		layout = new GridLayout();
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 2;
		buttons.setLayout(layout);
		gd = new GridData();
		gd.horizontalAlignment = SWT.CENTER;
		gd.grabExcessHorizontalSpace = true;
		buttons.setLayoutData(gd);

      final Button radioAnd = new Button(buttons, SWT.RADIO);
      radioAnd.setText(i18n.tr("&AND condition"));
      radioAnd.setBackground(buttons.getBackground());
		radioAnd.setSelection(booleanOperation == ColumnFilterSetOperation.AND);
      radioAnd.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				setBooleanOperation(ColumnFilterSetOperation.AND);
			}
		});

      final Button radioOr = new Button(buttons, SWT.RADIO);
      radioOr.setText(i18n.tr("&OR condition"));
      radioOr.setBackground(buttons.getBackground());
		radioOr.setSelection(booleanOperation == ColumnFilterSetOperation.OR);
      radioOr.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				setBooleanOperation(ColumnFilterSetOperation.OR);
			}
		});

      ImageHyperlink link = new ImageHyperlink(header, SWT.NONE);
		link.setImage(SharedIcons.IMG_ADD_OBJECT);
      link.setBackground(header.getBackground());
		link.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				addCondition(null);
			}
		});

      link = new ImageHyperlink(header, SWT.NONE);
		link.setImage(SharedIcons.IMG_DELETE_OBJECT);
      link.setBackground(header.getBackground());
		link.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				ColumnFilterEditor.this.dispose();
				deleteHandler.run();
			}
		});
	}

	/**
	 * @param op
	 */
	private void setBooleanOperation(ColumnFilterSetOperation op)
	{
		booleanOperation = op;
      final String opName = (op == ColumnFilterSetOperation.AND) ? i18n.tr("AND") : i18n.tr("OR");
		for(int i = 1; i < conditions.size(); i++)
		{
			conditions.get(i).setLogicalOperation(opName);
		}
	}

	/**
	 * Add condition
	 */
	private void addCondition(ColumnFilter initialFilter)
	{
		final ConditionEditor ce = createConditionEditor(initialFilter);
		ce.setDeleteHandler(new Runnable() {
			@Override
			public void run()
			{
				conditions.remove(ce);
				if (conditions.size() > 0)
					conditions.get(0).setLogicalOperation(""); //$NON-NLS-1$
				filterBuilder.updateLayout();
			}
		});

		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		ce.setLayoutData(gd);
		conditions.add(ce);
		if (conditions.size() > 1)
         ce.setLogicalOperation((booleanOperation == ColumnFilterSetOperation.AND) ? i18n.tr("AND") : i18n.tr("OR"));

		filterBuilder.updateLayout();
	}

	/**
	 * @param currentElement
	 * @return
	 */
	private ConditionEditor createConditionEditor(ColumnFilter initialFilter)
	{
	   ConditionEditor editor;
		switch(column.getType())
		{
			case LogColumn.LC_ALARM_HD_STATE:
            editor = new AlarmHDStateConditionEditor(this);
				break;
			case LogColumn.LC_ALARM_STATE:
            editor = new AlarmStateConditionEditor(this);
            break;
         case LogColumn.LC_ASSET_OPERATION:
            editor = new AssetOperationConditionEditor(this);
            break;
         case LogColumn.LC_COMPLETION_STATUS:
            editor = new CompletionStatusConditionEditor(this);
            break;
			case LogColumn.LC_EVENT_CODE:
            editor = new EventConditionEditor(this);
            break;
         case LogColumn.LC_EVENT_ORIGIN:
            editor = new EventOriginConditionEditor(this);
            break;
			case LogColumn.LC_INTEGER:
            editor = new IntegerConditionEditor(this);
            break;
			case LogColumn.LC_OBJECT_ID:
            editor = new ObjectConditionEditor(this);
            break;
			case LogColumn.LC_SEVERITY:
            editor = new SeverityConditionEditor(this);
            break;
			case LogColumn.LC_TIMESTAMP:
            editor = new TimestampConditionEditor(this);
            break;
			case LogColumn.LC_USER_ID:
            editor = new UserConditionEditor(this);
            break;
         case LogColumn.LC_ZONE_UIN:
            editor = new ZoneConditionEditor(this);
            break;
			default:
            editor = new TextConditionEditor(this);
            break;
		}
		editor.initialize(initialFilter);
		return editor;
	}

	/**
	 * @param filterBuilder the filterBuilder to set
	 */
	public void attachFilterBuilder(FilterBuilder filterBuilder, ColumnFilter initialFilter)
	{
		this.filterBuilder = filterBuilder;
      if ((initialFilter == null) || (initialFilter.getSubFilters() == null) || initialFilter.getSubFilters().isEmpty())
      {
         addCondition( (initialFilter != null) ? initialFilter : null);
      }
      else
      {
         for(ColumnFilter f : initialFilter.getSubFilters())
         {
            f.setOperation(f.getOperation());
            addCondition(f);
         }
      }
	}

	/**
	 * Build filter tree
	 */
	public ColumnFilter buildFilterTree()
	{
		if (conditions.size() == 0)
			return null;
		
		if (conditions.size() > 1)
		{
			ColumnFilter root = new ColumnFilter();
			root.setOperation(booleanOperation);
			for(ConditionEditor e : conditions)
			{
				ColumnFilter filter = e.createFilter();
				root.addSubFilter(filter);
			}
			return root;
		}
		else
		{
			return conditions.get(0).createFilter();
		}
	}
}
