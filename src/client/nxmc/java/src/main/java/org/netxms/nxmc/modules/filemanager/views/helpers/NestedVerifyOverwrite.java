/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Reden Solutions
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
package org.netxms.nxmc.modules.filemanager.views.helpers;

import java.io.IOException;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCException;
import org.netxms.client.constants.RCC;
import org.netxms.client.server.AgentFile;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.DialogData;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Nested class that check if file already exist and it should be overwritten
 */
public abstract class NestedVerifyOverwrite
{
   private I18n i18n;

   private boolean okPresseed;
   private int type;
   private String name;
   private boolean askFolderOverwrite;
   private boolean askFileOverwrite;
   private boolean overwrite;
   private Shell shell;

   public NestedVerifyOverwrite(Shell shell, int fileType, String fileName, boolean askFolderOverwrite, boolean askFileOverwrite, boolean overwrite)
   {
      type = fileType;
      name = fileName;
      this.askFolderOverwrite = askFolderOverwrite;
      this.askFileOverwrite = askFileOverwrite;
      this.overwrite = overwrite;
      okPresseed = true;
      this.shell = shell;
      shell.getDisplay().syncExec(() -> {
         i18n = LocalizationHelper.getI18n(NestedVerifyOverwrite.class);
      });
   }

   public boolean askFolderOverwrite()
   {
      return askFolderOverwrite;
   }

   public boolean askFileOverwrite()
   {
      return askFileOverwrite;
   }

   public void run(Display display) throws IOException, NXCException
   {         
      try
      {
         executeAction();
      }
      catch(final NXCException e)
      {
         if ((e.getErrorCode() == RCC.FOLDER_ALREADY_EXIST) || (type == AgentFile.DIRECTORY))
         {
            if (askFolderOverwrite)
            {
               display.syncExec(new Runnable() {
                  @Override
                  public void run()
                  {
                     DialogData data = MessageDialogHelper.openOneButtonWarningWithCheckbox(shell, 
                           i18n.tr("Folder Already Exists"), i18n.tr("Do not show again for this upload"), String.format(i18n.tr("Folder %s already exist"), name));
                     askFolderOverwrite = !data.getSaveSelection();
                     okPresseed = false;
                  }
               });
            }
         }   
         else if ((e.getErrorCode() == RCC.FILE_ALREADY_EXISTS) || (e.getErrorCode() == RCC.FOLDER_ALREADY_EXIST))
         {
            if (askFileOverwrite)
            {
               display.syncExec(new Runnable() {
                  @Override
                  public void run()
                  {
                     DialogData data = MessageDialogHelper.openWarningWithCheckbox(shell,
                           String.format(i18n.tr("%s Overwrite Confirmation"), type == AgentFile.DIRECTORY ? "Folder" : "File"),
                           i18n.tr("Apply this action to all files"),
                           String.format(i18n.tr("%s with name %s already exists. Are you sure you want to overwrite it?"), e.getErrorCode() == RCC.FOLDER_ALREADY_EXIST ? "Folder" : "File", name),
                           new String[] { i18n.tr("&Overwrite"), i18n.tr("&Skip") });
                     askFileOverwrite = !data.getSaveSelection();
                     okPresseed = data.isOkPressed();                           
                  }
               });
               if (okPresseed)
               {
                  executeSameFunctionWithOverwrite();
               }
            }
            else
            {
               if (overwrite)
               {
                  executeSameFunctionWithOverwrite();
               }
            }
         }
         else
         {
            throw e;
         }
      }
   }

   public boolean isOkPressed()
   {
      return okPresseed;
   }

   public abstract void executeAction() throws NXCException, IOException;

   public abstract void executeSameFunctionWithOverwrite() throws NXCException, IOException;
}
