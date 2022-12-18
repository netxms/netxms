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
package org.netxms.nxmc.modules.logviewer.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.constants.ColumnFilterType;
import org.netxms.client.log.ColumnFilter;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Condition editor for timestamp columns
 */
public class IntegerConditionEditor extends ConditionEditor
{
   private final I18n i18n = LocalizationHelper.getI18n(IntegerConditionEditor.class);
   private final String[] OPERATIONS = { i18n.tr("EQUAL"), i18n.tr("NOT EQUAL"), "<", "<=", ">=", ">", i18n.tr("BETWEEN") };

	private Text value1;
	private Text value2;
	private Label andLabel;
	
	/**
	 * @param parent
	 */
   public IntegerConditionEditor(Composite parent)
	{
      super(parent);
	}

   /**
    * @see org.netxms.nxmc.modules.logviewer.widgets.ui.eclipse.logviewer.widgets.ConditionEditor#getOperations()
    */
	@Override
	protected String[] getOperations()
	{
		return OPERATIONS;
	}

   /**
    * @see org.netxms.nxmc.modules.logviewer.widgets.ui.eclipse.logviewer.widgets.ConditionEditor#createContent(org.netxms.client.log.ColumnFilter)
    */
	@Override
   protected void createContent(ColumnFilter initialFilter)
	{
		Composite group = new Composite(this, SWT.NONE);
      group.setBackground(getBackground());
      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.numColumns = 3;
		group.setLayout(layout);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.CENTER;
		group.setLayoutData(gd);

      value1 = new Text(group, SWT.BORDER);
      value1.setText("0");
      gd = new GridData();
      gd.widthHint = 90;
      gd.verticalAlignment = SWT.CENTER;
      value1.setLayoutData(gd);

      andLabel = new Label(group, SWT.NONE);
      andLabel.setText(i18n.tr("  and  "));
      andLabel.setBackground(getBackground());
		andLabel.setVisible(false);
      gd = new GridData();
      gd.verticalAlignment = SWT.CENTER;
      andLabel.setLayoutData(gd);

      value2 = new Text(group, SWT.BORDER);
      value2.setText("0");
      gd = new GridData();
      gd.widthHint = 90;
      gd.verticalAlignment = SWT.CENTER;
      value2.setLayoutData(gd);
		value2.setVisible(false);

      if (initialFilter != null)
      {
         switch(initialFilter.getType())
         {
            case EQUALS:
               setSelectedOperation(initialFilter.isNegated() ? 1 : 0);
               value1.setText(Long.toString(initialFilter.getNumericValue()));
               break;
            case GREATER:
               setSelectedOperation(initialFilter.isNegated() ? 3 : 5);
               value1.setText(Long.toString(initialFilter.getNumericValue()));
               break;
            case LESS:
               setSelectedOperation(initialFilter.isNegated() ? 4 : 2);
               value1.setText(Long.toString(initialFilter.getNumericValue()));
               break;
            case RANGE:
               setSelectedOperation(6);
               andLabel.setVisible(true);
               value2.setVisible(true);
               value1.setText(Long.toString(initialFilter.getRangeFrom()));
               value2.setText(Long.toString(initialFilter.getRangeTo()));
               break;
            default:
               break;
         }
      }
   }

   /**
    * @see org.netxms.nxmc.modules.logviewer.widgets.ui.eclipse.logviewer.widgets.ConditionEditor#operationSelectionChanged(int)
    */
	@Override
	protected void operationSelectionChanged(int selectionIndex)
	{
		if (selectionIndex == 6)	// between
		{
			andLabel.setVisible(true);
			value2.setVisible(true);
		}
		else
		{
			andLabel.setVisible(false);
			value2.setVisible(false);
		}
	}

   /**
    * @see org.netxms.nxmc.modules.logviewer.widgets.ui.eclipse.logviewer.widgets.ConditionEditor#createFilter()
    */
	@Override
	public ColumnFilter createFilter()
	{
		long n1;
		try
		{
			n1 = Long.parseLong(value1.getText());
		}
		catch(NumberFormatException e)
		{
			n1 = 0;
		}

		ColumnFilter filter;
		switch(getSelectedOperation())
		{
			case 1:	// not equal
				filter = new ColumnFilter(ColumnFilterType.EQUALS, n1);
				filter.setNegated(true);
				break;
			case 2:	// less
				filter = new ColumnFilter(ColumnFilterType.LESS, n1);
				break;
			case 3:	// less or equal
				filter = new ColumnFilter(ColumnFilterType.GREATER, n1);
				filter.setNegated(true);
				break;
			case 4:	// greater or equal
				filter = new ColumnFilter(ColumnFilterType.LESS, n1);
				filter.setNegated(true);
				break;
			case 5:	// greater
				filter = new ColumnFilter(ColumnFilterType.GREATER, n1);
				break;
			case 6:	// between
				long n2;
				try
				{
					n2 = Long.parseLong(value2.getText());
				}
				catch(NumberFormatException e)
				{
					n2 = 0;
				}
				filter = new ColumnFilter(n1, n2);
				break;
			default:
				filter = new ColumnFilter(ColumnFilterType.EQUALS, n1);
				break;
		}
		return filter;
	}
}
