/**
 * 
 */
package org.netxms.webui.mobile.dialogs;

import java.lang.reflect.InvocationTargetException;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.operation.IRunnableWithProgress;
import org.eclipse.rap.rwt.service.ServerPushSession;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.ProgressBar;
import org.eclipse.swt.widgets.Shell;

/**
 * Progress dialog
 */
public class ProgressDialog extends Dialog
{
   private Label task;
   private ProgressBar progressBar;
   private IRunnableWithProgress runnable;
   private InvocationTargetException exception = null;
   
   /**
    * @param parentShell
    */
   public ProgressDialog(Shell parentShell)
   {
      super(parentShell);
   }

   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText("Connecting");
      newShell.setAlpha(64);
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Composite content = new Composite(parent, SWT.NONE);
      applyDialogFont(content);
      content.setLayout(new GridLayout());
      
      task = new Label(content, SWT.NONE);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      task.setLayoutData(gd);
      
      progressBar = new ProgressBar(content, SWT.HORIZONTAL | SWT.READ_ONLY);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.heightHint = 15;
      progressBar.setLayoutData(gd);
      
      startWorkerThread();
      return content;
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
                     display.asyncExec(new Runnable() {
                        @Override
                        public void run()
                        {
                           progressBar.setSelection(progressBar.getSelection() + work);
                        }
                     });
                     display.wake();
                     try
                     {
                        Thread.sleep(1000);
                     }
                     catch(InterruptedException e)
                     {
                        // TODO Auto-generated catch block
                        e.printStackTrace();
                     }
                  }
                  
                  @Override
                  public void subTask(String name)
                  {
                  }
                  
                  @Override
                  public void setTaskName(final String name)
                  {
                     display.asyncExec(new Runnable() {
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
                     display.asyncExec(new Runnable() {
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
            display.asyncExec(new Runnable() {
               @Override
               public void run()
               {
                  close();
               }
            });
            display.asyncExec(null);
            pushSession.stop();
         }
      });
      pushSession.start();
      workerThread.start();
   }

   /**
    * @param runnable
    * @throws InvocationTargetException
    * @throws InterruptedException
    */
   public void run(final IRunnableWithProgress runnable) throws InvocationTargetException
   {
      this.runnable = runnable;
      setBlockOnOpen(true);
      open();
      
      if (exception != null)
         throw exception;
   }
}
