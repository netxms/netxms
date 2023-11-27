/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.datacollection.dialogs;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.constants.Severity;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.TransformationTestResult;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.dialogs.ObjectSelectionDialog;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.xnap.commons.i18n.I18n;

/**
 * Test transformation script
 */
public class TestTransformationDlg extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(TestTransformationDlg.class);
   private static final int RUN = 111; // "Run" button ID

   private DataCollectionObject object;
   private String script;
   private LabeledText inputValue;
   private CLabel status;
   private LabeledText result;
   private Image imageWaiting;

   /**
    * @param parentShell
    * @param nodeId
    * @param script
    */
   public TestTransformationDlg(Shell parentShell, DataCollectionObject object, String script)
   {
      super(parentShell);
      this.object = object;
      this.script = script;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createButtonsForButtonBar(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createButtonsForButtonBar(Composite parent)
   {
      Button b = createButton(parent, RUN, i18n.tr("&Run"), true);
      b.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            runScript();
         }
      });
      createButton(parent, Window.CANCEL, i18n.tr("Close"), false);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      imageWaiting = ResourceManager.getImageDescriptor("icons/waiting.png").createImage(); //$NON-NLS-1$
      parent.addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            imageWaiting.dispose();
         }
      });

      final Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      dialogArea.setLayout(layout);

      inputValue = new LabeledText(dialogArea, SWT.NONE);
      inputValue.setLabel(i18n.tr("Input value"));
      GridData gd = new GridData();
      gd.widthHint = 300;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      inputValue.setLayoutData(gd);

      status = new CLabel(dialogArea, SWT.BORDER);
      status.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      status.setText(i18n.tr("Idle"));
      status.setImage(StatusDisplayInfo.getStatusImage(ObjectStatus.UNKNOWN));

      result = new LabeledText(dialogArea, SWT.NONE);
      result.setLabel(i18n.tr("Result"));
      result.getTextControl().setEditable(false);
      result.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      return dialogArea;
   }

   /**
    * Run script
    */
   private void runScript()
   {
      result.setText(""); //$NON-NLS-1$

      final NXCSession session = Registry.getSession();

      AbstractObject owner = session.findObjectById(object.getNodeId());
      if ((owner == null) || !(owner instanceof DataCollectionTarget))
      {
         ObjectSelectionDialog dlg = new ObjectSelectionDialog(getShell(), ObjectSelectionDialog.createDataCollectionTargetSelectionFilter());
         dlg.setTitle("Select Execution Target");
         if (dlg.open() != Window.OK)
         {
            status.setText("Cancelled");
            status.setImage(StatusDisplayInfo.getStatusImage(Severity.MINOR));
            return;
         }
         owner = dlg.getSelectedObjects().get(0);
         if ((owner == null) || !(owner instanceof DataCollectionTarget))
         {
            status.setText("Invalid execution target");
            status.setImage(StatusDisplayInfo.getStatusImage(Severity.CRITICAL));
            return;
         }
      }

      getButton(RUN).setEnabled(false);

      final String input = inputValue.getText();
      inputValue.getTextControl().setEditable(false);

      status.setText(i18n.tr("Running..."));
      status.setImage(imageWaiting);

      final long nodeId = owner.getObjectId();
      new Job(i18n.tr("Execute script on server"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final TransformationTestResult r = session.testTransformationScript(nodeId, script, input, object);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if ((getShell() == null) || getShell().isDisposed())
                     return;

                  if (r.success)
                  {
                     status.setText(i18n.tr("Success"));
                     status.setImage(StatusDisplayInfo.getStatusImage(Severity.NORMAL));
                  }
                  else
                  {
                     status.setText(i18n.tr("Failure"));
                     status.setImage(StatusDisplayInfo.getStatusImage(Severity.CRITICAL));
                  }
                  result.setText(r.result);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot execute script");
         }

         @Override
         protected void jobFinalize()
         {
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if ((getShell() == null) || getShell().isDisposed())
                     return;

                  getButton(RUN).setEnabled(true);
                  inputValue.getTextControl().setEditable(true);
                  inputValue.getTextControl().setFocus();
               }
            });
         }
      }.start();
   }
}
