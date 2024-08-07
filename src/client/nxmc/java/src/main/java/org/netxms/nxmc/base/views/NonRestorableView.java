package org.netxms.nxmc.base.views;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.util.Base64;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Text;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;

/**
 * View replacement for non restorable views
 */
public class NonRestorableView extends View
{
   private String viewName;
   private Exception exception;
   
   /**
    * View constructor from exception
    * 
    * @param exception 
    */
   public NonRestorableView(Exception exception, String viewName)
   {
      super(viewName, ResourceManager.getImageDescriptor("icons/invalid-report.png"), viewName);
      this.viewName = viewName;
      this.exception = exception;
   }
   
   /**
    * Restore view constructor
    */
   protected NonRestorableView()
   {
      super(LocalizationHelper.getI18n(NonRestorableView.class).tr("Not Restored"), ResourceManager.getImageDescriptor("icons/invalid-report.png"), null);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      Text text = new Text(parent, SWT.READ_ONLY);
      text.setText(viewName + ": " + exception.getMessage());
   }

   /**
    * @see org.netxms.nxmc.base.views.View#saveState(org.netxms.nxmc.Memento)
    */
   public void saveState(Memento memento)
   {
      super.saveState(memento);
      memento.set("viewName", viewName);
      try
      {
         ByteArrayOutputStream baos = new ByteArrayOutputStream();
         ObjectOutputStream oos = new ObjectOutputStream(baos);
         oos.writeObject(exception);
         oos.close();
         memento.set("exception", Base64.getEncoder().encodeToString(baos.toByteArray()));
      }
      catch(IOException e)
      {
         e.printStackTrace();
      }
   }

   /**
    * @throws ViewNotRestoredException
    * @see org.netxms.nxmc.base.views.ViewWithContext#restoreState(org.netxms.nxmc.Memento)
    */
   @Override
   public void restoreState(Memento memento) throws ViewNotRestoredException
   {
      super.restoreState(memento);
      viewName = memento.getAsString("viewName");
      String e = memento.getAsString("exception", null);
      if (e != null)
      {
         byte[] data = Base64.getDecoder().decode(e);
         try
         {
            ObjectInputStream ois = new ObjectInputStream(new ByteArrayInputStream(data));
            exception = (Exception)ois.readObject();
            ois.close();
         }
         catch(IOException | ClassNotFoundException e1)
         {
            exception = null;
         }
      }
   }
}
