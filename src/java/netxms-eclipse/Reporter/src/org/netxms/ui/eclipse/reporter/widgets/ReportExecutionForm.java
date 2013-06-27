/**
 * NetXMS - open source network management system
 * Copyright (C) 2013 Raden Solutions
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
import org.eclipse.swt.SWT;
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
import org.eclipse.ui.forms.widgets.TableWrapData;
import org.eclipse.ui.forms.widgets.TableWrapLayout;
import org.netxms.api.client.reporting.ReportDefinition;
import org.netxms.api.client.reporting.ReportParameter;
import org.netxms.api.client.reporting.ReportRenderFormat;
import org.netxms.api.client.reporting.ReportResult;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.reporter.Activator;
import org.netxms.ui.eclipse.reporter.api.CustomControlFactory;
import org.netxms.ui.eclipse.reporter.widgets.helpers.ReportResultLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.SharedIcons;
import org.netxms.ui.eclipse.tools.ImageCache;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Report execution widget. Provides form with report-specific parameters.
 */
public class ReportExecutionForm extends Composite
{
	private IWorkbenchPart workbenchPart = null;
	private ReportDefinition report;
	private FormToolkit toolkit;
	private ScrolledForm form;
	private List<FieldEditor> fields = new ArrayList<FieldEditor>();
	private SortableTableViewer resultList;
	private ImageCache imageCache;
	private List<ReportParameter> parameters;

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

		TableWrapLayout layout = new TableWrapLayout();
		layout.numColumns = 2;
		form.getBody().setLayout(layout);

		/* Parameters section */
		Section section = toolkit.createSection(form.getBody(), Section.DESCRIPTION | Section.TITLE_BAR);
		section.setText("Parameters");
		section.setDescription("Provide parameters necessary to run this report in fields below");
		TableWrapData td = new TableWrapData();
		td.align = TableWrapData.FILL;
		td.grabHorizontal = true;
		td.colspan = 2;
		section.setLayoutData(td);

		final Composite paramArea = toolkit.createComposite(section);
		layout = new TableWrapLayout();
		layout.numColumns = 2;
		paramArea.setLayout(layout);
		section.setClient(paramArea);
		createParamEntryFields(paramArea);

		/* Action section */
		section = toolkit.createSection(form.getBody(), Section.DESCRIPTION | Section.TITLE_BAR);
		section.setText("Actions");
		section.setDescription("Select desired action from the list below");
		td = new TableWrapData();
		td.align = TableWrapData.FILL;
		td.grabHorizontal = true;
		section.setLayoutData(td);

		final Composite actionArea = toolkit.createComposite(section);
		layout = new TableWrapLayout();
		actionArea.setLayout(layout);
		section.setClient(actionArea);

		ImageHyperlink link = toolkit.createImageHyperlink(actionArea, SWT.WRAP);
		link.setImage(SharedIcons.IMG_EXECUTE);
		link.setText("Execute report");
		link.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				executeReport();
			}
		});

		link = toolkit.createImageHyperlink(actionArea, SWT.WRAP);
		link.setImage(imageCache.add(Activator.getImageDescriptor("icons/schedule.png")));
		link.setText("Schedule report execution");
		link.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
			}
		});

		/* results section */
		section = toolkit.createSection(form.getBody(), Section.DESCRIPTION | Section.TITLE_BAR);
		section.setText("Results");
		section.setDescription("The following execution results are available for rendering");
		td = new TableWrapData();
		td.align = TableWrapData.FILL;
		td.grabHorizontal = true;
		td.grabVertical = true;
		section.setLayoutData(td);

		final Composite resultArea = toolkit.createComposite(section);
		layout = new TableWrapLayout();
		resultArea.setLayout(layout);
		section.setClient(resultArea);
		createResultsSection(resultArea);

		refreshResultList();
	}

	/**
	 * Create entry fields for parameters
	 * 
	 * @param parent
	 */
	private void createParamEntryFields(Composite parent)
	{
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
					// TODO: log error
				}
			}
			if (editor == null)
			{
				editor = new StringFieldEditor(parameter, toolkit, parent);
			}

			editors.put(parameter.getName(), editor);

			TableWrapData td = new TableWrapData();
			td.align = TableWrapData.FILL;
			td.grabHorizontal = true;
			System.out.println(parameter.getSpan());
			// td.colspan = parameter.getSpan();
			editor.setLayoutData(td);
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
	 * Create "Results" section's content
	 * 
	 * @param parent
	 *            parent composite
	 */
	private void createResultsSection(Composite parent)
	{
		GridLayout layout = new GridLayout();
		parent.setLayout(layout);

		ImageHyperlink link = toolkit.createImageHyperlink(parent, SWT.WRAP);
		link.setImage(SharedIcons.IMG_REFRESH);
		link.setText("Refresh");
		link.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				refreshResultList();
			}
		});
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.RIGHT;
		link.setLayoutData(gd);

		final String[] names = { "Execution Time", "Job ID" };
		final int[] widths = { 130, 250 };
		resultList = new SortableTableViewer(parent, names, widths, 0, SWT.DOWN, SWT.BORDER | SWT.FULL_SELECTION | SWT.MULTI);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		resultList.getControl().setLayoutData(gd);
		resultList.setContentProvider(new ArrayContentProvider());
		resultList.setLabelProvider(new ReportResultLabelProvider());

		link = toolkit.createImageHyperlink(parent, SWT.WRAP);
		link.setImage(imageCache.add(Activator.getImageDescriptor("icons/pdf.png")));
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
		link.setImage(SharedIcons.IMG_DELETE_OBJECT);
		link.setText("Delete");
		link.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				deleteResults();
			}
		});
	}

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

		if (fileName != null)
		{
			new ConsoleJob("Rendering report", workbenchPart, Activator.PLUGIN_ID, null) {
				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
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

						getDisplay().asyncExec(new Runnable() {
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
					return "Cannot render report " + report.getName() + " job " + jobId;
				}
			}.start();
		}
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
			{
				execParameters.put(parameters.get(i).getName(), fields.get(i).getValue());
			}
		}

		new ConsoleJob("Execute report", workbenchPart, Activator.PLUGIN_ID, null) {

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
				final UUID jobId = session.executeReport(report.getId(), execParameters);
				getDisplay().asyncExec(new Runnable() {

					@Override
					public void run()
					{
						MessageDialogHelper.openInformation(getShell(), "Report Execution", report.getName()
								+ " execution started successfully.");
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot execute report " + report.getName();
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
				return "Cannot get result list for report " + report.getName();
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
				getDisplay().asyncExec(new Runnable() {

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
				return null;
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
