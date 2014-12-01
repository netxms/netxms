/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.agentmanager.views;

import java.io.ByteArrayInputStream;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.rap.rwt.client.service.JavaScriptExecutor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.agentmanager.Activator;
import org.netxms.ui.eclipse.agentmanager.Messages;
import org.netxms.ui.eclipse.console.DownloadServiceHandler;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Screenshot view
 */
public class ScreenshotView extends ViewPart
{
   public static final String ID = "org.netxms.ui.eclipse.agentmanager.views.ScreenshotView"; //$NON-NLS-1$

   private NXCSession session;
   private long nodeId;
   private Image image;
   private Canvas canvas;
   private byte[] byteImage;
   private Action actionRefresh;
   private Action actionSave;

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
    */
   @Override
   public void init(IViewSite site) throws PartInitException
   {
      super.init(site);

      session = (NXCSession)ConsoleSharedData.getSession();
      nodeId = Long.parseLong(site.getSecondaryId());

      setPartName(String.format(Messages.get().ScreenshotView_PartTitle, session.getObjectName(nodeId)));
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   {
      parent.setLayout(new FillLayout());
      canvas = new Canvas(parent, SWT.NONE);
      canvas.addPaintListener(new PaintListener() {
         public void paintControl(PaintEvent e)
         {
            if (image != null)
            {
               GC gc = e.gc;
               gc.drawImage(image, 10, 10);
            }
         }
      });
      createActions();
      contributeToActionBars();
      refresh();
   }

   /**
    * Get new screenshot and refresh image
    */
   public void refresh()
   {
      final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      new ConsoleJob(Messages.get().ScreenshotView_JobTitle, this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            byteImage = session.takeScreenshot(nodeId, "Console"); //$NON-NLS-1$
            final ImageData data = new ImageData(new ByteArrayInputStream(byteImage));
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  image = new Image(getDisplay(), data);
                  canvas.redraw();
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return Messages.get().ScreenshotView_JobError;
         }
      }.start();
   }
   
   /**
    * Create actions
    */
   private void createActions()
   {      
      actionRefresh = new RefreshAction()
      {
         @Override
         public void run()
         {
            refresh();
         }
      };
      
      actionSave = new Action(Messages.get().ScreenshotView_Save, SharedIcons.SAVE) {
         @Override
         public void run()
         {
            saveImage();
         }
      }; 
   }
   
   /**
    * Save image to file
    */
   private void saveImage()
   {
      String id = Long.toString(System.currentTimeMillis());
      DownloadServiceHandler.addDownload(id, session.getObjectName(nodeId) + "-screenshot.png", byteImage, "application/octet-stream"); //$NON-NLS-1$ //$NON-NLS-2$
      JavaScriptExecutor executor = RWT.getClient().getService(JavaScriptExecutor.class);
      if( executor != null ) 
      {
         StringBuilder js = new StringBuilder();
         js.append("var hiddenIFrameID = 'hiddenDownloader',"); //$NON-NLS-1$
         js.append("   iframe = document.getElementById(hiddenIFrameID);"); //$NON-NLS-1$
         js.append("if (iframe === null) {"); //$NON-NLS-1$
         js.append("   iframe = document.createElement('iframe');"); //$NON-NLS-1$
         js.append("   iframe.id = hiddenIFrameID;"); //$NON-NLS-1$
         js.append("   iframe.style.display = 'none';"); //$NON-NLS-1$
         js.append("   document.body.appendChild(iframe);"); //$NON-NLS-1$
         js.append("}"); //$NON-NLS-1$
         js.append("iframe.src = '"); //$NON-NLS-1$
         js.append(DownloadServiceHandler.createDownloadUrl(id));
         js.append("';"); //$NON-NLS-1$
         executor.execute(js.toString());
      }
   }

   /*
    * (non-Javadoc)
    * 
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
      canvas.setFocus();
   }
   
   /**
    * Contribute actions to action bar
    */
   private void contributeToActionBars()
   {
      IActionBars bars = getViewSite().getActionBars();
      fillLocalPullDown(bars.getMenuManager());
      fillLocalToolBar(bars.getToolBarManager());
   }

   /**
    * Fill local pull-down menu
    * 
    * @param manager
    *           Menu manager for pull-down menu
    */
   private void fillLocalPullDown(IMenuManager manager)
   {
      manager.add(actionSave);
      manager.add(new Separator());
      manager.add(actionRefresh);
   }

   /**
    * Fill local tool bar
    * 
    * @param manager
    *           Menu manager for local toolbar
    */
   private void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionSave);
      manager.add(new Separator());
      manager.add(actionRefresh);
   }
}
