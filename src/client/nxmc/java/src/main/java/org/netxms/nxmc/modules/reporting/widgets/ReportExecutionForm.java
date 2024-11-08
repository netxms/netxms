/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.ServiceLoader;
import java.util.UUID;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.preference.PreferenceNode;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
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
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Shell;
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
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.ImageHyperlink;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.base.widgets.Section;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.base.widgets.events.HyperlinkAdapter;
import org.netxms.nxmc.base.widgets.events.HyperlinkEvent;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.reporting.propertypages.General;
import org.netxms.nxmc.modules.reporting.propertypages.Notifications;
import org.netxms.nxmc.modules.reporting.widgets.helpers.ReportRenderingHelper;
import org.netxms.nxmc.modules.reporting.widgets.helpers.ReportResultComparator;
import org.netxms.nxmc.modules.reporting.widgets.helpers.ReportResultLabelProvider;
import org.netxms.nxmc.modules.reporting.widgets.helpers.ScheduleLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.services.ReportFieldEditorFactory;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Report execution widget. Provides form with report-specific parameters.
 */
public class ReportExecutionForm extends Composite
{
   public static final int COLUMN_SCHEDULE_EXEC_TIME = 0;
   public static final int COLUMN_SCHEDULE_OWNER = 1;
   public static final int COLUMN_SCHEDULE_LAST_EXEC_TIME = 2;
   public static final int COLUMN_SCHEDULE_STATUS = 3;

	public static final int COLUMN_RESULT_EXEC_TIME = 0;
   public static final int COLUMN_RESULT_STARTED_BY = 1;
   public static final int COLUMN_RESULT_STATUS = 2;

   private final I18n i18n = LocalizationHelper.getI18n(ReportExecutionForm.class);

   private NXCSession session = Registry.getSession();

   private View view;
	private SortableTableViewer resultList;
	private SortableTableViewer scheduleList;

	private ReportDefinition report;
	private List<ReportParameter> parameters;
	private List<ReportFieldEditor> fields = new ArrayList<ReportFieldEditor>();

	private Action actionDeleteSchedule;
   private Action actionDeleteResult;
   private Action actionRenderXLSX;
   private Action actionRenderPDF;
   private Action actionViewResult;

	/**
	 * @param parent
	 * @param style
	 */
   public ReportExecutionForm(Composite parent, int style, ReportDefinition report, View view)
	{		
		super(parent, style);
		this.report = report;
      this.view = view;

		createActions();

		GridLayout layout = new GridLayout();
      layout.numColumns = 2;
		layout.makeColumnsEqualWidth = true;
      setLayout(layout);

		// Parameters section
      Section section = new Section(this, i18n.tr("Parameters"), true);
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
      linkExecute.setText(i18n.tr("Execute now"));
      linkExecute.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            executeReport();
         }
      });

      ImageHyperlink linkSchedule = new ImageHyperlink(actionArea, SWT.WRAP);
      linkSchedule.setImage(SharedIcons.IMG_CALENDAR);
      linkSchedule.setText(i18n.tr("Schedule execution"));
      linkSchedule.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            scheduleExecution();
         }
      });

      // Results section
      section = new Section(this, i18n.tr("Results"), false);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      section.setLayoutData(gd);

      createResultsSection(section.getClient());

		// Schedules section
      section = new Section(this, i18n.tr("Schedules"), false);
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

      actionRenderPDF = new Action("Render to &PDF", ResourceManager.getImageDescriptor("icons/file-types/pdf.png")) {
         @Override
         public void run()
         {
            renderSelectedResult(ReportRenderFormat.PDF, false);
         }
      };
      actionRenderPDF.setEnabled(false);

      actionRenderXLSX = new Action("Render to &XLSX", ResourceManager.getImageDescriptor("icons/file-types/xls.png")) {
         @Override
         public void run()
         {
            renderSelectedResult(ReportRenderFormat.XLSX, false);
         }
      };
      actionRenderXLSX.setEnabled(false);

      actionViewResult = new Action("Pre&view", SharedIcons.PREVIEW) {
         @Override
         public void run()
         {
            renderSelectedResult(ReportRenderFormat.PDF, true);
         }
      };
      actionViewResult.setEnabled(false);
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

      WidgetHelper.restoreTableViewerSettings(scheduleList, "ReportExecutionForm.ScheduleList");
		scheduleList.getControl().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(scheduleList, "ReportExecutionForm.ScheduleList");
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

      WidgetHelper.restoreTableViewerSettings(resultList, "ReportExecutionForm.ResultList");
      resultList.getControl().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTableViewerSettings(resultList, "ReportExecutionForm.ResultList");
         }
      });

		createResultsContextMenu();

		resultList.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = resultList.getStructuredSelection();
            actionDeleteResult.setEnabled(selection.size() > 0);
            if (selection.size() == 1)
            {
               ReportResult r = (ReportResult)selection.getFirstElement();
               actionViewResult.setEnabled(!r.isCarboneReport());
               actionRenderPDF.setEnabled(!r.isCarboneReport());
               actionRenderXLSX.setEnabled(true);
            }
            else
            {
               actionViewResult.setEnabled(false);
               actionRenderPDF.setEnabled(false);
               actionRenderXLSX.setEnabled(false);
            }
         }
      });

      resultList.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            renderSelectedResult(ReportRenderFormat.PDF, true);
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
         manager.add(actionViewResult);
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

      parameters = report.getParameters();
      if (parameters.isEmpty())
      {
         Label label = new Label(parent, SWT.NONE);
         label.setText("This report does not have parameters");
         return;
      }

      List<ReportFieldEditorFactory> fieldEditorFactories = new ArrayList<>(4);
      ServiceLoader<ReportFieldEditorFactory> loader = ServiceLoader.load(ReportFieldEditorFactory.class, getClass().getClassLoader());
      for(ReportFieldEditorFactory f : loader)
         fieldEditorFactories.add(f);
      Collections.sort(fieldEditorFactories, (f1, f2) -> f2.getPriority() - f1.getPriority());

		Map<String, ReportFieldEditor> editors = new HashMap<String, ReportFieldEditor>();

      int consumedColumns = 0;
      Label separator = null;
		for(ReportParameter parameter : parameters)
		{
			ReportFieldEditor editor = null;
         for(ReportFieldEditorFactory f : fieldEditorFactories)
			{
            editor = f.editorForType(parent, parameter);
            if (editor != null)
               break;
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
			final ReportFieldEditor editor = editors.get(parameter.getName());
			final ReportFieldEditor parentEditor = editors.get(parameter.getDependsOn());
			if (editor != null && parentEditor != null)
			{
				parentEditor.setDependantEditor(editor);
			}
		}
	}

	/**
    * Render currently selected report result
    * 
    * @param format rendering format
    * @param preview true for preview mode
    */
   private void renderSelectedResult(ReportRenderFormat format, boolean preview)
	{
      IStructuredSelection selection = resultList.getStructuredSelection();
	   if (selection.size() != 1)
	      return;

      ReportResult r = (ReportResult)selection.getFirstElement();
      new ReportRenderingHelper(view, report, r.getJobId(), r.getExecutionTime(), format, preview).renderReport();
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

      new Job(i18n.tr("Executing report"), view) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
            session.executeReport(jobConfiguration);
            runInUIThread(() -> {
               view.addMessage(MessageArea.SUCCESS, i18n.tr("Execution of report {0} successfully started", report.getName()));
               refreshResultList();
				});
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot execute report {0}", report.getName());
			}
		}.start();
	}

	/**
	 * Refresh result list
	 */
	private void refreshResultList()
	{
      new Job(i18n.tr("Loading result list for report {0}", report.getName()), view) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
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
            return i18n.tr("Cannot get result list for report {0}", report.getName());
			}
		}.start();
	}

	/**
	 * Refresh schedule list
	 */
	private void refreshScheduleList()
	{
      new Job(i18n.tr("Loaidng schedule list for report {0}", report.getName()), view) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
            final List<ReportingJob> results = session.getScheduledReportingJobs(report.getId());
            runInUIThread(() -> {
               if (!scheduleList.getControl().isDisposed())
                  scheduleList.setInput(results.toArray());
				});
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot get schedule list for report {0}", report.getName());
			}
		}.start();
	}

	/**
	 * Delete selected report results
	 */
	private void deleteResults()
	{
      IStructuredSelection selection = resultList.getStructuredSelection();
      if (selection.isEmpty())
			return;

      if (!MessageDialogHelper.openConfirm(getShell(), i18n.tr("Delete Report Results"), i18n.tr("Do you really want to delete selected results?")))
			return;

		final List<UUID> resultIdList = new ArrayList<UUID>(selection.size());
		for(Object o : selection.toList())
			resultIdList.add(((ReportResult)o).getJobId());

      new Job(i18n.tr("Deleting report execution results"), view) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
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
            return i18n.tr("Cannot delete report execution results");
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

      PreferenceManager pm = new PreferenceManager();
      pm.addToRoot(new PreferenceNode("general", new General(job)));
      pm.addToRoot(new PreferenceNode("notifications", new Notifications(job)));

      PreferenceDialog dlg = new PreferenceDialog(getShell(), pm) {
         @Override
         protected void configureShell(Shell newShell)
         {
            super.configureShell(newShell);
            newShell.setText(i18n.tr("Report Execution Schedule"));
         }
      };
      dlg.setBlockOnOpen(true);
      if (dlg.open() != Window.OK)
         return;

      new Job(i18n.tr("Scheduling report execution"), view) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
            ScheduledTask task = job.prepareTask();
            task.setComments(report.getName());
            session.addScheduledTask(task);
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot schedule report {0}", report.getName());
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

      if (!MessageDialogHelper.openConfirm(getShell(), i18n.tr("Delete Schedules"), i18n.tr("Do you really want to delete selected schedules?")))
			return;

      final long[] taskIdList = new long[selection.size()];
      int i = 0;
		for (Object o : selection.toList())
		{
         taskIdList[i++] = ((ReportingJob)o).getTask().getId();
		}

      new Job(i18n.tr("Deleting report schedules"), view) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
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
            return i18n.tr("Cannot delete report schedules");
			}
		}.start();
	}
}
