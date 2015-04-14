package org.netxms.ui.eclipse.reporter.propertypages;

import org.eclipse.jface.dialogs.IInputValidator;
import org.eclipse.jface.dialogs.InputDialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.ListViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.reporting.ReportRenderFormat;
import org.netxms.client.reporting.ReportingJob;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "Notification" property page for schedule
 */
public class Notifications extends PropertyPage
{
	public static final String ID = "org.netxms.ui.eclipse.reporter.propertypages.Notification"; //$NON-NLS-1$
	
	private Composite emailGroup;
	private ListViewer listViewer;
	private Button sendReport;
	private Button sendNotify;
	private Button pdfFormat;
	private Button xlsFormat;

   private ReportingJob job = null;

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse
	 * .swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{		
      job = (ReportingJob)getElement().getAdapter(ReportingJob.class);

      Composite dialogArea = new Composite(parent, SWT.NONE);
      GridLayout dialogLayout = new GridLayout();
      dialogLayout.marginWidth = 0;
      dialogLayout.marginHeight = 0;
      dialogArea.setLayout(dialogLayout);
		
		sendNotify = new Button(dialogArea, SWT.CHECK);
		sendNotify.setLayoutData(new GridData(SWT.LEFT, SWT.TOP, true, false, 2, 1));
		sendNotify.setText("Send notification on job completion");
		sendNotify.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            job.setNotifyOnCompletion(sendNotify.getSelection());
            recursiveSetEnabled(emailGroup, sendNotify.getSelection());
            xlsFormat.setEnabled(sendReport.isEnabled() && sendReport.getSelection());
            pdfFormat.setEnabled(sendReport.isEnabled() && sendReport.getSelection());
            emailGroup.layout(true, true);
         }
      });
		
		emailGroup = new Composite(dialogArea, SWT.NONE);
		GridLayout layout = new GridLayout();
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 2;
		emailGroup.setLayout(layout);
		emailGroup.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
		
		Label lblMail = new Label(emailGroup, SWT.NONE);
		lblMail.setLayoutData(new GridData(SWT.LEFT, SWT.TOP, true, false, 2, 1));
		lblMail.setText("Recipients");
		
		listViewer = new ListViewer(emailGroup, SWT.BORDER | SWT.MULTI | SWT.V_SCROLL);
		listViewer.getList().setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true, 1, 2));
		listViewer.setContentProvider(new ArrayContentProvider());
		listViewer.setComparator(new ViewerComparator() {
			@Override
			public int compare(Viewer viewer, Object e1, Object e2)
			{
				return ((String)e1).compareToIgnoreCase(((String)e2));
			}
		});

		Button buttonAdd = new Button(emailGroup, SWT.PUSH);
		GridData gd = new GridData(SWT.FILL, SWT.TOP, false, false);
		gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
		buttonAdd.setLayoutData(gd);
		buttonAdd.setText("Add...");
		buttonAdd.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				addMail();
			}
		});

		final Button buttonRemove = new Button(emailGroup, SWT.PUSH);
		gd = new GridData(SWT.FILL, SWT.TOP, false, false);
		gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
		buttonRemove.setLayoutData(gd);
		buttonRemove.setText("Remove");
		buttonRemove.setEnabled(false);
		buttonRemove.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				removeMail();
			}
		});
		
		final Composite attachmentGroup = new Composite(emailGroup, SWT.NONE);
		layout = new GridLayout();
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 2;
		layout.horizontalSpacing = 10;
		attachmentGroup.setLayout(layout);
		attachmentGroup.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));
		
		sendReport = new Button(attachmentGroup, SWT.CHECK);
		sendReport.setLayoutData(new GridData(SWT.LEFT, SWT.FILL, true, false, 2, 1));
		sendReport.setText("Attach rendered report to notification email as");
		sendReport.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            xlsFormat.setEnabled(sendReport.getSelection());
            pdfFormat.setEnabled(sendReport.getSelection());
         }
      });

		pdfFormat = new Button(attachmentGroup, SWT.RADIO);
		pdfFormat.setEnabled(sendReport.getSelection());
		pdfFormat.setText("PDF"); //$NON-NLS-1$
		pdfFormat.setSelection(true);
		
		xlsFormat = new Button(attachmentGroup, SWT.RADIO);
		xlsFormat.setEnabled(sendReport.getSelection());
		xlsFormat.setText("XLS"); //$NON-NLS-1$

		recursiveSetEnabled(emailGroup, false);
		return dialogArea;
	}
	
	/**
	 * Add mail to list
	 */
	private void addMail()
	{
		InputDialog dlg = new InputDialog(emailGroup.getShell(), "Add mail", "Enter mail", "", new IInputValidator() //$NON-NLS-1$
		{
			@Override
			public String isValid(String newText)
			{
				String emailreg = "^[_A-Za-z0-9-]+(\\.[_A-Za-z0-9-]+)*@[A-Za-z0-9]+(\\.[A-Za-z0-9]+)*(\\.[A-Za-z]{2,})$"; //$NON-NLS-1$
                if (!newText.matches(emailreg))
                	return "Invalid mail";
				return null;
			}
		});
		if (dlg.open() == Window.OK)
		{
			job.getEmailRecipients().add(dlg.getValue());
			listViewer.setInput(job.getEmailRecipients().toArray());
		}
	}
	
	/**
	 * Remove selected mails
	 */
	private void removeMail()
	{
		IStructuredSelection selection = (IStructuredSelection)listViewer.getSelection();
		for(Object o : selection.toList())
		   job.getEmailRecipients().remove((String)o);
		listViewer.setInput(job.getEmailRecipients().toArray());
	}
	
	/**
	 * Disable/enable the composite and the controls inside
	 * 
	 * @param control
	 * @param enabled
	 */
	public void recursiveSetEnabled(Control control, boolean enabled) 
	{
		if (control instanceof Composite) 
		{
			Composite comp = (Composite) control;
			for (Control c : comp.getChildren())
				recursiveSetEnabled(c, enabled);
		}
		control.setEnabled(enabled);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
	   if (sendReport.getSelection())
	      job.setRenderFormat(pdfFormat.getSelection() ? ReportRenderFormat.PDF : ReportRenderFormat.XLS);
	   else
	      job.setRenderFormat(ReportRenderFormat.NONE);
		return true;
	}
}
