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
package org.netxms.nxmc.modules.reporting.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.reporting.ReportParameter;
import org.netxms.nxmc.base.widgets.helpers.SelectorConfigurator;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;

/**
 * Field editor for "object" type field
 */
public class ObjectFieldEditor extends ReportFieldEditor
{
   private ObjectSelector objectSelector;

	/**
	 * @param parameter
	 * @param toolkit
	 * @param parent
	 */
   public ObjectFieldEditor(ReportParameter parameter, Composite parent)
	{
      super(parameter, parent);
	}

   /**
    * @see org.netxms.nxmc.modules.reporting.widgets.ReportFieldEditor#setupLocalization()
    */
   @Override
   protected void setupLocalization()
   {
   }

   /**
    * @see org.netxms.nxmc.modules.reporting.widgets.ReportFieldEditor#createContent(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContent(Composite parent)
	{
      objectSelector = new ObjectSelector(parent, SWT.NONE, new SelectorConfigurator().setShowLabel(false).setShowClearButton(true));
      objectSelector.removeSelectionClassFilter();
      return objectSelector;
	}

   /**
    * @see org.netxms.nxmc.modules.reporting.widgets.ReportFieldEditor#getValue()
    */
	@Override
	public String getValue()
	{
      return Long.toString(objectSelector.getObjectId());
	}
}
