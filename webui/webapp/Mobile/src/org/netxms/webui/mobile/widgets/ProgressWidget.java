/**
 * 
 */
package org.netxms.webui.mobile.widgets;

import java.lang.reflect.InvocationTargetException;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.operation.IRunnableWithProgress;
import org.eclipse.rap.rwt.service.ServerPushSession;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.ProgressBar;

/**
 * @author Victor
 *
 */
public class ProgressWidget extends Composite
{
   private Label task;
   private ProgressBar progressBar;
   private IRunnableWithProgress runnable;
   private InvocationTargetException exception = null;
   private boolean runEventLoop = true;

   public ProgressWidget(Composite parent, int style)
   {
      super(parent, style);
      
      setLayout(new GridLayout());

      task = new Label(this, SWT.NONE);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      task.setLayoutData(gd);
      
      progressBar = new ProgressBar(this, SWT.HORIZONTAL | SWT.READ_ONLY);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.heightHint = 15;
      progressBar.setLayoutData(gd);
   }

   /**
    * 
    */
   private void startWorkerThread()
   {
      final Display display = getShell().getDisplay();
      final ServerPushSession pushSession = new ServerPushSession();
      final Thread workerThread = new Thread(new Runnable() {
         @Override
         public void run()
         {
            try
            {
               runnable.run(new IProgressMonitor() {        
                  @Override
                  public void worked(final int work)
                  {
                     display.syncExec(new Runnable() {
                        @Override
                        public void run()
                        {
                           progressBar.setSelection(progressBar.getSelection() + work);
                        }
                     });
                  }
                  
                  @Override
                  public void subTask(String name)
                  {
                  }
                  
                  @Override
                  public void setTaskName(final String name)
                  {
                     display.syncExec(new Runnable() {
                        @Override
                        public void run()
                        {
                           task.setText(name);
                        }
                     });
                  }
                  
                  @Override
                  public void setCanceled(boolean value)
                  {
                  }
                  
                  @Override
                  public boolean isCanceled()
                  {
                     return false;
                  }
                  
                  @Override
                  public void internalWorked(double work)
                  {
                  }
                  
                  @Override
                  public void done()
                  {
                  }
                  
                  @Override
                  public void beginTask(final String name, final int totalWork)
                  {
                     display.syncExec(new Runnable() {
                        @Override
                        public void run()
                        {
                           progressBar.setMinimum(0);
                           progressBar.setMaximum(totalWork);
                           progressBar.setSelection(0);
                           task.setText(name);
                        }
                     });
                  }
               });
            }
            catch(Exception e)
            {
               exception = new InvocationTargetException(e);
            }
            runEventLoop = false;
            display.syncExec(null);
         }
      });
      pushSession.start();
      workerThread.start();
      while(runEventLoop)
      {
         if (!display.readAndDispatch())
            display.sleep();
      }
      pushSession.stop();
   }

   /**
    * @param runnable
    * @throws InvocationTargetException
    * @throws InterruptedException
    */
   public void run(final IRunnableWithProgress runnable) throws InvocationTargetException
   {
      this.runnable = runnable;
      startWorkerThread();
      if (exception != null)
         throw exception;
   }
}
