/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.datacollection.dialogs;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
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
import org.netxms.client.datacollection.TransformationTestResult;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Test transformation script
 */
public class TestTransformationDlg extends Dialog
{
	private static final int RUN = 111;	// "Run" button ID
	
	private long nodeId;
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
	public TestTransformationDlg(Shell parentShell, long nodeId, String script)
	{
		super(parentShell);
		this.nodeId = nodeId;
		this.script = script;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createButtonsForButtonBar(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected void createButtonsForButtonBar(Composite parent)
	{
		Button b = createButton(parent, RUN, Messages.get().TestTransformationDlg_Run, true);
		b.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				runScript();
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
			}
		});
		createButton(parent, Window.CANCEL, Messages.get().TestTransformationDlg_Close, false);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		imageWaiting = Activator.getImageDescriptor("icons/waiting.png").createImage(); //$NON-NLS-1$
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
		inputValue.setLabel(Messages.get().TestTransformationDlg_Input);
		GridData gd = new GridData();
		gd.widthHint = 300;
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		inputValue.setLayoutData(gd);
		
		status = new CLabel(dialogArea, SWT.BORDER);
		status.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
		status.setText(Messages.get().TestTransformationDlg_Idle);
		status.setImage(StatusDisplayInfo.getStatusImage(ObjectStatus.UNKNOWN));
		
		result = new LabeledText(dialogArea, SWT.NONE);
		result.setLabel(Messages.get().TestTransformationDlg_Result);
		result.getTextControl().setEditable(false);
		result.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
		
		return dialogArea;
	}

	/**
	 * Run script
	 */
	private void runScript()
	{
		getButton(RUN).setEnabled(false);
		
		final String input = inputValue.getText();
		inputValue.getTextControl().setEditable(false);
		
		status.setText(Messages.get().TestTransformationDlg_Running);
		status.setImage(imageWaiting);
		
		result.setText(""); //$NON-NLS-1$
		
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob(Messages.get().TestTransformationDlg_JobTitle, null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final TransformationTestResult r = session.testTransformationScript(nodeId, script, input);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						if ((getShell() == null) || getShell().isDisposed())
							return;
						
						if (r.success)
						{
							status.setText(Messages.get().TestTransformationDlg_Success);
							status.setImage(StatusDisplayInfo.getStatusImage(Severity.NORMAL));
						}
						else
						{
							status.setText(Messages.get().TestTransformationDlg_Failure);
							status.setImage(StatusDisplayInfo.getStatusImage(Severity.CRITICAL));
						}
						result.setText(r.result);
					}
				});
			}
			
			@Override
			protected String getErrorMessage()
			{
				return Messages.get().TestTransformationDlg_JobError;
			}

			/* (non-Javadoc)
			 * @see org.netxms.ui.eclipse.jobs.ConsoleJob#jobFinalize()
			 */
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
