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

import java.awt.image.BufferedImage;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.IOException;
import javax.imageio.ImageIO;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.dnd.Clipboard;
import org.eclipse.swt.dnd.ImageTransfer;
import org.eclipse.swt.dnd.Transfer;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.agentmanager.Activator;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Screenshot view
 */
public class ScreenshotView extends ViewPart
{
   public static final String ID = "org.netxms.ui.eclipse.agentmanager.views.ScreenshotView";

   private NXCSession session;
   private long nodeId;
   private Image image;
   private Canvas canvas;
   private byte[] byteImage;
   private Action actionRefresh;
   private Action actionSave;
   private Action actionCopyToClipboard;

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

      setPartName(String.format("Screenshot - %s", session.getObjectName(nodeId)));
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
      new ConsoleJob("Take screenshot", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            byteImage = session.takeScreenshot(nodeId, "Console");
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
            return "Cannot take screenshot";
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
      
      actionCopyToClipboard = new Action("Copy to clipboard", SharedIcons.COPY) {
         @Override
         public void run()
         {
            ImageTransfer imageTransfer = ImageTransfer.getInstance();
            final Clipboard clipboard = new Clipboard(canvas.getDisplay());
            clipboard.setContents(new Object[] { image.getImageData() }, new Transfer[] { imageTransfer });
         }
      };
      
      actionSave = new Action("Save...", SharedIcons.SAVE) {
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
      FileDialog fd = new FileDialog(getSite().getShell(), SWT.SAVE);
      fd.setText("Save Screenshot");
      fd.setFilterExtensions(new String[] { "*.png", "*.*" });
      fd.setFilterNames(new String[] { "PNG Files", "All Files" });
      fd.setFileName(session.getObjectName(nodeId) + "-screenshot.png");
      String name = fd.open();
      if (name == null)
         return;
      
      File outputFile = new File(name);
      try
      {
         outputFile.createNewFile();
      }
      catch(IOException e)
      {
         MessageDialogHelper.openError(getViewSite().getShell(), "Error",
               String.format("Cannot create file %s: %s", name, e.getLocalizedMessage()));
      }
      BufferedImage bi;
      try
      {
         bi = ImageIO.read(new ByteArrayInputStream(byteImage));
         ImageIO.write(bi, "png", outputFile);
      }
      catch(IOException e)
      {

         MessageDialogHelper.openError(getViewSite().getShell(), "Error",
               String.format("Cannot save image to %s: %s", name, e.getLocalizedMessage()));
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
      manager.add(actionCopyToClipboard);      
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
      manager.add(actionCopyToClipboard);
      manager.add(new Separator());
      manager.add(actionRefresh);
   }
}
