/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.events.EventTemplate;
import org.netxms.client.reporting.ReportParameter;
import org.netxms.nxmc.base.widgets.ImageHyperlink;
import org.netxms.nxmc.base.widgets.events.HyperlinkAdapter;
import org.netxms.nxmc.base.widgets.events.HyperlinkEvent;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.events.dialogs.EventSelectionDialog;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Field editor for "event code" type field
 */
public class EventFieldEditor extends ReportFieldEditor
{
   private I18n i18n;
   private CLabel text;
	private long eventCode = 0;

	/**
	 * @param parameter
	 * @param toolkit
	 * @param parent
	 */
   public EventFieldEditor(ReportParameter parameter, Composite parent)
	{
      super(parameter, parent);
	}

   /**
    * @see org.netxms.nxmc.modules.reporting.widgets.ReportFieldEditor#setupLocalization()
    */
   @Override
   protected void setupLocalization()
   {
      i18n = LocalizationHelper.getI18n(EventFieldEditor.class);
   }

   /**
    * @see org.netxms.ReportFieldEditor.eclipse.reporter.widgets.FieldEditor#createContent(org.eclipse.swt.widgets.Composite)
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
      selectionLink.setToolTipText(i18n.tr("Select event"));
		selectionLink.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				selectEvent();
			}
		});

      final ImageHyperlink clearLink = new ImageHyperlink(content, SWT.NONE);
		clearLink.setImage(SharedIcons.IMG_CLEAR);
      clearLink.setToolTipText(i18n.tr("Clear selection"));
		clearLink.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            eventCode = 0;
            text.setText(i18n.tr("Any"));
            text.setImage(null);
         }
      });
		
		return content;
	}
	
	/**
	 * Select event
	 */
	private void selectEvent()
	{
		EventSelectionDialog dlg = new EventSelectionDialog(getShell());
		dlg.enableMultiSelection(false);
		if (dlg.open() == Window.OK)
		{
		   EventTemplate[] events = dlg.getSelectedEvents();
			if (events.length > 0)
			{
				eventCode = events[0].getCode();
				text.setText(events[0].getName());
				if (events[0] instanceof EventTemplate)
				   text.setImage(StatusDisplayInfo.getStatusImage(((EventTemplate)events[0]).getSeverity()));
			}
			else
			{
				eventCode = 0;
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
		return Long.toString(eventCode);
	}
}
