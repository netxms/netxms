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
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.constants.ColumnFilterType;
import org.netxms.client.log.ColumnFilter;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.base.widgets.helpers.SelectorConfigurator;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.xnap.commons.i18n.I18n;

/**
 * Condition editor for object columns
 */
public class ObjectConditionEditor extends ConditionEditor
{
   private final I18n i18n = LocalizationHelper.getI18n(ObjectConditionEditor.class);
   private final String[] OPERATIONS = { i18n.tr("IS"), i18n.tr("IS NOT"), i18n.tr("WITHIN"), i18n.tr("NOT WITHIN") };

   private ObjectSelector objectSelector;

	/**
	 * @param parent
	 */
   public ObjectConditionEditor(Composite parent)
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
      objectSelector = new ObjectSelector(this, SWT.NONE, new SelectorConfigurator().setShowLabel(false));
      objectSelector.setObjectClass(AbstractObject.class);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.CENTER;
      objectSelector.setLayoutData(gd);

      if (initialFilter != null)
      {
         switch(initialFilter.getType())
         {
            case EQUALS:
               setSelectedOperation(initialFilter.isNegated() ? 1 : 0);
               objectSelector.setObjectId(initialFilter.getNumericValue());
               break;
            case CHILDOF:
               setSelectedOperation(initialFilter.isNegated() ? 3 : 2);
               objectSelector.setObjectId(initialFilter.getNumericValue());
               break;
            default:
               break;
         }
      }
	}

	/**
	 * @see org.netxms.nxmc.modules.logviewer.widgets.ui.eclipse.logviewer.widgets.ConditionEditor#createFilter()
	 */
	@Override
	public ColumnFilter createFilter()
	{
		int op = getSelectedOperation();
      ColumnFilter filter = new ColumnFilter(((op == 2) || (op == 3)) ? ColumnFilterType.CHILDOF : ColumnFilterType.EQUALS, objectSelector.getObjectId());
		filter.setNegated((op == 1) || (op == 3));
		return filter;
	}
}
