/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Raden Solutions
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
package org.netxms.ui.eclipse.reporter.widgets;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IConfigurationElement;
import org.eclipse.core.runtime.IExtensionRegistry;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.program.Program;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.eclipse.ui.forms.widgets.ImageHyperlink;
import org.eclipse.ui.forms.widgets.ScrolledForm;
import org.eclipse.ui.forms.widgets.Section;
import org.eclipse.ui.internal.dialogs.PropertyDialog;
import org.netxms.api.client.SessionListener;
import org.netxms.api.client.SessionNotification;
import org.netxms.api.client.reporting.ReportDefinition;
import org.netxms.api.client.reporting.ReportParameter;
import org.netxms.api.client.reporting.ReportRenderFormat;
import org.netxms.api.client.reporting.ReportResult;
import org.netxms.api.client.reporting.ReportingJob;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.reporter.Activator;
import org.netxms.ui.eclipse.reporter.api.CustomControlFactory;
import org.netxms.ui.eclipse.reporter.propertypages.General;
import org.netxms.ui.eclipse.reporter.widgets.helpers.ReportResultLabelProvider;
import org.netxms.ui.eclipse.reporter.widgets.helpers.ScheduleLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.ImageCache;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Report execution widget. Provides form with report-specific parameters.
 */
@SuppressWarnings("restriction")
public class ReportExecutionForm extends Composite
{
	public static final int SCHEDULE_TYPE = 0;
	public static final int SCHEDULE_START_TIME = 1;
	public static final int SCHEDULE_OWNER = 2;
	public static final int SCHEDULE_COMMENTS = 3;
	
	public static final int RESULT_EXEC_TIME = 0;
   public static final int RESULT_STARTED_BY = 1;
   public static final int RESULT_STATUS = 2;
	
	private NXCSession session = (NXCSession)ConsoleSharedData.getSession();
	
	private IWorkbenchPart workbenchPart = null;
	private FormToolkit toolkit;
	private ScrolledForm form;
	private ImageCache imageCache;
	private SortableTableViewer resultList;
	private SortableTableViewer scheduleList;
	
	private ReportDefinition report;
	private List<ReportParameter> parameters;
	private List<FieldEditor> fields = new ArrayList<FieldEditor>();
	
	/**
	 * @param parent
	 * @param style
	 */
	public ReportExecutionForm(Composite parent, int style, ReportDefinition report)
	{		
		super(parent, style);
		this.report = report;

		imageCache = new ImageCache(this);
		setLayout(new FillLayout());

		/* FORM */
		toolkit = new FormToolkit(getDisplay());
		form = toolkit.createScrolledForm(this);
		form.setText(report.getName());

		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		form.getBody().setLayout(layout);

		// Parameters section
		Section section = toolkit.createSection(form.getBody(), Section.DESCRIPTION | Section.TITLE_BAR);
		section.setText("Parameters");
		section.setDescription("Provide parameters necessary to run this report in fields below");
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		section.setLayoutData(gd);

		final Composite paramArea = toolkit.createComposite(section);
		section.setClient(paramArea);
		createParamEntryFields(paramArea);

		// Schedules section
		section = toolkit.createSection(form.getBody(), Section.DESCRIPTION | Section.TITLE_BAR);
		section.setText("Schedules");
		section.setDescription("Scheduling of report generation");
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
		section.setLayoutData(gd);
		
		final Composite scheduleArea = toolkit.createComposite(section);
		section.setClient(scheduleArea);
		createSchedulesSection(scheduleArea);
		
		// Results section
		section = toolkit.createSection(form.getBody(), Section.DESCRIPTION | Section.TITLE_BAR);
		section.setText("Results");
		section.setDescription("The following execution results are available for rendering");
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
		section.setLayoutData(gd);
		
		final Composite resultArea = toolkit.createComposite(section);
		section.setClient(resultArea);
		createResultsSection(resultArea);

		session.addListener(new SessionListener() {			
			@Override
			public void notificationHandler(SessionNotification n) 
			{
				if (n.getCode() == SessionNotification.RS_SCHEDULES_MODIFIED)
				{
					refreshScheduleList();
				} 
				else if (n.getCode() == SessionNotification.RS_RESULTS_MODIFIED) 
				{
					refreshResultList();
				}
			}
		});
		
		refreshScheduleList();
		refreshResultList();
	}
	
	/**
	 * Create "Schedules" section's content
	 * 
	 * @param parent
	 *            parent composite
	 */
	/**
	 * @param section
	 * @return
	 */
	private void createSchedulesSection(Composite parent)
	{
		GridLayout layout = new GridLayout(2, false);
		parent.setLayout(layout);

		final String[] names = { "Type", "Schedule", "Owner", "Comments" };
		final int[] widths = { 100, 140, 100, 300 };
		scheduleList = new SortableTableViewer(parent, names, widths, 0, SWT.DOWN, SWT.BORDER | SWT.FULL_SELECTION | SWT.MULTI);
		GridData gd = new GridData();
		gd.horizontalSpan = 1;
		gd.verticalSpan = 2;
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		scheduleList.getControl().setLayoutData(gd);
		scheduleList.setContentProvider(new ArrayContentProvider());
		scheduleList.setLabelProvider(new ScheduleLabelProvider());
		
		WidgetHelper.restoreTableViewerSettings(scheduleList, Activator.getDefault().getDialogSettings(), "ReportExecutionForm.ScheduleList");
		scheduleList.getControl().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(scheduleList, Activator.getDefault().getDialogSettings(), "ReportExecutionForm.ScheduleList");
         }
      });

		ImageHyperlink link = toolkit.createImageHyperlink(parent, SWT.WRAP);
		link.setImage(SharedIcons.IMG_DELETE_OBJECT);
		link.setText("Delete");
		link.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				deleteSchedules();
			}
		});
		link.setLayoutData(new GridData(SWT.LEFT, SWT.TOP, true, true, 1, 2));
		link = toolkit.createImageHyperlink(parent, SWT.WRAP);
		link.setImage(imageCache.add(Activator.getImageDescriptor("icons/schedule.png"))); //$NON-NLS-1$
		link.setText("Add Schedule");
		link.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				addSchedule();
			}
		});
	}
	
	/**
	 * Create "Results" section's content
	 * 
	 * @param parent
	 *            parent composite
	 */
	private void createResultsSection(Composite parent)
	{
		GridLayout layout = new GridLayout(2, false);
		parent.setLayout(layout);

		final String[] names = { "Execution Time", "Started by", "Status" };
		final int[] widths = { 180, 140, 100 };
		resultList = new SortableTableViewer(parent, names, widths, 0, SWT.DOWN, SWT.BORDER | SWT.FULL_SELECTION | SWT.MULTI);
		GridData gd = new GridData();
		gd.horizontalSpan = 1;
		gd.verticalSpan = 4;
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		resultList.getControl().setLayoutData(gd);
		resultList.setContentProvider(new ArrayContentProvider());
		resultList.setLabelProvider(new ReportResultLabelProvider());

      WidgetHelper.restoreTableViewerSettings(resultList, Activator.getDefault().getDialogSettings(), "ReportExecutionForm.ResultList");
      resultList.getControl().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(resultList, Activator.getDefault().getDialogSettings(), "ReportExecutionForm.ResultList");
         }
      });
		
		ImageHyperlink link = toolkit.createImageHyperlink(parent, SWT.WRAP);
		link.setImage(imageCache.add(Activator.getImageDescriptor("icons/pdf.png"))); //$NON-NLS-1$
		link.setText("Render to PDF");
		link.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				final ReportResult firstElement = (ReportResult)((IStructuredSelection)resultList.getSelection()).getFirstElement();
				if (firstElement != null)
				{
					renderReport(firstElement.getJobId(), firstElement.getExecutionTime(), ReportRenderFormat.PDF);
				}
			}
		});

		link = toolkit.createImageHyperlink(parent, SWT.WRAP);
		link.setImage(imageCache.add(Activator.getImageDescriptor("icons/xls.png"))); //$NON-NLS-1$
		link.setText("Render to Excel");
		link.addHyperlinkListener(new HyperlinkAdapter()
		{
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				final ReportResult firstElement = (ReportResult) ((IStructuredSelection) resultList.getSelection()).getFirstElement();
				if (firstElement != null)
				{
					renderReport(firstElement.getJobId(), firstElement.getExecutionTime(), ReportRenderFormat.XLS);
				}
			}
		});

		link = toolkit.createImageHyperlink(parent, SWT.WRAP);
		link.setImage(SharedIcons.IMG_DELETE_OBJECT);
		link.setText("Delete");
		link.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				deleteResults();
			}
		});
		link.setLayoutData(new GridData(SWT.LEFT, SWT.TOP, true, true, 1, 2));
		link = toolkit.createImageHyperlink(parent, SWT.WRAP);
		link.setImage(SharedIcons.IMG_EXECUTE);
		link.setText("Execute Report");
		link.addHyperlinkListener(new HyperlinkAdapter()
		{
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				executeReport();
			}
		});
	}

	/**
	 * Create entry fields for parameters
	 * 
	 * @param parent
	 */
	private void createParamEntryFields(Composite parent)
	{
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      layout.verticalSpacing = 10;
      parent.setLayout(layout);
      
		final IExtensionRegistry reg = Platform.getExtensionRegistry();
		IConfigurationElement[] elements = reg.getConfigurationElementsFor("org.netxms.ui.eclipse.reporter.customfields"); //$NON-NLS-1$
		sortFieldProviders(elements);

		parameters = report.getParameters();

		Map<String, FieldEditor> editors = new HashMap<String, FieldEditor>();

		for(ReportParameter parameter : parameters)
		{
			FieldEditor editor = null;
			for(int i = 0; i < elements.length; i++)
			{
				try
				{
					final CustomControlFactory provider = (CustomControlFactory)elements[i].createExecutableExtension("class"); //$NON-NLS-1$
					editor = provider.editorForType(parent, parameter, toolkit);
					if (editor != null)
					{
						break;
					}
				}
				catch(CoreException e)
				{
				   Activator.logError("Cannot create CustomControlFactory instance", e);
				}
			}
			if (editor == null)
			{
				editor = new StringFieldEditor(parameter, toolkit, parent);
			}

			GridData gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.verticalAlignment = SWT.CENTER;
			gd.grabExcessHorizontalSpace = true;
			gd.grabExcessVerticalSpace = false;
			gd.horizontalSpan = parameter.getSpan();
			System.out.println("span=" + gd.horizontalSpan);
			editor.setLayoutData(gd);
			
			if (layout.numColumns < parameter.getSpan())
			   layout.numColumns = parameter.getSpan();

         editors.put(parameter.getName(), editor);
         fields.add(editor);
		}

		for(ReportParameter parameter : parameters)
		{
			final FieldEditor editor = editors.get(parameter.getName());
			final FieldEditor parentEditor = editors.get(parameter.getDependsOn());
			if (editor != null && parentEditor != null)
			{
				parentEditor.setDependantEditor(editor);
			}
		}
	}

	/**
	 * @param elements
	 */
	private void sortFieldProviders(IConfigurationElement[] elements)
	{
		Arrays.sort(elements, new Comparator<IConfigurationElement>() {

			@Override
			public int compare(IConfigurationElement o1, IConfigurationElement o2)
			{
				String attribute1 = o1.getAttribute("priority");
				String attribute2 = o2.getAttribute("priority");
				int priority1 = 0;
				int priority2 = 0;
				if (attribute1 != null)
				{
					try
					{
						priority1 = Integer.parseInt(attribute1);
					}
					catch(Exception e)
					{
						// ignore
					}
				}
				if (attribute2 != null)
				{
					try
					{
						priority2 = Integer.parseInt(attribute2);
					}
					catch(Exception e)
					{
						// ignore
					}
				}

				return priority2 - priority1;
			}
		});
	}

	/**
	 * @param jobId
	 * @param executionTime
	 * @param format
	 */
	protected void renderReport(final UUID jobId, Date executionTime, final ReportRenderFormat format)
	{
		StringBuilder nameTemplate = new StringBuilder();
		nameTemplate.append(report.getName());
		nameTemplate.append(" ");
		nameTemplate.append(new SimpleDateFormat("ddMMyyyy HHmm").format(executionTime));
		nameTemplate.append(".");
		nameTemplate.append(format.getExtension());

		FileDialog fileDialog = new FileDialog(getShell(), SWT.SAVE);
		fileDialog.setFilterNames(new String[] { "PDF Files", "All Files" });
		fileDialog.setFilterExtensions(new String[] { "*.pdf", "*.*" });
		fileDialog.setFileName(nameTemplate.toString());
		final String fileName = fileDialog.open();

		if (fileName == null)
		   return;
		
      final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob("Rendering report", workbenchPart, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final File reportFile = session.renderReport(report.getId(), jobId, format);

				// save
				FileInputStream inputStream = null;
				FileOutputStream outputStream = null;
				try
				{
					inputStream = new FileInputStream(reportFile);
					outputStream = new FileOutputStream(fileName);

					byte[] buffer = new byte[1024];
					int size = 0;
					do
					{
						size = inputStream.read(buffer);
						if (size > 0)
						{
							outputStream.write(buffer, 0, size);
						}
					} while(size == buffer.length);

					outputStream.close();
					outputStream = null;

					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
							openReport(fileName, format);
						}
					});
				}
				finally
				{
					if (inputStream != null)
					{
						inputStream.close();
					}
					if (outputStream != null)
					{
						outputStream.close();
					}
				}
			}

			@Override
			protected String getErrorMessage()
			{
				return String.format("Cannot render report %1$s (job ID %2$s)", report.getName(), jobId);
			}
		}.start();
	}

	/**
	 * Execute report
	 */
	private void executeReport()
	{
		final Map<String, String> execParameters = new HashMap<String, String>();
		if (parameters != null)
		{
			for(int i = 0; i < parameters.size(); i++)
				execParameters.put(parameters.get(i).getName(), fields.get(i).getValue());
		}

		new ConsoleJob("Execute report", workbenchPart, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.executeReport(report.getId(), execParameters);
				getDisplay().asyncExec(new Runnable() {
					@Override
					public void run()
					{
						MessageDialogHelper.openInformation(getShell(), "Report Execution", 
								 report.getName() + " execution started successfully.");

						refreshResultList();
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return String.format("Cannot execute report %s", report.getName());
			}
		}.start();
	}

	/**
	 * @param workbenchPart
	 *            the workbenchPart to set
	 */
	public void setWorkbenchPart(IWorkbenchPart workbenchPart)
	{
		this.workbenchPart = workbenchPart;
	}

	/**
	 * Refresh result list
	 */
	private void refreshResultList()
	{
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob("Refresh result list for report " + report.getName(), workbenchPart, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final List<ReportResult> results = session.listReportResults(report.getId());

				getDisplay().asyncExec(new Runnable() {

					@Override
					public void run()
					{
						if (ReportExecutionForm.this.isDisposed())
						{
							return;
						}

						resultList.setInput(results.toArray());
						ReportExecutionForm.this.getParent().layout(true, true);
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return String.format("Cannot get result list for report %s", report.getName());
			}
		}.start();

	}
	
	/**
	 * Refresh schedule list
	 */
	private void refreshScheduleList()
	{
		new ConsoleJob(String.format("Refresh schedule list for report %s", report.getName()), workbenchPart, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final List<ReportingJob> results = session.listScheduledJobs(report.getId());
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						if (!scheduleList.getControl().isDisposed())
						   scheduleList.setInput(results.toArray());
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return String.format("Cannot get schedule list for report %s", report.getName());
			}
		}.start();

	}

	/**
	 * Delete selected report results
	 */
	private void deleteResults()
	{
		IStructuredSelection selection = (IStructuredSelection)resultList.getSelection();
		if (selection.size() == 0)
		{
			return;
		}

		if (!MessageDialogHelper.openConfirm(getShell(), "Delete Report Results", "Do you really want to delete selected results?"))
		{
			return;
		}

		final List<UUID> resultIdList = new ArrayList<UUID>(selection.size());
		for(Object o : selection.toList())
		{
			resultIdList.add(((ReportResult)o).getJobId());
		}

		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob("Delete report execution results", workbenchPart, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				for(UUID uuid : resultIdList)
				{
					session.deleteReportResult(report.getId(), uuid);
				}
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						if (!ReportExecutionForm.this.isDisposed())
							refreshResultList();
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot delete report results";
			}
		}.start();
	}

	/**
	 * Add schedule
	 */
	protected void addSchedule()
	{
		final Map<String, String> execParameters = new HashMap<String, String>();
		if (parameters != null)
		{
			for (int i = 0; i < parameters.size(); i++)
				execParameters.put(parameters.get(i).getName(), fields.get(i).getValue());
		}
		
		final ReportingJob job = new ReportingJob(report.getId());
		final PropertyDialog dialog = PropertyDialog.createDialogOn(workbenchPart.getSite().getShell(), General.ID, job);
		dialog.getShell().setText("Report Execution Schedule");
		if (dialog.open() != Window.OK)
		   return;
		
		new ConsoleJob("Adding report schedule", workbenchPart, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.scheduleReport(job, execParameters);
			}

			@Override
			protected String getErrorMessage()
			{
				return String.format("Cannot schedule report %s", report.getName());
			}
		}.start();
	}
	
	/**
	 * Delete selected schedules
	 */
	private void deleteSchedules()
	{
		IStructuredSelection selection = (IStructuredSelection) scheduleList.getSelection();
		if (selection.size() == 0)
		{
			return;
		}

		if (!MessageDialogHelper.openConfirm(getShell(), "Delete schedules", "Do you really want to delete selected schedules?"))
		{
			return;
		}

		final List<UUID> schedulesIdList = new ArrayList<UUID>(selection.size());
		for (Object o : selection.toList())
		{
			schedulesIdList.add(((ReportingJob) o).getJobId());
		}

		new ConsoleJob("Delete report schedules", workbenchPart, Activator.PLUGIN_ID, null)
		{

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				for (UUID uuid : schedulesIdList)
				{
					session.deleteReportSchedule(report.getId(), uuid);
				}
				getDisplay().asyncExec(new Runnable()
				{

					@Override
					public void run()
					{
						if (!ReportExecutionForm.this.isDisposed())
							refreshResultList();
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot delete report schedules";
			}
		}.start();
	}

	/**
	 * Open rendered report in appropriate external program (like PDF viewer)
	 * 
	 * @param fileName
	 *            rendered report file
	 */
	private void openReport(String fileName, ReportRenderFormat format)
	{
		final Program program = Program.findProgram(format.getExtension());
		if (program != null)
		{
			program.execute(fileName);
		}
		else
		{
			MessageDialogHelper.openError(getShell(), "Error",
					"Report was rendered successfully, but external viewer cannot be opened");
		}
	}
}
