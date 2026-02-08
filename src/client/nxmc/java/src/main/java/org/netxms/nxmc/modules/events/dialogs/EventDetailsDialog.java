/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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

import java.util.Date;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.swt.widgets.TableItem;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.Severity;
import org.netxms.client.events.Event;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.events.widgets.helpers.HistoricalEvent;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for displaying event details
 */
public class EventDetailsDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(EventDetailsDialog.class);

   private Object event;
   private NXCSession session;

   /**
    * Create dialog for displaying event details.
    *
    * @param parentShell parent shell
    * @param event event object (Event or HistoricalEvent)
    */
   public EventDetailsDialog(Shell parentShell, Object event)
   {
      super(parentShell);
      setShellStyle(getShellStyle() | SWT.RESIZE);
      this.event = event;
      this.session = Registry.getSession();
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Event Details"));
      newShell.setMinimumSize(500, 400);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#isResizable()
    */
   @Override
   protected boolean isResizable()
   {
      return true;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createButtonsForButtonBar(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createButtonsForButtonBar(Composite parent)
   {
      createButton(parent, IDialogConstants.OK_ID, i18n.tr("Close"), true);
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
      layout.numColumns = 2;
      layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      dialogArea.setLayout(layout);

      // Extract event data
      long id = getEventId();
      int code = getEventCode();
      Date timeStamp = getEventTimestamp();
      long sourceId = getEventSourceId();
      long dciId = getEventDciId();
      Severity severity = getEventSeverity();
      String message = getEventMessage();
      String userTag = getEventUserTag();
      String[] parameters = getEventParameters();

      String eventName = session.getEventName(code);
      String sourceName = session.getObjectName(sourceId);

      // Event ID
      createLabel(dialogArea, i18n.tr("Event ID:"));
      createReadOnlyText(dialogArea, Long.toString(id));

      // Event Code
      createLabel(dialogArea, i18n.tr("Event code:"));
      createReadOnlyText(dialogArea, Integer.toString(code));

      // Event Name
      createLabel(dialogArea, i18n.tr("Event name:"));
      createReadOnlyText(dialogArea, eventName);

      // Timestamp
      createLabel(dialogArea, i18n.tr("Timestamp:"));
      createReadOnlyText(dialogArea, DateFormatFactory.getDateTimeFormat().format(timeStamp));

      // Severity
      createLabel(dialogArea, i18n.tr("Severity:"));
      Composite severityComposite = new Composite(dialogArea, SWT.NONE);
      GridLayout severityLayout = new GridLayout(2, false);
      severityLayout.marginWidth = 0;
      severityLayout.marginHeight = 0;
      severityComposite.setLayout(severityLayout);
      severityComposite.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      Label severityIcon = new Label(severityComposite, SWT.NONE);
      severityIcon.setImage(StatusDisplayInfo.getStatusImage(severity));

      Label severityText = new Label(severityComposite, SWT.NONE);
      severityText.setText(StatusDisplayInfo.getStatusText(severity));
      severityText.setForeground(StatusDisplayInfo.getStatusColor(severity));

      // Source
      createLabel(dialogArea, i18n.tr("Source:"));
      createReadOnlyText(dialogArea, sourceName + " [" + sourceId + "]");

      // DCI ID (if applicable)
      if (dciId > 0)
      {
         createLabel(dialogArea, i18n.tr("DCI ID:"));
         createReadOnlyText(dialogArea, Long.toString(dciId));
      }

      // User Tag (only for Event, not HistoricalEvent)
      if ((userTag != null) && !userTag.isEmpty())
      {
         createLabel(dialogArea, i18n.tr("User tag:"));
         createReadOnlyText(dialogArea, userTag);
      }

      // Message
      Label messageLabel = createLabel(dialogArea, i18n.tr("Message:"));
      GridData gd = (GridData)messageLabel.getLayoutData();
      gd.verticalAlignment = SWT.TOP;

      Text messageText = new Text(dialogArea, SWT.BORDER | SWT.MULTI | SWT.READ_ONLY | SWT.WRAP | SWT.V_SCROLL);
      messageText.setText(message);
      gd = new GridData(SWT.FILL, SWT.FILL, true, false);
      gd.heightHint = 60;
      gd.widthHint = 400;
      messageText.setLayoutData(gd);

      // Parameters (only for Event, not HistoricalEvent)
      if ((parameters != null) && (parameters.length > 0))
      {
         String[] parameterNames = getEventParameterNames();

         Label parametersLabel = createLabel(dialogArea, i18n.tr("Parameters:"));
         gd = (GridData)parametersLabel.getLayoutData();
         gd.verticalAlignment = SWT.TOP;

         Table parametersTable = new Table(dialogArea, SWT.BORDER | SWT.FULL_SELECTION);
         parametersTable.setHeaderVisible(true);
         parametersTable.setLinesVisible(true);
         gd = new GridData(SWT.FILL, SWT.FILL, true, true);
         gd.heightHint = 100;
         parametersTable.setLayoutData(gd);

         TableColumn colIndex = new TableColumn(parametersTable, SWT.LEFT);
         colIndex.setText(i18n.tr("Index"));
         colIndex.setWidth(60);

         TableColumn colName = new TableColumn(parametersTable, SWT.LEFT);
         colName.setText(i18n.tr("Name"));
         colName.setWidth(150);

         TableColumn colValue = new TableColumn(parametersTable, SWT.LEFT);
         colValue.setText(i18n.tr("Value"));
         colValue.setWidth(250);

         for(int i = 0; i < parameters.length; i++)
         {
            TableItem item = new TableItem(parametersTable, SWT.NONE);
            item.setText(0, Integer.toString(i + 1));
            String name = (parameterNames != null && i < parameterNames.length) ? parameterNames[i] : "";
            item.setText(1, (name != null) ? name : "");
            item.setText(2, parameters[i] != null ? parameters[i] : "");
         }
      }

      return dialogArea;
   }

   /**
    * Create a label with standard layout.
    *
    * @param parent parent composite
    * @param text label text
    * @return created label
    */
   private Label createLabel(Composite parent, String text)
   {
      Label label = new Label(parent, SWT.NONE);
      label.setText(text);
      label.setLayoutData(new GridData(SWT.RIGHT, SWT.CENTER, false, false));
      return label;
   }

   /**
    * Create a read-only text field with standard layout.
    *
    * @param parent parent composite
    * @param text text content
    * @return created text widget
    */
   private Text createReadOnlyText(Composite parent, String text)
   {
      Text textWidget = new Text(parent, SWT.BORDER | SWT.READ_ONLY);
      textWidget.setText(text != null ? text : "");
      textWidget.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      return textWidget;
   }

   /**
    * Get event ID from event object.
    */
   private long getEventId()
   {
      if (event instanceof Event)
         return ((Event)event).getId();
      if (event instanceof HistoricalEvent)
         return ((HistoricalEvent)event).getId();
      return 0;
   }

   /**
    * Get event code from event object.
    */
   private int getEventCode()
   {
      if (event instanceof Event)
         return ((Event)event).getCode();
      if (event instanceof HistoricalEvent)
         return ((HistoricalEvent)event).getCode();
      return 0;
   }

   /**
    * Get event timestamp from event object.
    */
   private Date getEventTimestamp()
   {
      if (event instanceof Event)
         return ((Event)event).getTimeStamp();
      if (event instanceof HistoricalEvent)
         return ((HistoricalEvent)event).getTimeStamp();
      return new Date();
   }

   /**
    * Get event source ID from event object.
    */
   private long getEventSourceId()
   {
      if (event instanceof Event)
         return ((Event)event).getSourceId();
      if (event instanceof HistoricalEvent)
         return ((HistoricalEvent)event).getSourceId();
      return 0;
   }

   /**
    * Get event DCI ID from event object.
    */
   private long getEventDciId()
   {
      if (event instanceof Event)
         return ((Event)event).getDciId();
      if (event instanceof HistoricalEvent)
         return ((HistoricalEvent)event).getDciId();
      return 0;
   }

   /**
    * Get event severity from event object.
    */
   private Severity getEventSeverity()
   {
      if (event instanceof Event)
         return ((Event)event).getSeverity();
      if (event instanceof HistoricalEvent)
         return ((HistoricalEvent)event).getSeverity();
      return Severity.NORMAL;
   }

   /**
    * Get event message from event object.
    */
   private String getEventMessage()
   {
      if (event instanceof Event)
         return ((Event)event).getMessage();
      if (event instanceof HistoricalEvent)
         return ((HistoricalEvent)event).getMessage();
      return "";
   }

   /**
    * Get event user tag from event object (only available for Event, not HistoricalEvent).
    */
   private String getEventUserTag()
   {
      if (event instanceof Event)
         return ((Event)event).getUserTag();
      return null;
   }

   /**
    * Get event parameters from event object (only available for Event, not HistoricalEvent).
    */
   private String[] getEventParameters()
   {
      if (event instanceof Event)
         return ((Event)event).getParameters();
      return null;
   }

   /**
    * Get event parameter names from event object (only available for Event, not HistoricalEvent).
    */
   private String[] getEventParameterNames()
   {
      if (event instanceof Event)
         return ((Event)event).getParameterNames();
      return null;
   }
}
