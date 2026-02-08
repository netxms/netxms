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
package org.netxms.nxmc.modules.events.dialogs;

import java.util.HashSet;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.constants.Severity;
import org.netxms.client.events.EventTemplate;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Event template editing dialog
 */
public class EditEventTemplateDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(EditEventTemplateDialog.class);
   private static final String CONFIG_PREFIX = "EditEventTemplateDialog"; //$NON-NLS-1$

	private EventTemplate eventTemplate;
	private boolean isNew;
	private LabeledText id;
   private LabeledText guid;
	private LabeledText name;
	private LabeledText message;
   private LabeledText tags;
	private LabeledText description;
	private Combo severity;
	private Button optionLog;
   private Button optionDoNotMonitor;

	/**
	 * Default constructor.
	 * 
	 * @param parentShell
	 * @param eventTemplate
	 * @param isNew
	 */
	public EditEventTemplateDialog(Shell parentShell, EventTemplate eventTemplate, boolean isNew)
	{
		super(parentShell);
      setShellStyle(getShellStyle() | SWT.RESIZE);
		this.eventTemplate = eventTemplate;
		this.isNew = isNew;
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);

		GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.numColumns = 3;
      layout.horizontalSpacing = WidgetHelper.OUTER_SPACING * 2;
      dialogArea.setLayout(layout);

      id = new LabeledText(dialogArea, SWT.NONE);
      id.setLabel(i18n.tr("Event code"));
      id.setText(Long.toString(eventTemplate.getCode()));
      id.getTextControl().setEditable(false);

      guid = new LabeledText(dialogArea, SWT.NONE);
      guid.setLabel(i18n.tr("GUID"));
      guid.setText(eventTemplate.getGuid().toString());
      guid.getTextControl().setEditable(false);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      guid.setLayoutData(gd);

      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      severity = WidgetHelper.createLabeledCombo(dialogArea, SWT.DROP_DOWN | SWT.READ_ONLY, i18n.tr("Severity"), gd);
      severity.add(StatusDisplayInfo.getStatusText(Severity.NORMAL));
      severity.add(StatusDisplayInfo.getStatusText(Severity.WARNING));
      severity.add(StatusDisplayInfo.getStatusText(Severity.MINOR));
      severity.add(StatusDisplayInfo.getStatusText(Severity.MAJOR));
      severity.add(StatusDisplayInfo.getStatusText(Severity.CRITICAL));
      severity.select(eventTemplate.getSeverity().getValue());

      name = new LabeledText(dialogArea, SWT.NONE);
      name.setLabel(i18n.tr("Event name"));
      name.setText(eventTemplate.getName());
      gd = new GridData();
      gd.horizontalSpan = 2;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.TOP;
      name.setLayoutData(gd);

      Group optionGroup = new Group(dialogArea, SWT.NONE);
      optionGroup.setText("Options");
      GridLayout optionLayout = new GridLayout();
      optionGroup.setLayout(optionLayout);

      optionLog = new Button(optionGroup, SWT.CHECK);
      optionLog.setText(i18n.tr("Write to log"));
      optionLog.setSelection((eventTemplate.getFlags() & EventTemplate.FLAG_WRITE_TO_LOG) != 0);
      gd = new GridData();
      gd.horizontalAlignment = SWT.LEFT;
      gd.verticalAlignment = SWT.CENTER;
      optionLog.setLayoutData(gd);

      optionDoNotMonitor = new Button(optionGroup, SWT.CHECK);
      optionDoNotMonitor.setText(i18n.tr("Hide from event &monitor"));
      optionDoNotMonitor.setSelection((eventTemplate.getFlags() & EventTemplate.FLAG_DO_NOT_MONITOR) != 0);
      gd = new GridData();
      gd.horizontalAlignment = SWT.LEFT;
      gd.verticalAlignment = SWT.CENTER;
      optionDoNotMonitor.setLayoutData(gd);

      message = new LabeledText(dialogArea, SWT.NONE);
      message.setLabel(i18n.tr("Message"));
      message.setText(eventTemplate.getMessage());
      gd = new GridData();
      gd.horizontalSpan = 3;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.widthHint = 900;
      message.setLayoutData(gd);

      tags = new LabeledText(dialogArea, SWT.NONE);
      tags.setLabel("Tags");
      tags.setText(eventTemplate.getTagList());
      gd = new GridData();
      gd.horizontalSpan = 3;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.widthHint = 450;
      tags.setLayoutData(gd);
      
      description = new LabeledText(dialogArea, SWT.NONE, SWT.MULTI | SWT.BORDER | SWT.H_SCROLL | SWT.V_SCROLL | SWT.WRAP);
      description.setLabel(i18n.tr("Description"));
      description.setText(eventTemplate.getDescription());
      gd = new GridData();
      gd.horizontalSpan = 3;
      gd.grabExcessVerticalSpace = true;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.heightHint = 300;
      gd.widthHint = 450;
      gd.verticalAlignment = SWT.FILL;
      description.setLayoutData(gd);
      
      description.addDisposeListener(new DisposeListener() {
         
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            Point size = getShell().getSize();
            PreferenceStore settings = PreferenceStore.getInstance();
            settings.set(CONFIG_PREFIX + ".cx", size.x);
            settings.set(CONFIG_PREFIX + ".cy", size.y); 
         }
      });

		return dialogArea;
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
		eventTemplate.setName(name.getText());
		eventTemplate.setSeverity(Severity.getByValue(severity.getSelectionIndex()));
		eventTemplate.setMessage(message.getText());
		eventTemplate.setDescription(description.getText());
      eventTemplate.setFlags((optionLog.getSelection() ? EventTemplate.FLAG_WRITE_TO_LOG : 0) | (optionDoNotMonitor.getSelection() ? EventTemplate.FLAG_DO_NOT_MONITOR : 0));

		HashSet<String> tagSet = new HashSet<String>();
		for(String s : tags.getText().split(","))
		{
		   tagSet.add(s.trim());
		}
		eventTemplate.setTags(tagSet);

		super.okPressed();
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
      newShell.setText(isNew ? i18n.tr("Create Event Template") : i18n.tr("Edit Event Template"));
		super.configureShell(newShell);
      PreferenceStore settings = PreferenceStore.getInstance();
      int cx = settings.getAsInteger(CONFIG_PREFIX + ".cx", 0);
      int cy = settings.getAsInteger(CONFIG_PREFIX + ".cy", 0);
      if ((cx > 0) && (cy > 0))
      {
         newShell.setSize(cx, cy);
      }
      else
      {
         newShell.setSize(920, 631);         
      }
	}
}
