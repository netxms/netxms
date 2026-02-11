/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.modules.objects.dialogs;

import org.eclipse.core.runtime.IProgressMonitor;
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
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

/**
 * Dialog to show explanation of how an object's compound status was calculated
 */
public class StatusExplanationDialog extends Dialog
{
   private static final int REFRESH_ID = IDialogConstants.CLIENT_ID + 1;

   private final I18n i18n = LocalizationHelper.getI18n(StatusExplanationDialog.class);

   private AbstractObject object;
   private NXCSession session;
   private Label statusLabel;
   private Label algorithmLabel;
   private Table childrenTable;
   private Label alarmLabel;
   private Label additionalLabel;
   private Label decidingFactorLabel;

   /**
    * Constructor
    *
    * @param parentShell parent shell
    * @param object the object to explain status for
    */
   public StatusExplanationDialog(Shell parentShell, AbstractObject object)
   {
      super(parentShell);
      this.object = object;
      this.session = Registry.getSession();
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Status Explanation"));
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
      createButton(parent, REFRESH_ID, i18n.tr("&Refresh"), false);
      createButton(parent, IDialogConstants.OK_ID, i18n.tr("Close"), true);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#buttonPressed(int)
    */
   @Override
   protected void buttonPressed(int buttonId)
   {
      if (buttonId == REFRESH_ID)
      {
         refreshExplanation();
         return;
      }
      super.buttonPressed(buttonId);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.numColumns = 2;
      dialogArea.setLayout(layout);

      // Object name and status header
      Label nameHeader = new Label(dialogArea, SWT.NONE);
      nameHeader.setText(i18n.tr("Object:"));
      nameHeader.setFont(parent.getFont());

      Label nameValue = new Label(dialogArea, SWT.NONE);
      nameValue.setText(object.getObjectName());
      GridData gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      nameValue.setLayoutData(gd);

      Label statusHeader = new Label(dialogArea, SWT.NONE);
      statusHeader.setText(i18n.tr("Current status:"));

      statusLabel = new Label(dialogArea, SWT.NONE);
      statusLabel.setText(StatusDisplayInfo.getStatusText(object.getStatus()));
      statusLabel.setForeground(StatusDisplayInfo.getStatusColor(object.getStatus()));
      gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      statusLabel.setLayoutData(gd);

      Label algHeader = new Label(dialogArea, SWT.NONE);
      algHeader.setText(i18n.tr("Algorithm:"));

      algorithmLabel = new Label(dialogArea, SWT.NONE);
      algorithmLabel.setText("");
      gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      algorithmLabel.setLayoutData(gd);

      // Separator
      Label separator1 = new Label(dialogArea, SWT.SEPARATOR | SWT.HORIZONTAL);
      gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.horizontalSpan = 2;
      separator1.setLayoutData(gd);

      // Children section
      Label childrenHeader = new Label(dialogArea, SWT.NONE);
      childrenHeader.setText(i18n.tr("Children:"));
      gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.horizontalSpan = 2;
      childrenHeader.setLayoutData(gd);

      childrenTable = new Table(dialogArea, SWT.BORDER | SWT.FULL_SELECTION);
      childrenTable.setHeaderVisible(true);
      childrenTable.setLinesVisible(true);
      gd = new GridData(SWT.FILL, SWT.FILL, true, true);
      gd.horizontalSpan = 2;
      gd.heightHint = 200;
      gd.widthHint = 600;
      childrenTable.setLayoutData(gd);

      TableColumn colName = new TableColumn(childrenTable, SWT.LEFT);
      colName.setText(i18n.tr("Name"));
      colName.setWidth(200);

      TableColumn colStatus = new TableColumn(childrenTable, SWT.LEFT);
      colStatus.setText(i18n.tr("Status"));
      colStatus.setWidth(80);

      TableColumn colPropagated = new TableColumn(childrenTable, SWT.LEFT);
      colPropagated.setText(i18n.tr("Propagated"));
      colPropagated.setWidth(80);

      TableColumn colPropMethod = new TableColumn(childrenTable, SWT.LEFT);
      colPropMethod.setText(i18n.tr("Propagation"));
      colPropMethod.setWidth(120);

      TableColumn colContributor = new TableColumn(childrenTable, SWT.CENTER);
      colContributor.setText(i18n.tr("Contributor"));
      colContributor.setWidth(80);

      // Separator
      Label separator2 = new Label(dialogArea, SWT.SEPARATOR | SWT.HORIZONTAL);
      gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.horizontalSpan = 2;
      separator2.setLayoutData(gd);

      // Alarm severity
      Label alarmHeader = new Label(dialogArea, SWT.NONE);
      alarmHeader.setText(i18n.tr("Alarm severity:"));

      alarmLabel = new Label(dialogArea, SWT.NONE);
      alarmLabel.setText("");
      gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      alarmLabel.setLayoutData(gd);

      // Additional status
      Label additionalHeader = new Label(dialogArea, SWT.NONE);
      additionalHeader.setText(i18n.tr("Additional status:"));

      additionalLabel = new Label(dialogArea, SWT.NONE);
      additionalLabel.setText("");
      gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      additionalLabel.setLayoutData(gd);

      // Separator
      Label separator3 = new Label(dialogArea, SWT.SEPARATOR | SWT.HORIZONTAL);
      gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.horizontalSpan = 2;
      separator3.setLayoutData(gd);

      // Deciding factor
      Label factorHeader = new Label(dialogArea, SWT.NONE);
      factorHeader.setText(i18n.tr("Deciding factor:"));

      decidingFactorLabel = new Label(dialogArea, SWT.NONE);
      decidingFactorLabel.setText("");
      gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      decidingFactorLabel.setLayoutData(gd);

      refreshExplanation();

      return dialogArea;
   }

   /**
    * Refresh status explanation from server
    */
   private void refreshExplanation()
   {
      Job job = new Job(i18n.tr("Loading status explanation"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final String json = session.getStatusExplanation(object.getObjectId());
            runInUIThread(() -> {
               if (childrenTable.isDisposed())
                  return;
               updateDisplay(json);
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load status explanation");
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * Update display with parsed JSON data
    *
    * @param json JSON string from server
    */
   private void updateDisplay(String json)
   {
      JsonObject root = JsonParser.parseString(json).getAsJsonObject();

      // Check if unmanaged
      if (root.has("unmanaged") && root.get("unmanaged").getAsBoolean())
      {
         algorithmLabel.setText(i18n.tr("(unmanaged)"));
         childrenTable.removeAll();
         alarmLabel.setText(i18n.tr("(unmanaged)"));
         additionalLabel.setText(i18n.tr("(unmanaged)"));
         decidingFactorLabel.setText(i18n.tr("Object is unmanaged"));
         return;
      }

      // Algorithm
      String algorithm = root.has("algorithm") ? root.get("algorithm").getAsString() : "Unknown";
      algorithmLabel.setText(algorithm);

      // Children table
      childrenTable.removeAll();
      if (root.has("children"))
      {
         JsonArray children = root.getAsJsonArray("children");
         for(JsonElement elem : children)
         {
            JsonObject child = elem.getAsJsonObject();
            TableItem item = new TableItem(childrenTable, SWT.NONE);
            item.setText(0, child.get("name").getAsString());
            item.setText(1, child.get("statusText").getAsString());
            item.setText(2, child.get("propagatedStatusText").getAsString());
            item.setText(3, child.get("propagation").getAsString());
            boolean contributor = child.has("contributor") && child.get("contributor").getAsBoolean();
            item.setText(4, contributor ? "\u2713" : "");

            int propagatedStatus = child.get("propagatedStatus").getAsInt();
            if (propagatedStatus >= 0 && propagatedStatus <= 4)
            {
               item.setForeground(1, StatusDisplayInfo.getStatusColor(child.get("status").getAsInt()));
               item.setForeground(2, StatusDisplayInfo.getStatusColor(propagatedStatus));
            }
         }
      }

      // Status from children
      String childrenStatus = root.has("statusFromChildrenText") ? root.get("statusFromChildrenText").getAsString() : "Unknown";

      // Alarm severity
      int alarmSeverity = root.has("alarmSeverity") ? root.get("alarmSeverity").getAsInt() : 5;
      String alarmText = root.has("alarmSeverityText") ? root.get("alarmSeverityText").getAsString() : "Unknown";
      if (alarmSeverity == 5)
      {
         alarmLabel.setText(i18n.tr("(none)"));
         alarmLabel.setForeground(null);
      }
      else
      {
         alarmLabel.setText(alarmText);
         alarmLabel.setForeground(StatusDisplayInfo.getStatusColor(alarmSeverity));
      }

      // Additional status
      int additionalStatus = root.has("additionalStatus") ? root.get("additionalStatus").getAsInt() : 5;
      String additionalText = root.has("additionalStatusText") ? root.get("additionalStatusText").getAsString() : "Unknown";
      String additionalExplanation = root.has("additionalExplanation") ? root.get("additionalExplanation").getAsString() : null;
      if (additionalStatus == 5)
      {
         additionalLabel.setText(i18n.tr("(none)"));
         additionalLabel.setForeground(null);
      }
      else
      {
         String text = additionalText;
         if (additionalExplanation != null)
            text += " (" + additionalExplanation + ")";
         additionalLabel.setText(text);
         additionalLabel.setForeground(StatusDisplayInfo.getStatusColor(additionalStatus));
      }

      // Deciding factor
      String factor = root.has("decidingFactor") ? root.get("decidingFactor").getAsString() : "unknown";
      String factorText;
      switch(factor)
      {
         case "children":
            factorText = i18n.tr("Children status ({0})", childrenStatus);
            break;
         case "alarms":
            factorText = i18n.tr("Alarm severity ({0})", alarmText);
            break;
         case "additional":
            factorText = i18n.tr("Additional status ({0})", additionalText);
            break;
         default:
            factorText = factor;
            break;
      }
      decidingFactorLabel.setText(factorText);

      // Relayout to accommodate changed label sizes
      decidingFactorLabel.getParent().layout(true, true);
   }
}
