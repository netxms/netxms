/**
 * 
 */
package org.netxms.ui.eclipse.widgets;

import java.text.DateFormat;
import java.util.Date;
import java.util.TimeZone;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.FontData;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.console.Messages;
import org.netxms.ui.eclipse.console.tools.RegionalSettings;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Widget showing server clock
 */
public class ServerClock extends Composite
{
   private static final int REFRESH_TIME = 30000;
   
   private Label time;
   
   /**
    * @param parent
    * @param style
    */
   public ServerClock(Composite parent, int style)
   {
      super(parent, style);
    
      setLayout(new FillLayout());
      
      time = new Label(this, SWT.NONE);
      time.setToolTipText(Messages.ServerClock_ServerTime);

      FontData fd = time.getFont().getFontData()[0];
      fd.setStyle(SWT.BOLD);
      final Font font = new Font(time.getDisplay(), fd);
      time.setFont(font);
      time.addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            font.dispose();
         }
      });

      final Runnable timer = new Runnable() {
         public void run() {
            if (time.isDisposed())
               return;
            time.getDisplay().timerExec(REFRESH_TIME, this);
            updateTimeDisplay();
         }
      };
      timer.run();
   }

   /**
    * 
    */
   private void updateTimeDisplay()
   {
      DateFormat df = RegionalSettings.getShortTimeFormat();
      NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      String tz = session.getServerTimeZone();
      df.setTimeZone(TimeZone.getTimeZone(tz.replaceAll("[A-Za-z]+([\\+\\-][0-9]+).*", "GMT$1"))); //$NON-NLS-1$ //$NON-NLS-2$
      time.setText(String.format(Messages.ServerClock_TimeFormat, df.format(new Date(session.getServerTime())), tz));
   }
}
