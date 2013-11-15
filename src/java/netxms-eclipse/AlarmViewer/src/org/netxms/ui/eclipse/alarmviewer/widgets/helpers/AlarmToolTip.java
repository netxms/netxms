/**
 * 
 */
package org.netxms.ui.eclipse.alarmviewer.widgets.helpers;

import org.eclipse.jface.window.ToolTip;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.NXCSession;
import org.netxms.client.events.Alarm;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.alarmviewer.Activator;
import org.netxms.ui.eclipse.alarmviewer.Messages;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.SharedIcons;

/**
 * Alarm tooltip
 */
public class AlarmToolTip extends ToolTip
{
   private static final String[] stateImage = { "icons/outstanding.png", "icons/acknowledged.png", "icons/resolved.png", "icons/terminated.png", "icons/acknowledged_sticky.png" }; //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$ //$NON-NLS-4$ //$NON-NLS-5$
   private static final String[] stateText = { Messages.get().AlarmListLabelProvider_AlarmState_Outstanding, Messages.get().AlarmListLabelProvider_AlarmState_Acknowledged, Messages.get().AlarmListLabelProvider_AlarmState_Resolved, Messages.get().AlarmListLabelProvider_AlarmState_Terminated };
   
   private Alarm alarm;
   private WorkbenchLabelProvider wbLabelProvider;

   /**
    * @param control
    * @param alarm
    */
   public AlarmToolTip(Control control, Alarm alarm)
   {
      super(control, NO_RECREATE, true);
      this.alarm = alarm;
      wbLabelProvider = new WorkbenchLabelProvider();
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.window.ToolTip#createToolTipContentArea(org.eclipse.swt.widgets.Event, org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Composite createToolTipContentArea(Event event, Composite parent)
   {
      final Composite content = new Composite(parent, SWT.NONE);
      
      GridLayout layout = new GridLayout();
      layout.numColumns = 3;
      content.setLayout(layout);
      
      CLabel alarmSeverity = new CLabel(content, SWT.NONE);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.TOP;
      alarmSeverity.setLayoutData(gd);
      
      Label sep = new Label(content, SWT.VERTICAL | SWT.SEPARATOR);
      gd = new GridData();
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.verticalSpan = 3;
      sep.setLayoutData(gd);
      
      Text alarmText = new Text(content, SWT.MULTI);
      alarmText.setEditable(false);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.verticalSpan = 3;
      alarmText.setLayoutData(gd);

      CLabel alarmState = new CLabel(content, SWT.NONE);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.TOP;
      alarmState.setLayoutData(gd);
      
      CLabel alarmSource = new CLabel(content, SWT.NONE);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.TOP;
      alarmSource.setLayoutData(gd);
      
      alarmSeverity.setImage(StatusDisplayInfo.getStatusImage(alarm.getCurrentSeverity()));
      alarmSeverity.setText(StatusDisplayInfo.getStatusText(alarm.getCurrentSeverity()));
      
      int state = alarm.getState();
      if ((state == Alarm.STATE_ACKNOWLEDGED) && alarm.isSticky())
         state = Alarm.STATE_TERMINATED + 1;
      final Image image = Activator.getImageDescriptor(stateImage[state]).createImage();
      alarmState.setImage(image);
      alarmState.setText(stateText[alarm.getState()]);
      
      AbstractObject object = ((NXCSession)ConsoleSharedData.getSession()).findObjectById(alarm.getSourceObjectId());
      alarmSource.setImage((object != null) ? wbLabelProvider.getImage(object) : SharedIcons.IMG_UNKNOWN_OBJECT);
      alarmSource.setText((object != null) ? object.getObjectName() : ("[" + Long.toString(alarm.getSourceObjectId()) + "]")); //$NON-NLS-1$ //$NON-NLS-2$
      
      alarmText.setText(alarm.getMessage());
      
      content.addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            wbLabelProvider.dispose();
            image.dispose();
         }
      });

      return content;
   }
}
