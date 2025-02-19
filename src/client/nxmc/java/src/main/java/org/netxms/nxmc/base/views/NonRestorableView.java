/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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
package org.netxms.nxmc.base.views;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.util.Base64;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Text;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * View replacement for non restorable views
 */
public class NonRestorableView extends View
{
   private static final Logger logger = LoggerFactory.getLogger(NonRestorableView.class);

   private final I18n i18n = LocalizationHelper.getI18n(NonRestorableView.class);

   private String viewName;
   private Exception exception;
   private Action actionShowDetails;

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
      final Text text = new Text(parent, SWT.READ_ONLY | SWT.MULTI | SWT.V_SCROLL);
      text.setFont(JFaceResources.getTextFont());
      text.setText((exception != null) ? exception.getLocalizedMessage() : i18n.tr("View cannot be restored (internal error)"));

      actionShowDetails = new Action(i18n.tr("&Show details"), ResourceManager.getImageDescriptor("icons/expand.png")) {
         @Override
         public void run()
         {
            if (exception != null)
            {
               StringBuilder sb = new StringBuilder(exception.getLocalizedMessage());
               sb.append("\n----------------------------------------------------------------------------------------\n");
               appendExceptionTrace(sb, exception);
               text.setText(sb.toString());
            }
         }
      };
   }

   /**
    * Append exception trace to string builder
    *
    * @param sb string builder
    * @param e exception
    */
   private static void appendExceptionTrace(StringBuilder sb, Throwable e)
   {
      sb.append(e.getClass().getName());
      if ((e.getMessage() != null) && !e.getMessage().isBlank())
      {
         sb.append(": ");
         sb.append(e.getMessage());
      }
      sb.append('\n');
      for(StackTraceElement s : e.getStackTrace())
      {
         sb.append("\tat ");
         sb.append(s.toString());
         sb.append('\n');
      }
      if (e.getCause() != null)
      {
         sb.append("Caused by: ");
         appendExceptionTrace(sb, e.getCause());
      }
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionShowDetails);
      super.fillLocalToolBar(manager);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionShowDetails);
      super.fillLocalMenu(manager);
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
         logger.error("Unexpected serialization error", e);
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
         try (ObjectInputStream ois = new ObjectInputStream(new ByteArrayInputStream(data)))
         {
            exception = (Exception)ois.readObject();
         }
         catch(IOException | ClassNotFoundException e1)
         {
            logger.error("Unexpected deserialization error", e);
         }
      }
   }
}
