package org.netxms.ui.eclipse.objectview.views;

import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Rack;
import org.netxms.ui.eclipse.objectview.Activator;
import org.netxms.ui.eclipse.objectview.widgets.PhysicalLinkWidget;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

public class PhysicalLinkView extends ViewPart
{
   public static final String ID = "org.netxms.ui.eclipse.objectview.views.PhysicalLinkView"; //$NON-NLS-1$
   
   private PhysicalLinkWidget linkWidget;  
   private long objectId;
   private long patchPanelId;
   private boolean initShowFilter = true;
   private IDialogSettings settings;
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
    */
   @Override
   public void init(IViewSite site) throws PartInitException
   {
      super.init(site);
      String[] params = site.getSecondaryId().split("&");
      objectId = Long.parseLong(params[0]);
      patchPanelId = Long.parseLong(params[1]);    
      
      settings = Activator.getDefault().getDialogSettings();
      initShowFilter = safeCast(settings.get("PhysicalLinkView"), settings.getBoolean("PhysicalLinkView"), initShowFilter);  

      if (objectId != 0)
      {
         NXCSession session = (NXCSession)ConsoleSharedData.getSession();
         Rack rack = (Rack)session.findObjectById(objectId);
         setPartName(String.format("Physical link for %s at %s", rack.getPassiveElement(patchPanelId).toString(), rack.getObjectName()));
      }
   }

   /**
    * @param b
    * @param defval
    * @return
    */
   private static boolean safeCast(String s, boolean b, boolean defval)
   {
      return (s != null) ? b : defval;
   }

   @Override
   public void createPartControl(Composite parent)
   {
      FormLayout formLayout = new FormLayout();
      parent.setLayout(formLayout);
      
      linkWidget = new PhysicalLinkWidget(this, parent, SWT.NONE, objectId, patchPanelId, initShowFilter, null); //$NON-NLS-1$
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      linkWidget.setLayoutData(fd);

      linkWidget.addDisposeListener(new DisposeListener() {

         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            settings.put("PhysicalLinkView", linkWidget.isFilterEnabled());            
         }
      });

      linkWidget.contributeToActionBars();
      activateContext();
   }

   /**
    * Activate context
    */
   private void activateContext()
   {
      IContextService contextService = (IContextService)getSite().getService(IContextService.class);
      if (contextService != null)
      {
         contextService.activateContext("org.netxms.ui.eclipse.objectview.context.PhysicalLinkView"); //$NON-NLS-1$
      }
   }

   @Override
   public void setFocus()
   {
      linkWidget.setFocus();
   }

}
