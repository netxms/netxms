/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.views.elements;

import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.xnap.commons.i18n.I18n;

/**
 * Show object's comments
 */
public class Comments extends OverviewPageElement
{
   private final I18n i18n = LocalizationHelper.getI18n(Comments.class);

	private Text comments;
   private boolean showEmptyComments;

	/**
	 * The constructor
	 * 
	 * @param parent
	 * @param anchor
	 * @param objectView
	 */
   public Comments(Composite parent, OverviewPageElement anchor, ObjectView objectView)
	{
		super(parent, anchor, objectView);
      showEmptyComments = !Registry.getSession().getClientConfigurationHintAsBoolean("ObjectOverview.ShowCommentsOnlyIfPresent", true);
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#createClientArea(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createClientArea(Composite parent)
	{
		comments = new Text(parent, SWT.MULTI | SWT.READ_ONLY | SWT.WRAP);
		comments.setBackground(getDisplay().getSystemColor(SWT.COLOR_LIST_BACKGROUND));
		if (getObject() != null)
			comments.setText(getObject().getComments());
		return comments;
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#getTitle()
    */
	@Override
	protected String getTitle()
	{
      return i18n.tr("Comments");
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#onObjectChange()
    */
	@Override
	protected void onObjectChange()
	{
		if (getObject() != null)
			comments.setText(getObject().getComments());
	}

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#isApplicableForObject(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public boolean isApplicableForObject(AbstractObject object)
   {
      return showEmptyComments || !object.getComments().isBlank();
   }
}
