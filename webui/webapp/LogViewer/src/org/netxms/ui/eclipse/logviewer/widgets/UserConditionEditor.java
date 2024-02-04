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
package org.netxms.ui.eclipse.logviewer.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.constants.ColumnFilterType;
import org.netxms.client.log.ColumnFilter;
import org.netxms.ui.eclipse.logviewer.Messages;
import org.netxms.ui.eclipse.usermanager.widgets.UserSelector;

/**
 * Condition editor for user columns
 */
public class UserConditionEditor extends ConditionEditor
{
	private final String[] OPERATIONS = { Messages.get().UserConditionEditor_Is, Messages.get().UserConditionEditor_IsNot };

   private UserSelector userSelector;

	/**
	 * @param parent
	 */
   public UserConditionEditor(Composite parent)
	{
      super(parent);
	}

   /**
    * @see org.netxms.ui.eclipse.logviewer.widgets.ConditionEditor#getOperations()
    */
	@Override
	protected String[] getOperations()
	{
		return OPERATIONS;
	}

   /**
    * @see org.netxms.ui.eclipse.logviewer.widgets.ConditionEditor#createContent(org.netxms.client.log.ColumnFilter)
    */
	@Override
   protected void createContent(ColumnFilter initialFilter)
	{
      userSelector = new UserSelector(this, SWT.NONE, UserSelector.HIDE_LABEL);
      userSelector.setBackground(getBackground());
      GridData gd = new GridData();
      gd.verticalAlignment = SWT.CENTER;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      userSelector.setLayoutData(gd);

      if ((initialFilter != null) && (initialFilter.getType() == ColumnFilterType.EQUALS))
      {
         setSelectedOperation(initialFilter.isNegated() ? 1 : 0);
         userSelector.setUserId((int)initialFilter.getNumericValue());
      }
	}

   /**
    * @see org.netxms.ui.eclipse.logviewer.widgets.ConditionEditor#createFilter()
    */
	@Override
	public ColumnFilter createFilter()
	{
      ColumnFilter filter = new ColumnFilter(ColumnFilterType.EQUALS, userSelector.getUserId());
		filter.setNegated(getSelectedOperation() == 1);
		return filter;
	}
}
