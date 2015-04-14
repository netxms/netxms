package org.netxms.ui.eclipse.reporter.propertypages;

import java.util.Calendar;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.DateTime;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.reporting.ReportingJob;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "General" property page for schedule
 */
public class General extends PropertyPage
{
	public static final String ID = "org.netxms.ui.eclipse.reporter.propertypages.General"; //$NON-NLS-1$
	
	private Composite dailyComposite;
	private Composite weeklyComposite;
	private Composite monthlyComposite;
	private Composite onceComposite;
	
	private Button onceButton;
	private Button dailyButton;
	private Button weeklyButton;
	private Button monthlyButton;
	
	private Button[] monthlyButtons;
	private Button[] weeklyButtons;
	
	public int type;
	private DateTime executionDate;
	private DateTime executionTime;
	
	private ReportingJob job = null;

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		job = (ReportingJob)getElement().getAdapter(ReportingJob.class);

      final Composite dialogArea = new Composite(parent, SWT.NONE);
      GridLayout dialogLayout = new GridLayout();
      dialogLayout.marginWidth = 0;
      dialogLayout.marginHeight = 0;
      dialogLayout.numColumns = 4;
      dialogLayout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      dialogArea.setLayout(dialogLayout);
		
		onceButton = new Button(dialogArea, SWT.RADIO);
		onceButton.setText("Once");
		onceButton.addSelectionListener(new GranularitySelectorListener(ReportingJob.TYPE_ONCE, true, false, false, false));
		
		dailyButton = new Button(dialogArea, SWT.RADIO);
		dailyButton.setText("Daily");
		dailyButton.addSelectionListener(new GranularitySelectorListener(ReportingJob.TYPE_DAILY, false, true, false, false));
		
		weeklyButton = new Button(dialogArea, SWT.RADIO);
		weeklyButton.setLayoutData(new GridData(SWT.LEFT, SWT.CENTER, false, false, 1, 1));
		weeklyButton.setText("Weekly");
		weeklyButton.addSelectionListener(new GranularitySelectorListener(ReportingJob.TYPE_WEEKLY, false, false, true, false));
		
		monthlyButton = new Button(dialogArea, SWT.RADIO);
		monthlyButton.setLayoutData(new GridData(SWT.LEFT, SWT.CENTER, false, false, 1, 1));
		monthlyButton.setText("Monthly");
		monthlyButton.addSelectionListener(new GranularitySelectorListener(ReportingJob.TYPE_MONTHLY, false, false, false, true));

		onceComposite = createOnce(dialogArea);
		dailyComposite = createDaily(dialogArea);
		weeklyComposite = createWeekly(dialogArea);
		monthlyComposite = createMonthly(dialogArea);
		
		type = ReportingJob.TYPE_ONCE;
		onceButton.setSelection(true);

		return dialogArea;
	}

	/**
	 * @param group
	 * @return
	 */
	private Composite createOnce(Composite group)
	{
		final Composite onceComposite = new Composite(group, SWT.NONE);
		onceComposite.setLayoutData(new GridData(SWT.LEFT, SWT.CENTER, true, false, 4, 1));
		onceComposite.setLayout(new GridLayout(4, false));
		
		final DateTime startDate = new DateTime(onceComposite, SWT.DATE | SWT.DROP_DOWN);
		final DateTime startTime = new DateTime(onceComposite, SWT.TIME);
		executionTime = startTime;
		executionDate = startDate;
		
		onceButton.addSelectionListener(new SelectionListener() {
			
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				executionTime = startTime;
				executionDate = startDate;
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
			}
		});

		return onceComposite;
	}

	/**
	 * @param group
	 * @return
	 */
	private Composite createDaily(Composite group)
	{
		final Composite dailyComposite = new Composite(group, SWT.NONE);
		dailyComposite.setLayoutData(new GridData(SWT.LEFT, SWT.CENTER, true, false, 4, 1));
		dailyComposite.setLayout(new GridLayout(4, false));
		dailyComposite.setVisible(false);
		
		final DateTime startTime = new DateTime(dailyComposite, SWT.TIME);
		
		dailyButton.addSelectionListener(new SelectionListener() {
			
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				executionTime = startTime;
				executionDate = null;
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
			}
		});

		return dailyComposite;
	}

	/**
	 * @param group
	 * @return
	 */
	protected Composite createWeekly(Composite group)
	{	
		final Composite weeklyComposite = new Composite(group, SWT.NONE);
		weeklyComposite.setLayoutData(new GridData(SWT.LEFT, SWT.CENTER, true, false, 4, 1));
		weeklyComposite.setLayout(new GridLayout(7, false));
		weeklyComposite.setVisible(false);
		
		String[] dayOfWeek = { "MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN" }; //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$ //$NON-NLS-4$ //$NON-NLS-5$ //$NON-NLS-6$ //$NON-NLS-7$
		weeklyButtons = new Button[7];
		for (int i = 0; i < 7; i++)
		{
			weeklyButtons[i] = new Button(weeklyComposite, SWT.CHECK);
			weeklyButtons[i].setText(dayOfWeek[i]);
		}
		final DateTime startTime = new DateTime(weeklyComposite, SWT.TIME);
		startTime.setLayoutData(new GridData(SWT.LEFT, SWT.TOP, false, false, 7, 1));

		weeklyButton.addSelectionListener(new SelectionListener() {
			
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				executionTime = startTime;
				executionDate = null;
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
			}
		});

		return weeklyComposite;
	}

	/**
	 * @param group
	 * @return
	 */
	private Composite createMonthly(Composite group)
	{
		final Composite monthlyComposite = new Composite(group, SWT.NONE);
		monthlyComposite.setLayoutData(new GridData(SWT.LEFT, SWT.CENTER, true, false, 4, 1));
		monthlyComposite.setLayout(new GridLayout(7, true));
		monthlyComposite.setVisible(false);
		
		monthlyButtons = new Button[31];
		for (int i = 0; i < 31; i++)
		{
			monthlyButtons[i] = new Button(monthlyComposite, SWT.CHECK);
			monthlyButtons[i].setText(Integer.toString(i + 1));
		}
		final DateTime startTime = new DateTime(monthlyComposite, SWT.TIME);
		startTime.setLayoutData(new GridData(SWT.LEFT, SWT.TOP, false, false, 7, 1));

		monthlyButton.addSelectionListener(new SelectionListener() {
			
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				executionTime = startTime;
				executionDate = null;
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
			}
		});
				
		return monthlyComposite;
	}
	
	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected boolean applyChanges(final boolean isApply)
	{
		final Calendar calendar = Calendar.getInstance();
		switch (type)
		{
			case ReportingJob.TYPE_ONCE:
			{
				calendar.set(Calendar.YEAR, executionDate.getYear());
				calendar.set(Calendar.MONTH, executionDate.getMonth());
				calendar.set(Calendar.DAY_OF_MONTH, executionDate.getDay());
				calendar.set(Calendar.HOUR_OF_DAY, executionTime.getHours());
				calendar.set(Calendar.MINUTE, executionTime.getMinutes());
				break;
			}
			case ReportingJob.TYPE_DAILY:
			{
				calendar.set(Calendar.HOUR_OF_DAY, executionTime.getHours());
				calendar.set(Calendar.MINUTE, executionTime.getMinutes());
				break;
			}
			case ReportingJob.TYPE_WEEKLY:
			{
				calendar.set(Calendar.HOUR_OF_DAY, executionTime.getHours());
				calendar.set(Calendar.MINUTE, executionTime.getMinutes());
				int outWeeklyMask = 0;
				for (int i = 0; i < 7; i++)
				{
					outWeeklyMask <<= 1;
					outWeeklyMask += weeklyButtons[i].getSelection() ? 1 : 0;
				}
				job.setDaysOfWeek(outWeeklyMask);
				break;
			}
			case ReportingJob.TYPE_MONTHLY:
			{
				calendar.set(Calendar.HOUR_OF_DAY, executionTime.getHours());
				calendar.set(Calendar.MINUTE, executionTime.getMinutes());
				int outMonthlyMask = 0;
				for (int i = 0; i < 31; i++)
				{
					outMonthlyMask <<= 1;
					outMonthlyMask += monthlyButtons[i].getSelection() ? 1 : 0;
				}
				job.setDaysOfMonth(outMonthlyMask);
				break;
			}
			default:
				break;
		}
		job.setStartTime(calendar.getTime());
		job.setType(type);
		return true;
	}
	
	/**
	 * @param onceVisible
	 * @param dailyVisible
	 * @param weeklyVisible
	 * @param monthlyVisible
	 */
	private void changeVisibility(boolean onceVisible, boolean dailyVisible, boolean weeklyVisible, boolean monthlyVisible)
	{
		onceComposite.setVisible(onceVisible);
		((GridData)onceComposite.getLayoutData()).exclude = !onceVisible;
		dailyComposite.setVisible(dailyVisible);
		((GridData)dailyComposite.getLayoutData()).exclude = !dailyVisible;
		weeklyComposite.setVisible(weeklyVisible);
		((GridData)weeklyComposite.getLayoutData()).exclude = !weeklyVisible;
		monthlyComposite.setVisible(monthlyVisible);
		((GridData)monthlyComposite.getLayoutData()).exclude = !monthlyVisible;

		getShell().layout(true, true);
		getShell().pack();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
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
    *
    */
   private final class GranularitySelectorListener implements SelectionListener
   {
      private boolean dailyVisible;
      private boolean weeklyVisible;
      private boolean monthlyVisible;
      private Boolean onceVisible;
      private int type;

      private GranularitySelectorListener(int type, Boolean onceVisible, boolean dailyVisible, boolean weeklyVisible, boolean monthlyVisible)
      {
         this.type = type;
         this.onceVisible = onceVisible;
         this.dailyVisible = dailyVisible;
         this.weeklyVisible = weeklyVisible;
         this.monthlyVisible = monthlyVisible;
      }

      @Override
      public void widgetSelected(SelectionEvent e)
      {
         General.this.type = this.type;
         changeVisibility(onceVisible, dailyVisible, weeklyVisible, monthlyVisible);
      }

      @Override
      public void widgetDefaultSelected(SelectionEvent e)
      {
      }
   }
}
