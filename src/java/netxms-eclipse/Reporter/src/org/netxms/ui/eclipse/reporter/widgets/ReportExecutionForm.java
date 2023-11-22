/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.program.Program;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.widgets.ImageHyperlink;
import org.eclipse.ui.internal.dialogs.PropertyDialog;
import org.netxms.client.NXCSession;
import org.netxms.client.ScheduledTask;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.reporting.ReportDefinition;
import org.netxms.client.reporting.ReportParameter;
import org.netxms.client.reporting.ReportRenderFormat;
import org.netxms.client.reporting.ReportResult;
import org.netxms.client.reporting.ReportingJob;
import org.netxms.client.reporting.ReportingJobConfiguration;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.reporter.Activator;
import org.netxms.ui.eclipse.reporter.api.CustomControlFactory;
import org.netxms.ui.eclipse.reporter.propertypages.General;
import org.netxms.ui.eclipse.reporter.widgets.helpers.ReportResultComparator;
import org.netxms.ui.eclipse.reporter.widgets.helpers.ReportResultLabelProvider;
import org.netxms.ui.eclipse.reporter.widgets.helpers.ScheduleLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.ImageCache;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.Section;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Report execution widget. Provides form with report-specific parameters.
 */
@SuppressWarnings("restriction")
public class ReportExecutionForm extends Composite
{
   public static final int COLUMN_SCHEDULE_EXEC_TIME = 0;
   public static final int COLUMN_SCHEDULE_OWNER = 1;
   public static final int COLUMN_SCHEDULE_LAST_EXEC_TIME = 2;
   public static final int COLUMN_SCHEDULE_STATUS = 3;

	public static final int COLUMN_RESULT_EXEC_TIME = 0;
   public static final int COLUMN_RESULT_STARTED_BY = 1;
   public static final int COLUMN_RESULT_STATUS = 2;

   private NXCSession session = ConsoleSharedData.getSession();

	private IWorkbenchPart workbenchPart = null;
	private ImageCache imageCache;
	private SortableTableViewer resultList;
	private SortableTableViewer scheduleList;

	private ReportDefinition report;
	private List<ReportParameter> parameters;
	private List<FieldEditor> fields = new ArrayList<FieldEditor>();

	private Action actionDeleteSchedule;
   private Action actionDeleteResult;
   private Action actionRenderXLSX;
   private Action actionRenderPDF;

	/**
	 * @param parent
	 * @param style
	 */
	public ReportExecutionForm(Composite parent, int style, ReportDefinition report, IWorkbenchPart workbenchPart)
	{		
		super(parent, style);
		this.report = report;
		this.workbenchPart = workbenchPart;
		
		createActions();

		imageCache = new ImageCache(this);

		GridLayout layout = new GridLayout();
      layout.numColumns = 2;
		layout.makeColumnsEqualWidth = true;
      setLayout(layout);

		// Parameters section
      Section section = new Section(this, "Parameters", true);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
		section.setLayoutData(gd);

      final Composite paramArea = new Composite(section.getClient(), SWT.NONE);
		createParamEntryFields(paramArea);

      // Action links
      Composite actionArea = new Composite(this, SWT.NONE);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      actionArea.setLayoutData(gd);
      actionArea.setLayout(new RowLayout(SWT.HORIZONTAL));

      ImageHyperlink linkExecute = new ImageHyperlink(actionArea, SWT.WRAP);
      linkExecute.setImage(SharedIcons.IMG_EXECUTE);
      linkExecute.setText("Execute now");
      linkExecute.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            executeReport();
         }
      });

      ImageHyperlink linkSchedule = new ImageHyperlink(actionArea, SWT.WRAP);
      linkSchedule.setImage(imageCache.add(Activator.getImageDescriptor("icons/schedule.png"))); //$NON-NLS-1$
      linkSchedule.setText("Schedule execution");
      linkSchedule.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            scheduleExecution();
         }
      });

      // Results section
      section = new Section(this, "Results", false);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      section.setLayoutData(gd);
      
      createResultsSection(section.getClient());

		// Schedules section
      section = new Section(this, "Schedules", false);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
		section.setLayoutData(gd);

      createSchedulesSection(section.getClient());

      final SessionListener sessionListener = new SessionListener() {
			@Override
			public void notificationHandler(SessionNotification n) 
			{
            if (isDisposed())
               return;

            if (n.getCode() == SessionNotification.SCHEDULE_UPDATE)
				{
               getDisplay().asyncExec(() -> refreshScheduleList());
				} 
				else if (n.getCode() == SessionNotification.RS_RESULTS_MODIFIED) 
				{
               getDisplay().asyncExec(() -> refreshResultList());
				}
			}
      };
      session.addListener(sessionListener);
      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            session.removeListener(sessionListener);
         }
      });

		refreshScheduleList();
		refreshResultList();
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
	   actionDeleteSchedule = new Action("&Delete", SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteSchedules();
         }
      };
      actionDeleteSchedule.setEnabled(false);

      actionDeleteResult = new Action("&Delete", SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteResults();
         }
      };
      actionDeleteResult.setEnabled(false);

      actionRenderPDF = new Action("Render to &PDF", Activator.getImageDescriptor("icons/pdf.png")) {
         @Override
         public void run()
         {
            renderSelectedResult(ReportRenderFormat.PDF);
         }
      };
      actionRenderPDF.setEnabled(false);

      actionRenderXLSX = new Action("Render to &XLSX", Activator.getImageDescriptor("icons/xls.png")) {
         @Override
         public void run()
         {
            renderSelectedResult(ReportRenderFormat.XLSX);
         }
      };
      actionRenderXLSX.setEnabled(false);
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
      final String[] names = { "Schedule", "Owner", "Last Run", "Status" };
      final int[] widths = { 200, 140, 150, 250 };
      scheduleList = new SortableTableViewer(parent, names, widths, 0, SWT.DOWN, SWT.FULL_SELECTION | SWT.MULTI);
		scheduleList.setContentProvider(new ArrayContentProvider());
		scheduleList.setLabelProvider(new ScheduleLabelProvider(scheduleList));

		WidgetHelper.restoreTableViewerSettings(scheduleList, Activator.getDefault().getDialogSettings(), "ReportExecutionForm.ScheduleList");
		scheduleList.getControl().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(scheduleList, Activator.getDefault().getDialogSettings(), "ReportExecutionForm.ScheduleList");
         }
      });

	   createSchedulesContextMenu();

	   scheduleList.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = scheduleList.getStructuredSelection();
            actionDeleteSchedule.setEnabled(selection.size() > 0);
         }
      });
	}
	
   /**
    * Create schedules context menu
    */
   private void createSchedulesContextMenu()
   {
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillSchedulesContextMenu(mgr);
         }
      });

      // Create menu.
      Menu menu = menuMgr.createContextMenu(scheduleList.getControl());
      scheduleList.getControl().setMenu(menu);
   }
	
   /**
    * Fill schedules context menu
    * @param mgr Menu manager
    */
   private void fillSchedulesContextMenu(IMenuManager manager)
   {
      manager.add(actionDeleteSchedule);
   }

	/**
	 * Create "Results" section's content
	 * 
	 * @param parent
	 *            parent composite
	 */
	private void createResultsSection(Composite parent)
	{
		final String[] names = { "Execution Time", "Started by", "Status" };
		final int[] widths = { 180, 140, 100 };
      resultList = new SortableTableViewer(parent, names, widths, 0, SWT.DOWN, SWT.FULL_SELECTION | SWT.MULTI);
		resultList.setContentProvider(new ArrayContentProvider());
		resultList.setLabelProvider(new ReportResultLabelProvider(resultList));
      resultList.setComparator(new ReportResultComparator());

      WidgetHelper.restoreTableViewerSettings(resultList, Activator.getDefault().getDialogSettings(), "ReportExecutionForm.ResultList");
      resultList.getControl().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(resultList, Activator.getDefault().getDialogSettings(), "ReportExecutionForm.ResultList");
         }
      });

		createResultsContextMenu();

		resultList.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = resultList.getStructuredSelection();
            actionDeleteResult.setEnabled(selection.size() > 0);
            actionRenderPDF.setEnabled(selection.size() == 1);
            actionRenderXLSX.setEnabled(selection.size() == 1);
         }
      });
	}

   /**
    * Create results context menu
    */
   private void createResultsContextMenu()
   {
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillResultsContextMenu(mgr);
         }
      });

      // Create menu.
      Menu menu = menuMgr.createContextMenu(resultList.getControl());
      resultList.getControl().setMenu(menu);
   }

   /**
    * Fill results context menu
    * @param mgr Menu manager
    */
   private void fillResultsContextMenu(IMenuManager manager)
   {
      IStructuredSelection selection = resultList.getStructuredSelection();
      if ((selection.size() == 1) && ((ReportResult)selection.getFirstElement()).isSuccess())
      {
         manager.add(actionRenderPDF);
         manager.add(actionRenderXLSX);
         manager.add(new Separator());
      }
      manager.add(actionDeleteResult);
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
      layout.verticalSpacing = 15;
      layout.horizontalSpacing = 20;
      parent.setLayout(layout);

		final IExtensionRegistry reg = Platform.getExtensionRegistry();
		IConfigurationElement[] elements = reg.getConfigurationElementsFor("org.netxms.ui.eclipse.reporter.customfields"); //$NON-NLS-1$
		sortFieldProviders(elements);

		parameters = report.getParameters();
      if (parameters.isEmpty())
      {
         Label label = new Label(parent, SWT.NONE);
         label.setText("This report does not have parameters");
         return;
      }

		Map<String, FieldEditor> editors = new HashMap<String, FieldEditor>();

      int consumedColumns = 0;
      Label separator = null;
		for(ReportParameter parameter : parameters)
		{
			FieldEditor editor = null;
			for(int i = 0; i < elements.length; i++)
			{
				try
				{
					final CustomControlFactory provider = (CustomControlFactory)elements[i].createExecutableExtension("class"); //$NON-NLS-1$
               editor = provider.editorForType(parent, parameter);
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
            editor = new StringFieldEditor(parameter, parent);
			}

			GridData gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.verticalAlignment = SWT.TOP;
			gd.grabExcessHorizontalSpace = true;
			gd.grabExcessVerticalSpace = false;
			gd.horizontalSpan = parameter.getSpan();
			editor.setLayoutData(gd);

			if (layout.numColumns < parameter.getSpan())
			   layout.numColumns = parameter.getSpan();

         editors.put(parameter.getName(), editor);
         fields.add(editor);

         consumedColumns += parameter.getSpan();
         if (consumedColumns >= layout.numColumns)
         {
            separator = new Label(parent, SWT.SEPARATOR | SWT.HORIZONTAL);
            separator.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, layout.numColumns, 1));
            consumedColumns = 0;
         }
		}

      if ((separator != null) && (consumedColumns == 0))
         separator.dispose();

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
      Arrays.sort(elements, (o1, o2) -> {
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
		});
	}

	/**
	 * Render currently selected report result
	 * 
	 * @param format rendering format
	 */
	private void renderSelectedResult(ReportRenderFormat format)
	{
      IStructuredSelection selection = resultList.getStructuredSelection();
	   if (selection.size() != 1)
	      return;

      ReportResult r = (ReportResult)selection.getFirstElement();
      renderReport(r.getJobId(), r.getExecutionTime(), format);
	}

	/**
	 * @param jobId
	 * @param executionTime
	 * @param format
	 */
	private void renderReport(final UUID jobId, Date executionTime, final ReportRenderFormat format)
	{
		final StringBuilder nameTemplate = new StringBuilder();
		nameTemplate.append(report.getName());
		nameTemplate.append(" "); //$NON-NLS-1$
		nameTemplate.append(new SimpleDateFormat("ddMMyyyy HHmm").format(executionTime)); //$NON-NLS-1$
		nameTemplate.append("."); //$NON-NLS-1$
		nameTemplate.append(format.getExtension());

		FileDialog fileDialog = new FileDialog(getShell(), SWT.SAVE);
		switch(format)
		{
			case PDF:
				fileDialog.setFilterNames(new String[] { "PDF Files", "All Files" });
				fileDialog.setFilterExtensions(new String[] { "*.pdf", "*.*" }); //$NON-NLS-1$ //$NON-NLS-2$
				break;
         case XLSX:
				fileDialog.setFilterNames(new String[] { "Excel Files", "All Files" });
            fileDialog.setFilterExtensions(new String[] { "*.xlsx", "*.*" }); //$NON-NLS-1$ //$NON-NLS-2$
				break;
			default:
				fileDialog.setFilterNames(new String[] { "All Files" });
				fileDialog.setFilterExtensions(new String[] { "*.*" }); //$NON-NLS-1$
				break;
		}
		fileDialog.setFileName(nameTemplate.toString());
		final String fileName = fileDialog.open();

		if (fileName == null)
		   return;
		
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

               runInUIThread(() -> openReport(fileName, format));
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
   public void executeReport()
	{
      final ReportingJobConfiguration jobConfiguration = new ReportingJobConfiguration(report.getId());
		if (parameters != null)
		{
			for(int i = 0; i < parameters.size(); i++)
            jobConfiguration.executionParameters.put(parameters.get(i).getName(), fields.get(i).getValue());
		}

		new ConsoleJob("Execute report", workbenchPart, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
            session.executeReport(jobConfiguration);
            runInUIThread(() -> {
               MessageDialogHelper.openInformation(getShell(), "Report Execution", report.getName() + " execution started successfully.");
               refreshResultList();
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
	 * Refresh result list
	 */
	private void refreshResultList()
	{
		new ConsoleJob(String.format("Refresh result list for report %s", report.getName()), workbenchPart, Activator.PLUGIN_ID) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
            final List<ReportResult> results = session.getReportResults(report.getId());
            runInUIThread(() -> {
               if (ReportExecutionForm.this.isDisposed())
                  return;

               resultList.setInput(results.toArray());
               ReportExecutionForm.this.getParent().layout(true, true);
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
		new ConsoleJob(String.format("Refresh schedule list for report %s", report.getName()), workbenchPart, Activator.PLUGIN_ID) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
            final List<ReportingJob> results = session.getScheduledReportingJobs(report.getId());
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
      IStructuredSelection selection = resultList.getStructuredSelection();
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

		new ConsoleJob("Delete report execution results", workbenchPart, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				for(UUID uuid : resultIdList)
				{
					session.deleteReportResult(report.getId(), uuid);
				}
            runInUIThread(() -> {
               if (!ReportExecutionForm.this.isDisposed())
                  refreshResultList();
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
   public void scheduleExecution()
	{
      final ReportingJob job = new ReportingJob(report);
		if (parameters != null)
		{
			for (int i = 0; i < parameters.size(); i++)
				job.setExecutionParameter(parameters.get(i).getName(), fields.get(i).getValue());
		}
		
		final PropertyDialog dialog = PropertyDialog.createDialogOn(workbenchPart.getSite().getShell(), General.ID, job);
		dialog.getShell().setText("Report Execution Schedule");
		if (dialog.open() != Window.OK)
		   return;
		
		new ConsoleJob("Adding report schedule", workbenchPart, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
            ScheduledTask task = job.prepareTask();
            task.setComments(report.getName());
            session.addScheduledTask(task);
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
      IStructuredSelection selection = scheduleList.getStructuredSelection();
      if (selection.isEmpty())
			return;

		if (!MessageDialogHelper.openConfirm(getShell(), "Delete schedules", "Do you really want to delete selected schedules?"))
			return;

      final long[] taskIdList = new long[selection.size()];
      int i = 0;
		for (Object o : selection.toList())
		{
         taskIdList[i++] = ((ReportingJob)o).getTask().getId();
		}

      new ConsoleJob("Delete report schedules", workbenchPart, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
            for(long id : taskIdList)
				{
               session.deleteScheduledTask(id);
				}
            runInUIThread(() -> {
               if (!ReportExecutionForm.this.isDisposed())
                  refreshScheduleList();
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
