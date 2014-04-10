package org.netxms.ui.eclipse.reporter.propertypages;

import java.util.ArrayList;
import java.util.List;

import org.eclipse.jface.dialogs.IInputValidator;
import org.eclipse.jface.dialogs.InputDialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.ListViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.api.client.reporting.ReportRenderFormat;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "Notification" property page for schedule
 */
public class Notification extends PropertyPage
{
	final public static String ID = "org.netxms.ui.eclipse.reporter.propertypages.Notification"; //$NON-NLS-1$
	final public static int FILE_PDF = 1;
	final public static int FILE_XLS = 2;
	
	final  NXCSession session = (NXCSession)ConsoleSharedData.getSession();
	
	private static List<String> mails = new ArrayList<String>(0);
	private static ReportRenderFormat attachFormat = null;
	private static boolean sendNotification = false;
	
	private Composite emailComp;
	private ListViewer listViewer;
	private Button sendReport;
	private Button sendNotify;
	private Button pdfFormat;
	private Button xlsFormat;

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
		GridLayout layout = new GridLayout();
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		parent.setLayout(layout);
		GridData gd1 = new GridData(SWT.FILL, SWT.FILL, true, true);
		parent.setLayoutData(gd1);
		
		Composite dialogArea = new Composite(parent, SWT.NONE);
		layout = new GridLayout(2, false);
		dialogArea.setLayout(layout);
		dialogArea.setLayoutData(gd1);
		
		sendNotify = new Button(dialogArea, SWT.CHECK);
		sendNotify.setLayoutData(new GridData(SWT.LEFT, SWT.TOP, true, false, 2, 1));
		sendNotify.setText("Enable notification");
		sendNotify.addMouseListener(new MouseListener()
		{
			
			@Override
			public void mouseUp(MouseEvent e)
			{
			}
			
			@Override
			public void mouseDown(MouseEvent e)
			{
				if (e.button == 1)
				{
					sendNotification = !sendNotify.getSelection();
					recursiveSetEnabled(emailComp, sendNotification);
					xlsFormat.setEnabled(sendReport.isEnabled() && sendReport.getSelection());
					pdfFormat.setEnabled(sendReport.isEnabled() && sendReport.getSelection());
					emailComp.layout(true, true);
				}
			}
			
			@Override
			public void mouseDoubleClick(MouseEvent e)
			{
			}
		});
		
		emailComp = new Composite(dialogArea, SWT.NONE);
		emailComp.setLayout(layout);
		emailComp.setLayoutData(gd1);
		
		Label lblMail = new Label(emailComp, SWT.NONE);
		lblMail.setLayoutData(new GridData(SWT.LEFT, SWT.TOP, true, false, 2, 1));
		lblMail.setText("Recipients mails:");
		
		listViewer = new ListViewer(emailComp, SWT.BORDER | SWT.MULTI | SWT.V_SCROLL);
		listViewer.getList().setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true, 1, 2));
		listViewer.setContentProvider(new ArrayContentProvider());
		listViewer.setComparator(new ViewerComparator() {
			@Override
			public int compare(Viewer viewer, Object e1, Object e2)
			{
				return ((String)e1).compareToIgnoreCase(((String)e2));
			}
		});
		

		Button buttonAdd = new Button(emailComp, SWT.PUSH);
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

		final Button buttonRemove = new Button(emailComp, SWT.PUSH);
		gd = new GridData(SWT.FILL, SWT.TOP, false, false);
		gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
		buttonRemove.setLayoutData(gd);
		buttonRemove.setText("Remove");
		buttonRemove.setEnabled(false);
		buttonRemove.addSelectionListener(new SelectionAdapter(){
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				removeMail();
			}
		});
		
		Group attachmentGroup = new Group(emailComp, SWT.NONE);
		attachmentGroup.setText("Attachment");
		attachmentGroup.setLayout(layout);
		attachmentGroup.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));
		
		sendReport = new Button(attachmentGroup, SWT.CHECK);
		sendReport.setLayoutData(new GridData(SWT.LEFT, SWT.FILL, true, false, 2, 1));
		sendReport.setText("Render a report for attachment to email");
		sendReport.addMouseListener(new MouseListener() {
			
			@Override
			public void mouseUp(MouseEvent e)
			{
			}
			
			@Override
			public void mouseDown(MouseEvent e)
			{
				if (e.button == 1)
				{
					xlsFormat.setEnabled(sendReport.isEnabled() && !sendReport.getSelection());
					pdfFormat.setEnabled(sendReport.isEnabled() && !sendReport.getSelection());
				}
			}
			
			@Override
			public void mouseDoubleClick(MouseEvent e)
			{
			}
		});

		pdfFormat = new Button(attachmentGroup, SWT.RADIO);
		pdfFormat.setEnabled(sendReport.getSelection());
		pdfFormat.setText("PDF"); //$NON-NLS-1$
		pdfFormat.setSelection(true);
		
		xlsFormat = new Button(attachmentGroup, SWT.RADIO);
		xlsFormat.setEnabled(sendReport.getSelection());
		xlsFormat.setText("XLS"); //$NON-NLS-1$

		recursiveSetEnabled(emailComp, false);
		parent.layout(true, true);
		
		return parent;
	}
	
	/**
	 * Add mail to list
	 */
	private void addMail()
	{
		InputDialog dlg = new InputDialog(emailComp.getShell(), "Add mail", "Enter mail", "", new IInputValidator() //$NON-NLS-1$
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
			mails.add(dlg.getValue());
			listViewer.setInput(mails);
		}
	}
	
	/**
	 * Remove selected mails
	 */
	private void removeMail()
	{
		IStructuredSelection selection = (IStructuredSelection)listViewer.getSelection();
		for(Object o : selection.toList())
			mails.remove((String)o);
		listViewer.setInput(mails);
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

	/**
	 * Apply changes
	 * 
	 * @param isApply
	 *            true if update operation caused by "Apply" button
	 */
	protected boolean applyChanges(final boolean isApply)
	{
		return true;
	}
	
	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		Notification.attachFormat = (sendReport.getSelection() ? ReportRenderFormat.valueOf(pdfFormat.getSelection() ? FILE_PDF : FILE_XLS) : null);
		return applyChanges(false);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.preference.PreferencePage#performApply()
	 */
	@Override
	protected void performApply()
	{
		applyChanges(true);
	}

	/**
	 * @return send notify
	 */
	public static boolean sendNotify()
	{
		return sendNotification;
	}
	
	/**
	 * @return the mails
	 */
	public static List<String> getMails()
	{
		if (!sendNotify())
			return new ArrayList<String>(0);
		return mails;
	}

	/**
	 * @return the get attachment file format
	 */
	public static ReportRenderFormat getAttachFormatCode()
	{
		return attachFormat;
	}
	
	/**
	 * Clear static variable
	 */
	public static void clear()
	{
		mails = new ArrayList<String>(0);
		sendNotification = false;
		attachFormat = null;
	}
}
