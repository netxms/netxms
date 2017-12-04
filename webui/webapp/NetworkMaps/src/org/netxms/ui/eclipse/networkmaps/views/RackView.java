package org.netxms.ui.eclipse.networkmaps.views;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RackOrientation;
import org.netxms.client.objects.Rack;
import org.netxms.ui.eclipse.console.resources.SharedColors;
import org.netxms.ui.eclipse.objectview.widgets.RackWidget;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

public class RackView extends ViewPart
{
   public static final String ID = "org.netxms.ui.eclipse.networkmaps.views.rack_view";
   
   private Composite rackArea;
   private Rack rack;
   private RackWidget rackFrontWidget;
   private RackWidget rackRearWidget;


   /* (non-Javadoc)
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
    */
   @Override
   public void init(IViewSite site) throws PartInitException
   {
      super.init(site);
      
      NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      String[] parts = site.getSecondaryId().split("&"); //$NON-NLS-1$
      rack = session.findObjectById(Long.parseLong((parts.length > 0) ? parts[0] : site.getSecondaryId()), Rack.class);
      if (rack == null)
         throw new PartInitException("Rack not found!");
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   {
      if (rack != null)
      {
         rackArea = new Composite(parent, SWT.NONE) {
            @Override
            public Point computeSize(int wHint, int hHint, boolean changed)
            {
               if ((rackFrontWidget == null) || (rackRearWidget == null) || (hHint == SWT.DEFAULT))
                  return super.computeSize(wHint, hHint, changed);
               
               Point s = rackFrontWidget.computeSize(wHint, hHint, changed);
               return new Point(s.x * 2, s.y);
            }
         };
         rackFrontWidget = new RackWidget(rackArea, SWT.NONE, rack, RackOrientation.FRONT);
         rackRearWidget = new RackWidget(rackArea, SWT.NONE, rack, RackOrientation.REAR);
         
         rackArea.setBackground(SharedColors.getColor(SharedColors.RACK_BACKGROUND, parent.getDisplay()));
         rackArea.addControlListener(new ControlAdapter() {
            @Override
            public void controlResized(ControlEvent e)
            {
               if ((rackFrontWidget == null) || (rackRearWidget == null))
                  return;
               
               int height = rackArea.getSize().y;
               Point size = rackFrontWidget.computeSize(SWT.DEFAULT, height, true);
               rackFrontWidget.setSize(size);
               rackRearWidget.setSize(size);
               rackRearWidget.setLocation(size.x, 0);
            }
         });
      }
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
   }
}
