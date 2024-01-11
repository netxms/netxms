/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Reden Solutions
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
package org.netxms.nxmc.modules.filemanager;

import java.io.File;
import java.io.IOException;
import org.eclipse.core.runtime.IProgressMonitor;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.ProgressListener;
import org.netxms.client.server.AgentFile;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTreeViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.filemanager.views.AgentFileManager;
import org.netxms.nxmc.modules.filemanager.views.helpers.NestedVerifyOverwrite;
import org.xnap.commons.i18n.I18n;

class UploadConsoleJob extends Job
{
   private final I18n i18n = LocalizationHelper.getI18n(UploadConsoleJob.class);
   
   private boolean askFolderOverwrite;
   private boolean askFileOverwrite;
   private boolean overwrite;
   private File folder;
   private AgentFile uploadFolder;
   private String remoteFileName;
   private long objectId;
   private NXCSession session;
   private SortableTreeViewer viewer;

   public UploadConsoleJob(String name, File folder, long objectId, final AgentFile uploadFolder, final String remoteFileName, AgentFileManager view, final SortableTreeViewer viewer)
   {
      super(name, view);
      askFolderOverwrite = true;
      askFileOverwrite = true;
      overwrite = false;
      this.folder = folder;
      this.uploadFolder = uploadFolder;
      this.remoteFileName = remoteFileName;
      session = Registry.getSession();
      this.viewer = viewer;
      this.objectId = objectId;
   }

   @Override
   protected void run(IProgressMonitor monitor) throws Exception
   {
      NestedVerifyOverwrite verify = new NestedVerifyOverwrite(getView().getWindow().getShell(), AgentFile.DIRECTORY, folder.getName(), askFolderOverwrite, askFileOverwrite, overwrite) {
         
         @Override
         public void executeAction() throws NXCException, IOException
         {                        
            session.createFolderOnAgent(objectId, uploadFolder.getFullName()+"/"+remoteFileName); 
         }

         @Override
         public void executeSameFunctionWithOverwrite() throws NXCException, IOException
         {
            //do nothing
         }
      };
      verify.run(viewer.getControl().getDisplay());
      askFolderOverwrite = verify.askFolderOverwrite();
      
      uploadFilesInFolder(folder, uploadFolder.getFullName()+"/"+remoteFileName, monitor); 
      
      uploadFolder.setChildren(session.listAgentFiles(uploadFolder, uploadFolder.getFullName(), objectId));
      runInUIThread(new Runnable() {
         @Override
         public void run()
         {
            viewer.refresh(uploadFolder, true);
         }
      });
   }

   @Override
   protected String getErrorMessage()
   {
      return i18n.tr("Cannot upload file to server");
   }
   
   /**
    * Recursively uploads files to agent and creates correct folders
    * 
    * @param folder
    * @param upladFolder
    * @param monitor
    * @throws NXCException
    * @throws IOException
    */
   public void uploadFilesInFolder(final File folder, final String uploadFolder, final IProgressMonitor monitor) throws NXCException, IOException 
   {
      for(final File fileEntry : folder.listFiles())
      {
          if (fileEntry.isDirectory()) 
          {
             NestedVerifyOverwrite verify = new NestedVerifyOverwrite(getView().getWindow().getShell(), AgentFile.DIRECTORY, fileEntry.getName(), askFolderOverwrite, askFileOverwrite, overwrite) {
                
                @Override
                public void executeAction() throws NXCException, IOException
                {                        
                   session.createFolderOnAgent(objectId, uploadFolder + "/" + fileEntry.getName()); 
                }

                @Override
                public void executeSameFunctionWithOverwrite() throws NXCException, IOException
                {
                   //do nothing
                }
             };
             verify.run(viewer.getControl().getDisplay());
             askFolderOverwrite = verify.askFolderOverwrite();
             
             uploadFilesInFolder(fileEntry, uploadFolder + "/" + fileEntry.getName(), monitor); 
          } 
          else 
          {      
             NestedVerifyOverwrite verify = new NestedVerifyOverwrite(getView().getWindow().getShell(), AgentFile.FILE, fileEntry.getName(), askFolderOverwrite, askFileOverwrite, overwrite) {
                
                @Override
                public void executeAction() throws NXCException, IOException
                {                        
                   session.uploadLocalFileToAgent(objectId, fileEntry, uploadFolder + "/" + fileEntry.getName(), overwrite, new ProgressListener() { 
                      private long prevWorkDone = 0;

                      @Override
                      public void setTotalWorkAmount(long workTotal)
                      {
                         monitor.beginTask(i18n.tr("Upload file ") + fileEntry.getAbsolutePath(),
                               (int)workTotal);
                      }

                      @Override
                      public void markProgress(long workDone)
                      {
                         monitor.worked((int)(workDone - prevWorkDone));
                         prevWorkDone = workDone;
                      }
                   });
                   monitor.done();
                }

                @Override
                public void executeSameFunctionWithOverwrite() throws NXCException, IOException
                {
                   session.uploadLocalFileToAgent(objectId, fileEntry, uploadFolder + "/" + fileEntry.getName(), true, new ProgressListener() { 
                   private long prevWorkDone = 0;

                   @Override
                   public void setTotalWorkAmount(long workTotal)
                   {
                      monitor.beginTask(i18n.tr("Upload file ") + fileEntry.getAbsolutePath(),
                            (int)workTotal);
                   }

                   @Override
                   public void markProgress(long workDone)
                   {
                      monitor.worked((int)(workDone - prevWorkDone));
                      prevWorkDone = workDone;
                   }
                });
                monitor.done(); 
                }
             };
             verify.run(viewer.getControl().getDisplay());
             askFileOverwrite = verify.askFileOverwrite();
             if(!askFileOverwrite)
                overwrite = verify.isOkPressed();
          }
      }
  }
}
