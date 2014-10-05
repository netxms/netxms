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
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.agentmanager.Activator;
import org.netxms.ui.eclipse.console.DownloadServiceHandler;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

public class AgentScreenshotView extends ViewPart
{
   public static final String ID = "org.netxms.ui.eclipse.agentmanager.views.AgentScreenshotView";

   private NXCSession session;
   private long nodeId;
   private Image image;
   private Canvas canvas;
   private RefreshAction actionRefresh;
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
	  String id = Long.toString(System.currentTimeMillis());
      DownloadServiceHandler.addDownload(id, "screenshot.png", byteImage, "application/octet-stream");
      JavaScriptExecutor executor = RWT.getClient().getService(JavaScriptExecutor.class);
      if( executor != null ) 
      {
         StringBuilder js = new StringBuilder();
         js.append("var hiddenIFrameID = 'hiddenDownloader',");
         js.append("   iframe = document.getElementById(hiddenIFrameID);");
         js.append("if (iframe === null) {");
         js.append("   iframe = document.createElement('iframe');");
         js.append("   iframe.id = hiddenIFrameID;");
         js.append("   iframe.style.display = 'none';");
         js.append("   document.body.appendChild(iframe);");
         js.append("}");
         js.append("iframe.src = '");
         js.append(DownloadServiceHandler.createDownloadUrl(id));
         js.append("';");
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
      manager.add(actionSaveImage);  
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
      manager.add(new Separator());
      manager.add(actionRefresh);
   }
}
