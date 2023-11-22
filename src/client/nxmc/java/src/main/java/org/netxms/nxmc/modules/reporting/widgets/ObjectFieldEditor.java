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

import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.reporting.ReportParameter;
import org.netxms.nxmc.base.widgets.ImageHyperlink;
import org.netxms.nxmc.base.widgets.events.HyperlinkAdapter;
import org.netxms.nxmc.base.widgets.events.HyperlinkEvent;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.modules.objects.widgets.helpers.DecoratingObjectLabelProvider;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Field editor for "object" type field
 */
public class ObjectFieldEditor extends ReportFieldEditor
{
   private static final I18n i18n = LocalizationHelper.getI18n(ObjectFieldEditor.class);

	private CLabel text;
	private long objectId = 0;
   private DecoratingObjectLabelProvider labelProvider;

	/**
	 * @param parameter
	 * @param toolkit
	 * @param parent
	 */
   public ObjectFieldEditor(ReportParameter parameter, Composite parent)
	{
      super(parameter, parent);
      labelProvider = new DecoratingObjectLabelProvider();
		addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				labelProvider.dispose();
			}
		});
	}

   /**
    * @see org.netxms.nxmc.modules.reporting.widgets.ReportFieldEditor#createContent(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContent(Composite parent)
	{
      Composite content = new Composite(parent, SWT.BORDER);

		GridLayout layout = new GridLayout();
		layout.numColumns = 3;
		layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		content.setLayout(layout);

		text = new CLabel(content, SWT.NONE);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.TOP;
		text.setLayoutData(gd);
      text.setText(i18n.tr("Any"));

      final ImageHyperlink selectionLink = new ImageHyperlink(content, SWT.NONE);
		selectionLink.setImage(SharedIcons.IMG_FIND);
      selectionLink.setToolTipText(i18n.tr("Select object"));
		selectionLink.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				selectObject();
			}
		});

      final ImageHyperlink clearLink = new ImageHyperlink(content, SWT.NONE);
		clearLink.setImage(SharedIcons.IMG_CLEAR);
      clearLink.setToolTipText(i18n.tr("Clear selection"));
		clearLink.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            objectId = 0;
            text.setText(i18n.tr("Any"));
            text.setImage(null);
         }
      });

		return content;
	}
	
	/**
	 * Select object
	 */
	private void selectObject()
	{
      ObjectSelectionDialog dlg = new ObjectSelectionDialog(getShell());
		dlg.enableMultiSelection(false);
		if (dlg.open() == Window.OK)
		{
			AbstractObject[] objects = dlg.getSelectedObjects(AbstractObject.class);
			if (objects.length > 0)
			{
				objectId = objects[0].getObjectId();
				text.setText(objects[0].getObjectName());
				text.setImage(labelProvider.getImage(objects[0]));
			}
			else
			{
				objectId = 0;
            text.setText(i18n.tr("Any"));
				text.setImage(null);
			}
		}
	}

   /**
    * @see org.netxms.nxmc.modules.reporting.widgets.ReportFieldEditor#getValue()
    */
	@Override
	public String getValue()
	{
		return Long.toString(objectId);
	}
}
