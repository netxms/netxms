/**
 * 
 */
package org.netxms.ui.eclipse.widgets;

import java.text.DateFormat;
import java.util.Date;
import java.util.TimeZone;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.FontData;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.console.Activator;
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
   private boolean showText;
   private boolean showTimeZone;
   
   /**
    * @param parent
    * @param style
    */
   public ServerClock(Composite parent, int style)
   {
      super(parent, style);
    
      setLayout(new FillLayout());
      
      time = new Label(this, SWT.NONE);
      time.setToolTipText(Messages.get().ServerClock_Tooltip);

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
      
      createPopupMenu();
      
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
    * Create popup menu
    */
   private void createPopupMenu()
   {
      final IDialogSettings settings = Activator.getDefault().getDialogSettings();
      
      showText = settings.getBoolean("ServerClock.showText"); //$NON-NLS-1$
      showTimeZone = settings.getBoolean("ServerClock.showTimeZone"); //$NON-NLS-1$
      
      final Action actionShowText = new Action(Messages.get().ServerClock_OptionShowText, Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            showText = isChecked();
            settings.put("ServerClock.showText", showText); //$NON-NLS-1$
            updateTimeDisplay();
         }
      };
      actionShowText.setChecked(showText);
      
      final Action actionShowTimeZone = new Action(Messages.get().ServerClock_OptionShowTimeZone, Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            showTimeZone = isChecked();
            settings.put("ServerClock.showTimeZone", showTimeZone); //$NON-NLS-1$
            updateTimeDisplay();
         }
      };
      actionShowTimeZone.setChecked(showTimeZone);
      
      final MenuManager menuManager = new MenuManager();
      menuManager.add(actionShowText);
      menuManager.add(actionShowTimeZone);
      setMenu(menuManager.createContextMenu(this));
      time.setMenu(menuManager.createContextMenu(time));
   }

   /**
    * Update time displayed
    */
   private void updateTimeDisplay()
   {
      DateFormat df = RegionalSettings.getShortTimeFormat();
      NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      String tz = session.getServerTimeZone();
      df.setTimeZone(TimeZone.getTimeZone(tz.replaceAll("[A-Za-z]+([\\+\\-][0-9]+).*", "GMT$1"))); //$NON-NLS-1$ //$NON-NLS-2$
      StringBuilder sb = new StringBuilder();
      if (showText)
      {
         sb.append(Messages.get().ServerClock_ServerTime);
         sb.append(' ');
      }
      sb.append(df.format(new Date(session.getServerTime())));
      if (showTimeZone)
      {
         sb.append(' ');
         sb.append(tz);
      }
      time.setText(sb.toString());
      getParent().layout(true, true);
   }
}
