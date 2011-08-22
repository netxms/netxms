/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.eclipse.ui.forms.widgets.ImageHyperlink;
import org.eclipse.ui.forms.widgets.ScrolledForm;
import org.eclipse.ui.forms.widgets.Section;
import org.eclipse.ui.forms.widgets.TableWrapData;
import org.eclipse.ui.forms.widgets.TableWrapLayout;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Report;
import org.netxms.client.reports.ReportResult;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.reporter.Activator;
import org.netxms.ui.eclipse.reporter.widgets.helpers.ReportDefinition;
import org.netxms.ui.eclipse.reporter.widgets.helpers.ReportParameter;
import org.netxms.ui.eclipse.reporter.widgets.helpers.ReportResultLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.SharedIcons;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Report execution widget. Provides form with report-specific parameters.
 */
public class ReportExecutionForm extends Composite
{
	private IWorkbenchPart workbenchPart = null;
	private Report report;
	private FormToolkit toolkit;
	private ScrolledForm form;
	private List<FieldEditor> fields = new ArrayList<FieldEditor>();
	private List<ReportParameter> parameters;
	private SortableTableViewer resultList;

	/**
	 * @param parent
	 * @param style
	 */
	public ReportExecutionForm(Composite parent, int style, Report report)
	{
		super(parent, style);
		this.report = report;

		setLayout(new FillLayout());

		/* FORM */
		toolkit = new FormToolkit(getDisplay());
		form = toolkit.createScrolledForm(this);
		form.setText(report.getObjectName());

		TableWrapLayout layout = new TableWrapLayout();
		layout.numColumns = 2;
		form.getBody().setLayout(layout);

		if (!report.getComments().isEmpty())
			toolkit.createLabel(form.getBody(), report.getComments(), SWT.WRAP);

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
		link.setImage(Activator.getImageDescriptor("icons/execute.gif").createImage());
		link.setText("Execute report");
		link.addHyperlinkListener(new HyperlinkAdapter()
		{
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				executeReport();
			}
		});

		link = toolkit.createImageHyperlink(actionArea, SWT.WRAP);
		link.setImage(Activator.getImageDescriptor("icons/schedule.png").createImage());
		link.setText("Schedule report execution");
		link.addHyperlinkListener(new HyperlinkAdapter()
		{
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
		try
		{
			final ReportDefinition definition = ReportDefinition.createFromXml(report.getDefinition());
			parameters = definition.getParameters();
			for(ReportParameter p : parameters)
			{
				FieldEditor editor;
				switch(p.getDataType())
				{
					case ReportParameter.INTEGER:
						editor = new StringFieldEditor(p, toolkit, parent);
						break;
					case ReportParameter.TIMESTAMP:
						editor = new TimestampFieldEditor(p, toolkit, parent);
						break;
					default: // everything else as string
						editor = new StringFieldEditor(p, toolkit, parent);
						break;
				}

				TableWrapData td = new TableWrapData();
				td.align = TableWrapData.FILL;
				td.grabHorizontal = true;
				td.colspan = p.getColumnSpan();
				editor.setLayoutData(td);
				fields.add(editor);
			}
		}
		catch(Exception e)
		{
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}

	/**
	 * Create "Results" section's content
	 * 
	 * @param parent
	 *           parent composite
	 */
	private void createResultsSection(Composite parent)
	{
		GridLayout layout = new GridLayout();
		parent.setLayout(layout);

		ImageHyperlink link = toolkit.createImageHyperlink(parent, SWT.WRAP);
		link.setImage(SharedIcons.REFRESH.createImage());
		link.setText("Refresh");
		link.addHyperlinkListener(new HyperlinkAdapter()
		{
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				refreshResultList();
			}
		});
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.RIGHT;
		link.setLayoutData(gd);

		final String[] names = { "Job ID", "Execution Time" };
		final int[] widths = { 90, 160 };
		resultList = new SortableTableViewer(parent, names, widths, 0, SWT.DOWN, SWT.BORDER);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		resultList.getControl().setLayoutData(gd);
		resultList.setContentProvider(new ArrayContentProvider());
		resultList.setLabelProvider(new ReportResultLabelProvider());

		link = toolkit.createImageHyperlink(parent, SWT.WRAP);
		link.setImage(Activator.getImageDescriptor("icons/pdf.png").createImage());
		link.setText("Render to PDF");
		link.addHyperlinkListener(new HyperlinkAdapter()
		{
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				final ReportResult firstElement = (ReportResult)((IStructuredSelection)resultList.getSelection()).getFirstElement();
				if (firstElement != null)
				{
					renderReport(firstElement.getJobId());
				}
			}
		});
	}

	private void renderReport(final long jobId)
	{
		new ConsoleJob("Rendering report", workbenchPart, Activator.PLUGIN_ID, null)
		{
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				NXCSession session = (NXCSession)ConsoleSharedData.getSession();
				final byte[] blob = session.renderReport(jobId);

				// TODO: fix this
				final File tempFile = File.createTempFile("nxreport", ".pdf");
				FileOutputStream stream = null;
				try
				{
					stream = new FileOutputStream(tempFile);
					stream.write(blob);
				}
				finally
				{
					if (stream != null)
					{
						stream.close();
					}
				}

				getDisplay().asyncExec(new Runnable()
				{
					@Override
					public void run()
					{
						try
						{
							// TODO: win32 only
							Runtime.getRuntime().exec("rundll32 SHELL32.DLL,ShellExec_RunDLL " + tempFile.getAbsolutePath());
						}
						catch(IOException e)
						{
							e.printStackTrace();
						}
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot execute report " + report.getObjectName();
			}
		}.start();
	}

	/**
	 * Execute report
	 */
	private void executeReport()
	{
		final Map<String, String> execParameters = new HashMap<String, String>(parameters.size());
		for(int i = 0; i < parameters.size(); i++)
		{
			execParameters.put(parameters.get(i).getName(), fields.get(i).getValue());
		}

		new ConsoleJob("Execute report", workbenchPart, Activator.PLUGIN_ID, null)
		{
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				NXCSession session = (NXCSession)ConsoleSharedData.getSession();
				final long jobId = session.executeReport(report.getObjectId(), execParameters);
				getDisplay().asyncExec(new Runnable()
				{
					@Override
					public void run()
					{
						MessageDialog.openInformation(getShell(), "Report Execution", "Report " + report.getObjectName()
								+ " execution started successfully. Job ID is " + jobId + ".");
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot execute report " + report.getObjectName();
			}
		}.start();
	}

	/**
	 * @param workbenchPart
	 *           the workbenchPart to set
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
		new ConsoleJob("Refresh result list for report " + report.getObjectName(), workbenchPart, Activator.PLUGIN_ID, null)
		{
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final List<ReportResult> results = session.getReportResults(report.getObjectId());
				getDisplay().asyncExec(new Runnable()
				{
					@Override
					public void run()
					{
						if (ReportExecutionForm.this.isDisposed())
							return;

						resultList.setInput(results.toArray());
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot get result list for report " + report.getObjectName();
			}
		}.start();
	}
}
