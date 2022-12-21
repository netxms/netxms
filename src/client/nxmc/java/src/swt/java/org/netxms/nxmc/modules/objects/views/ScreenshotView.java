/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.views;

import java.awt.image.BufferedImage;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;
import javax.imageio.ImageIO;
import org.apache.commons.lang3.SystemUtils;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.dnd.Clipboard;
import org.eclipse.swt.dnd.ImageTransfer;
import org.eclipse.swt.dnd.Transfer;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.FileDialog;
import org.netxms.client.NXCSession;
import org.netxms.client.Table;
import org.netxms.client.objects.AbstractNode;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.PngTransfer;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Screenshot view
 */
public class ScreenshotView extends AdHocObjectView
{
   private static final I18n i18n = LocalizationHelper.getI18n(ScreenshotView.class);

   private NXCSession session;
   private String userSession;
   private String userName;
   private Image image;
   private String errorMessage;
   private String imageInfo;
   private ScrolledComposite scroller;
   private Canvas canvas;
   private byte[] byteImage;
   private Action actionAutoRefresh;
   private Action actionSave;
   private Action actionCopyToClipboard;
   private boolean autoRefresh = false;
   private long lastRequestTime;

   /**
    * Create screenshot view.
    *
    * @param node node to get screenshot from
    * @param userSession name of user session to get screenshot from or null to select first available
    * @param userName user name to display (can be null)
    */
   public ScreenshotView(AbstractNode node, String userSession, String userName)
   {
      super(i18n.tr("Screenshot"), ResourceManager.getImageDescriptor("icons/screenshot.png"), "Screenshot", node.getObjectId(), false);
      this.userSession = userSession;
      this.userName = userName;
      session = Registry.getSession();
   }

   /**
    * Create agent configuration editor for given node.
    *
    * @param node node object
    */
   protected ScreenshotView()
   {
      super(null, null, null, 0, false);
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#cloneView()
    */
   @Override
   public View cloneView()
   {
      ScreenshotView view = (ScreenshotView)super.cloneView();
      userSession = view.userSession;
      userName = view.userName;
      image = view.image;
      errorMessage = view.errorMessage;
      imageInfo = view.imageInfo;
      lastRequestTime = view.lastRequestTime;
      return view;
   }   

   /**
    * Post clone action
    */
   protected void postClone(View origin)
   {      
      canvas.redraw();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createContent(Composite parent)
   {
      parent.setLayout(new FillLayout());
      
      scroller = new ScrolledComposite(parent, SWT.H_SCROLL | SWT.V_SCROLL);
      
      canvas = new Canvas(scroller, SWT.NONE);
      canvas.addPaintListener(new PaintListener() {
         public void paintControl(PaintEvent e)
         {
            GC gc = e.gc;
            if (image != null)
            {
               gc.drawImage(image, 0, 0);
               gc.drawText(imageInfo, 10, image.getImageData().height + 10);
            }
            else if (errorMessage != null)
            {
               gc.setForeground(canvas.getDisplay().getSystemColor(SWT.COLOR_DARK_RED));
               gc.setFont(JFaceResources.getBannerFont());
               gc.drawText(errorMessage, 10, 10, true);
            }
         }
      });
      
      scroller.setContent(canvas);
      scroller.setExpandVertical(true);
      scroller.setExpandHorizontal(true);
      WidgetHelper.setScrollBarIncrement(scroller, SWT.VERTICAL, 20);
      WidgetHelper.setScrollBarIncrement(scroller, SWT.HORIZONTAL, 20);
      scroller.addControlListener(new ControlAdapter() {
         public void controlResized(ControlEvent e)
         {
            updateScrollerSize();
         }
      });
      
      createActions();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#activate()
    */
   @Override
   public void activate()
   {
      super.activate();
      refresh();
   }

   /**
    * Update scroller size
    */
   private void updateScrollerSize()
   {
      if (image != null)
      {
         ImageData d = image.getImageData();
         Point pt = WidgetHelper.getTextExtent(canvas, imageInfo);
         scroller.setMinSize(new Point(Math.max(d.width, pt.x + 20), d.height + pt.y + 15));
      }
      else if (errorMessage != null)
      {
         Point pt = WidgetHelper.getTextExtent(canvas, errorMessage);
         scroller.setMinSize(new Point(pt.x + 20, pt.y + 20));
      }
      else
      {
         scroller.setMinSize(new Point(0, 0));
      }
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      final long nodeId = getObjectId();
      Job job = new Job(i18n.tr("Taking screenshot"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            try
            {
               if (userSession == null)
               {
                  Table sessions = session.queryAgentTable(nodeId, "Agent.SessionAgents");
                  if ((sessions != null) && (sessions.getRowCount() > 0))
                  {
                     int colIndexName = sessions.getColumnIndex("SESSION_NAME");
                     int colIndexUser = sessions.getColumnIndex("USER_NAME");
                     for(int i = 0; i < sessions.getRowCount(); i++)
                     {
                        String n = sessions.getCellValue(i, colIndexName);
                        if ("Console".equalsIgnoreCase(n))
                        {
                           userSession = n;
                           userName = sessions.getCellValue(i, colIndexUser);
                           break;
                        }
                     }

                     if (userSession == null)
                     {
                        // Console session not found, use first available
                        userSession = sessions.getCellValue(0, colIndexName);
                        userName = sessions.getCellValue(0, colIndexUser);
                     }
                  }
               }
               
               if (userSession == null)
               {
                  // Cannot find any connected sessions
                  runInUIThread(new Runnable() {
                     @Override
                     public void run()
                     {
                        if (canvas.isDisposed())
                           return;

                        if (image != null)
                           image.dispose();
                        
                        image = null;
                        errorMessage = i18n.tr("ERROR (No active sessions or session agent is not running)");
                        canvas.redraw();

                        actionCopyToClipboard.setEnabled(false);
                        actionSave.setEnabled(false);
                        
                        updateScrollerSize();
                     }
                  });
                  return;
               }

               lastRequestTime = System.currentTimeMillis();
               byteImage = session.takeScreenshot(nodeId, userSession);
               final ImageData data = new ImageData(new ByteArrayInputStream(byteImage));
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     if (canvas.isDisposed())
                        return;

                     if (image != null)
                        image.dispose();
                     
                     image = new Image(getDisplay(), data);
                     imageInfo = userName + "@" + userSession;
                     errorMessage = null;
                     canvas.redraw();

                     actionCopyToClipboard.setEnabled(true);
                     actionSave.setEnabled(true);

                     updateScrollerSize();
                  }
               });
            }
            catch(Exception e)
            {
               byteImage = null;
               final String emsg = e.getLocalizedMessage();
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     if (canvas.isDisposed())
                        return;

                     if (image != null)
                        image.dispose();
                     
                     image = null;
                     errorMessage = (emsg != null) ? String.format(i18n.tr("ERROR (%s)"), emsg) : i18n.tr("ERROR");
                     canvas.redraw();

                     actionCopyToClipboard.setEnabled(false);
                     actionSave.setEnabled(false);

                     updateScrollerSize();
                  }
               });
            }
            if (autoRefresh)
            {
               long diff = System.currentTimeMillis() - lastRequestTime;
               if (diff < 250)
                  Thread.sleep(250 - diff);               
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {                     
                     if (autoRefresh && !canvas.isDisposed() && isActive())
                     {
                        refresh();
                     }
                  }
               });
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot take screenshot");
         }
      };
      job.setUser(false);
      job.start();
   }
   
   /**
    * Create actions
    */
   private void createActions()
   {      
      actionAutoRefresh = new Action("Auto refresh", Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            autoRefresh = actionAutoRefresh.isChecked();
            if (autoRefresh)
            {
               refresh();
            }
         }
      };
      actionAutoRefresh.setChecked(autoRefresh);

      actionCopyToClipboard = new Action(i18n.tr("&Copy to clipboard"), SharedIcons.COPY) {
         @Override
         public void run()
         {
            if (image == null)
               return;
            Transfer imageTransfer = SystemUtils.IS_OS_LINUX ? PngTransfer.getInstance() : ImageTransfer.getInstance();
            final Clipboard clipboard = new Clipboard(canvas.getDisplay());
            clipboard.setContents(new Object[] { image.getImageData() }, new Transfer[] { imageTransfer });
         }
      };
      actionCopyToClipboard.setEnabled(false);
      addKeyBinding("M1+C", actionCopyToClipboard);

      actionSave = new Action(i18n.tr("&Save..."), SharedIcons.SAVE) {
         @Override
         public void run()
         {
            saveImage();
         }
      };
      actionSave.setEnabled(false);
      addKeyBinding("M1+S", actionSave);
   }

   /**
    * Save image to file
    */
   private void saveImage()
   {
      if (byteImage == null)
         return;

      FileDialog fd = new FileDialog(getWindow().getShell(), SWT.SAVE);
      fd.setText(i18n.tr("Save Screenshot"));
      fd.setFilterExtensions(new String[] { "*.png", "*.*" });
      fd.setFilterNames(new String[] { i18n.tr("PNG files"), i18n.tr("All files") });
      fd.setFileName(getObject().getObjectName() + "-screenshot-" + new SimpleDateFormat("yyyyMMdd-HHmmss").format(new Date()) + ".png");
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
         addMessage(MessageArea.ERROR, String.format(i18n.tr("Cannot create file %s (%s)"), name, e.getLocalizedMessage()));
         return;
      }

      BufferedImage bi;
      try
      {
         bi = ImageIO.read(new ByteArrayInputStream(byteImage));
         ImageIO.write(bi, "png", outputFile);
      }
      catch(IOException e)
      {
         addMessage(MessageArea.ERROR, String.format(i18n.tr("Cannot save image to %s (%s)"), name, e.getLocalizedMessage()));
      }
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
      canvas.setFocus();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionSave);
      manager.add(actionCopyToClipboard);      
      manager.add(new Separator());
      manager.add(actionAutoRefresh);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionSave);
      manager.add(actionCopyToClipboard);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      if (image != null)
         image.dispose();
      super.dispose();
   }
}
