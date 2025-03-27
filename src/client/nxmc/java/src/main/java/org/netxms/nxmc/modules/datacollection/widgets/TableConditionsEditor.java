/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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
package org.netxms.nxmc.modules.datacollection.widgets;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CCombo;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.datacollection.TableCondition;
import org.netxms.nxmc.base.widgets.DashboardComposite;
import org.netxms.nxmc.base.widgets.ImageHyperlink;
import org.netxms.nxmc.base.widgets.events.HyperlinkAdapter;
import org.netxms.nxmc.base.widgets.events.HyperlinkEvent;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.TableColumnEnumerator;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Table threshold conditions editor
 */
public class TableConditionsEditor extends Composite
{
   private I18n i18n = LocalizationHelper.getI18n(TableConditionsEditor.class);

	private ScrolledComposite scroller;
	private Composite editorsArea;
	private ImageHyperlink addColumnLink;
	private List<GroupEditor> groups = new ArrayList<GroupEditor>();
   private TableColumnEnumerator columnEnumerator = null;
	private List<String> columnList;

	/**
	 * @param parent
	 * @param style
	 */
   public TableConditionsEditor(Composite parent, int style, TableColumnEnumerator columnEnumerator)
	{
		super(parent, style);
      this.columnEnumerator = columnEnumerator;

		setLayout(new FillLayout());

		scroller = new ScrolledComposite(this, SWT.NONE);
		scroller.setExpandHorizontal(true);
		scroller.setExpandVertical(true);
      WidgetHelper.setScrollBarIncrement(scroller, SWT.VERTICAL, 20);
      scroller.addControlListener(new ControlAdapter() {
         public void controlResized(ControlEvent e)
         {
            editorsArea.layout(true, true);
            scroller.setMinSize(editorsArea.computeSize(scroller.getClientArea().width, SWT.DEFAULT));
         }
      });
		
		editorsArea = new Composite(scroller, SWT.NONE);
      GridLayout layout = new GridLayout();
      editorsArea.setLayout(layout);

      scroller.setContent(editorsArea);

		addColumnLink = new ImageHyperlink(editorsArea, SWT.NONE);
		addColumnLink.setText(i18n.tr("Add..."));
		addColumnLink.setImage(SharedIcons.IMG_ADD_OBJECT);
		addColumnLink.setToolTipText(i18n.tr("Add new condition group"));
		addColumnLink.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				addGroup(addColumnLink, null);
			}
		});
	}

	/**
	 * @param conditions
	 */
	public void setConditions(List<List<TableCondition>> conditions)
	{
		for(List<TableCondition> g : conditions)
		{
			addGroup(addColumnLink, g);
		}
		editorsArea.layout(true);
	}

	/**
	 * @return
	 */
	public List<List<TableCondition>> getConditions()
	{
		List<List<TableCondition>> result = new ArrayList<List<TableCondition>>(groups.size());
		for(GroupEditor ge : groups)
		{
			result.add(ge.getConditions());
		}
		return result;
	}

	/**
	 * Add condition group
	 */
	private void addGroup(final Control lastControl, List<TableCondition> initialData)
	{
		final GroupEditor editor = new GroupEditor(editorsArea);
		editor.moveAbove(lastControl);
		editor.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));
		groups.add(editor);
		if (initialData != null)
		{
			for(TableCondition c : initialData)
			{
				editor.addCondition(c);
			}
		}
		else
		{
		   editorsArea.layout(true);
		}
	}
	
	/**
	 * Delete condition group
	 * 
	 * @param editor
	 */
	private void deleteGroup(final GroupEditor editor)
	{
		groups.remove(editor);
		editor.dispose();
		editorsArea.layout(true);
	}
	
	/**
	 * Group editor widget
	 */
	private class GroupEditor extends DashboardComposite
	{
		private Composite content;
		private List<ConditionEditor> conditions = new ArrayList<ConditionEditor>();
		
		public GroupEditor(Composite parent)
		{
			super(parent, SWT.BORDER);
			GridLayout layout = new GridLayout();
			layout.numColumns = 2;
			setLayout(layout);

			content = new Composite(this, SWT.NONE);
			layout = new GridLayout();
			layout.marginWidth = 0;
			layout.marginHeight = 0;
			layout.numColumns = 4;
			content.setLayout(layout);
			content.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

			Composite buttons = new Composite(this, SWT.NONE);
			layout = new GridLayout();
			layout.numColumns = 2;
			layout.marginWidth = 0;
			layout.marginHeight = 0;
			buttons.setLayout(layout);
			buttons.setLayoutData(new GridData(SWT.FILL, SWT.TOP, false, false));

			ImageHyperlink link = new ImageHyperlink(buttons, SWT.NONE);
			link.setImage(SharedIcons.IMG_DELETE_OBJECT);
			link.setToolTipText(i18n.tr("Delete condition group"));
			link.addHyperlinkListener(new HyperlinkAdapter() {
				@Override
				public void linkActivated(HyperlinkEvent e)
				{
					deleteGroup(GroupEditor.this);
				}
			});

			link = new ImageHyperlink(this, SWT.NONE);
			link.setImage(SharedIcons.IMG_ADD_OBJECT);
			link.setText(i18n.tr("Add..."));
			link.setToolTipText(i18n.tr("Add new condition"));
			link.addHyperlinkListener(new HyperlinkAdapter() {
				@Override
				public void linkActivated(HyperlinkEvent e)
				{
					addCondition(null);
				}
			});

		}

		/**
       * Get configured conditions
       * 
       * @return configured conditions
       */
		public List<TableCondition> getConditions()
		{
			List<TableCondition> result = new ArrayList<TableCondition>(conditions.size());
			for(ConditionEditor ce : conditions)
			{
				result.add(ce.getCondition());
			}
			return result;
		}

		/**
		 * Add condition to group
		 */
		private void addCondition(TableCondition initialData)
		{
			ConditionEditor editor = new ConditionEditor(content, this);
			conditions.add(editor);
			if (initialData != null)
			{
				editor.column.select(columnEnumerator == null ? -1 :columnList.indexOf(initialData.getColumn()));
				editor.operation.select(initialData.getOperation());
				editor.value.setText(initialData.getValue());
			}
			else
			{
			   editorsArea.layout(true);
			}
		}
		
		/**
		 * @param editor
		 */
		void deleteCondition(ConditionEditor editor)
		{
			conditions.remove(editor);
			editor.dispose();
			editorsArea.layout(true);
		}
	}
	
	/**
	 * Condition editor widget
	 */
	private class ConditionEditor
	{
		private CCombo column;
		private CCombo operation;
		private Text value;
		
		public ConditionEditor(Composite parent, final GroupEditor group)
		{
			column = new CCombo(parent, SWT.BORDER | SWT.READ_ONLY);
			column.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
			if(columnEnumerator != null)
			{
            columnList = columnEnumerator.getColumns();
   			for(String s : columnList)
   			{
   			   column.add(s);
   			}
			}

			operation = new CCombo(parent, SWT.BORDER | SWT.READ_ONLY);
			operation.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, false, false));
			operation.add(i18n.tr("<  : less than"));
			operation.add(i18n.tr("<= : less than or equal to"));
			operation.add(i18n.tr("== : equal to"));
			operation.add(i18n.tr(">= : greater than or equal to"));
			operation.add(i18n.tr(">  : greater than"));
			operation.add(i18n.tr("!= : not equal to"));
			operation.add(i18n.tr("like"));
			operation.add(i18n.tr("not like"));
         operation.add(i18n.tr("like (ignore case)"));
         operation.add(i18n.tr("not like (ignore case)"));

         value = new Text(parent, SWT.BORDER);
			value.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

			final ImageHyperlink link = new ImageHyperlink(parent, SWT.NONE);
			link.setImage(SharedIcons.IMG_DELETE_OBJECT);
			link.setToolTipText(i18n.tr("Delete condition"));
			link.addHyperlinkListener(new HyperlinkAdapter() {
				@Override
				public void linkActivated(HyperlinkEvent e)
				{
					group.deleteCondition(ConditionEditor.this);
					link.dispose();
				}
			});
			link.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, false, false));
		}

		/**
		 * Dispose condition editor
		 */
		public void dispose()
		{
			column.dispose();
			operation.dispose();
			value.dispose();
		}

		/**
		 * @return
		 */
		public TableCondition getCondition()
		{
			return new TableCondition(columnEnumerator == null? column.getText() : columnList.get(column.getSelectionIndex()), operation.getSelectionIndex(), value.getText());
		}	
	}
}
