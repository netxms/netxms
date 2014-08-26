package org.netxms.ui.eclipse.widgets;

import java.util.Date;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Spinner;
import org.netxms.client.datacollection.GraphSettings;
import org.netxms.ui.eclipse.tools.WidgetFactory;
import org.netxms.ui.eclipse.tools.WidgetHelper;

public class TimePeriodSelector extends Composite
{

   private Button radioBackFromNow;
   private Button radioFixedInterval;
   private Spinner timeRange;
   private Combo timeUnits;
   private DateTimeSelector timeFrom;
   private DateTimeSelector timeTo;
   
   public TimePeriodSelector(Composite parent, int style, int timeFrameType, int timeRangeValue, int timeUnitValue, Date timeFromValue, Date timeToValue)
   {
      super(parent, style);
      
      setLayout(new FillLayout());
      
      Group timeGroup = new Group(parent, SWT.NONE);
      timeGroup.setText("Time Period");
      GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.OUTER_SPACING;
      layout.marginHeight = WidgetHelper.OUTER_SPACING;
      layout.horizontalSpacing = 16;
      layout.makeColumnsEqualWidth = true;
      layout.numColumns = 2;
      timeGroup.setLayout(layout);
      
      final SelectionListener listener = new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            timeRange.setEnabled(radioBackFromNow.getSelection());
            timeUnits.setEnabled(radioBackFromNow.getSelection());
            timeFrom.setEnabled(radioFixedInterval.getSelection());
            timeTo.setEnabled(radioFixedInterval.getSelection());
         }
         
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      };

      radioBackFromNow = new Button(timeGroup, SWT.RADIO);
      radioBackFromNow.setText("Back from now");
      radioBackFromNow.setSelection(timeFrameType == GraphSettings.TIME_FRAME_BACK_FROM_NOW);
      radioBackFromNow.addSelectionListener(listener);
      
      radioFixedInterval = new Button(timeGroup, SWT.RADIO);
      radioFixedInterval.setText("Fixed time frame");
      radioFixedInterval.setSelection(timeFrameType == GraphSettings.TIME_FRAME_FIXED);
      radioFixedInterval.addSelectionListener(listener);
      
      Composite timeBackGroup = new Composite(timeGroup, SWT.NONE);
      layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
      layout.numColumns = 2;
      timeBackGroup.setLayout(layout);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.TOP;
      timeBackGroup.setLayoutData(gd);
      
      timeRange = WidgetHelper.createLabeledSpinner(timeBackGroup, SWT.BORDER, "Time interval", 1, 10000, WidgetHelper.DEFAULT_LAYOUT_DATA);
      timeRange.setSelection(timeRangeValue);
      timeRange.setEnabled(radioBackFromNow.getSelection());
      
      timeUnits = WidgetHelper.createLabeledCombo(timeBackGroup, SWT.READ_ONLY, "Time units", WidgetHelper.DEFAULT_LAYOUT_DATA);
      timeUnits.add("Minutes");
      timeUnits.add("Hours");
      timeUnits.add("Days");
      timeUnits.select(timeUnitValue);
      timeUnits.setEnabled(radioBackFromNow.getSelection());

      Composite timeFixedGroup = new Composite(timeGroup, SWT.NONE);
      layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      timeFixedGroup.setLayout(layout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.TOP;
      timeFixedGroup.setLayoutData(gd);
      
      final WidgetFactory factory = new WidgetFactory() {
         @Override
         public Control createControl(Composite parent, int style)
         {
            return new DateTimeSelector(parent, style);
         }
      };
      
      timeFrom = (DateTimeSelector)WidgetHelper.createLabeledControl(timeFixedGroup, SWT.NONE, factory, "Time from", WidgetHelper.DEFAULT_LAYOUT_DATA);
      timeFrom.setValue(timeFromValue);
      timeFrom.setEnabled(radioFixedInterval.getSelection());

      timeTo = (DateTimeSelector)WidgetHelper.createLabeledControl(timeFixedGroup, SWT.NONE, factory, "Time to", WidgetHelper.DEFAULT_LAYOUT_DATA);
      timeTo.setValue(timeToValue);
      timeTo.setEnabled(radioFixedInterval.getSelection());
   }
   
   public int getTimeFrameType()
   {
      return radioBackFromNow.getSelection() ? GraphSettings.TIME_FRAME_BACK_FROM_NOW : GraphSettings.TIME_FRAME_FIXED;
   }

   public int getTimeRangeValue()
   {
      return timeRange.getSelection();
   }
   
   public int getTimeUnitValue()
   {
      return timeUnits.getSelectionIndex();
   }
   
   public long getTimeInMinutes()
   {
      long result;
      if(radioBackFromNow.getSelection())
      {
         switch(timeUnits.getSelectionIndex())
         {
            case 0:
               result = timeRange.getSelection();
               break;
            case 1:
               result = timeRange.getSelection() * 60;
               break;
            case 2:
               result = timeRange.getSelection() * 24 * 60;
               break;
            default:
               result = 0;
         }
      }
      else 
      {
         result = (timeTo.getValue().getTime() - timeFrom.getValue().getTime()) / (60000);
      }
      return result; //TODO: implement
   }
   
   public Date getTimeFrom()
   {
      return timeFrom.getValue();
   }
   
   public Date getTimeTo()
   {
      return timeTo.getValue();
   }
   
   public void setDefaults()
   {
      radioBackFromNow.setSelection(true);
      radioFixedInterval.setSelection(false);
      
      timeRange.setSelection(60);
      timeRange.setEnabled(true);
      timeUnits.select(0);
      timeUnits.setEnabled(true);
      timeFrom.setEnabled(false);
      timeTo.setEnabled(false);
   }
}
