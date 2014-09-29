package org.netxms.ui.eclipse.agentmanager.views;

import java.awt.image.BufferedImage;
import java.io.BufferedInputStream;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
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
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.agentmanager.Activator;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

public class AgentScreenshotView extends ViewPart
{
   public static final String ID = "org.netxms.ui.eclipse.agentmanager.views.AgentScreenshotView";

   private NXCSession session;
   private long nodeId;
   private Image image;
   private Canvas canvas;
   private RefreshAction actionRefresh;
   private Action actionCopyToClipboard;
   private Action actionSaveImage;
   private byte[] byteImage;

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

      AbstractObject object = session.findObjectById(nodeId);
      setPartName("View screenshot " + ((object != null) ? object.getObjectName() : Long.toString(nodeId)));
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
      new ConsoleJob("Get screenshot", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            byteImage = session.takeScreenshot(nodeId, "Console");
            ImageData data = new ImageData(new ByteArrayInputStream(byteImage));
            image = new Image(getDisplay(), data);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  canvas.redraw();
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Screenshot could not be loaded";
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
         /* (non-Javadoc)
          * @see org.eclipse.jface.action.Action#run()
          */
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
      
      actionSaveImage = new Action("Save image", SharedIcons.SAVE) {
         @Override
         public void run()
         {
            saveImage();
         }
      }; 
   }
   
   private void saveImage()
   {
      String name;
      do
      {
         FileDialog fd = new FileDialog(getSite().getShell(), SWT.SAVE);
         fd.setText("Select how to save file");
         String[] filterExtensions = { "*.*" };
         fd.setFilterExtensions(filterExtensions);
         String[] filterNames = { "ALL" };
         fd.setFilterNames(filterNames);
         fd.setFileName("screenshot.png");
         name = fd.open();

         if (name == null)
            return;
      } while(name.isEmpty());
      
      
      File outputFile = new File(name);
      try
      {
         outputFile.createNewFile();
      }
      catch(IOException e)
      {
         MessageDialogHelper.openError(getViewSite().getShell(), "Error creating file",
               String.format("Error creating file %s: %s", name, e.getLocalizedMessage()));
      }
      BufferedImage bi;
      try
      {
         bi = ImageIO.read(new ByteArrayInputStream(byteImage));
         ImageIO.write(bi, "png", outputFile);
      }
      catch(IOException e)
      {

         MessageDialogHelper.openError(getViewSite().getShell(), "Error saving picture",
               String.format("Error saving picture %s: %s", name, e.getLocalizedMessage()));
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
      manager.add(actionSaveImage);
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
      manager.add(actionSaveImage);
      manager.add(actionCopyToClipboard);
      manager.add(new Separator());
      manager.add(actionRefresh);
   }
}
