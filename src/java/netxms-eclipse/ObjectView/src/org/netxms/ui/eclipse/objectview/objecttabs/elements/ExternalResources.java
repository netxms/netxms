/**
 * 
 */
package org.netxms.ui.eclipse.objectview.objecttabs.elements;

import java.net.URL;
import java.util.ArrayList;
import java.util.List;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.widgets.Hyperlink;
import org.netxms.client.ObjectUrl;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.console.resources.SharedColors;
import org.netxms.ui.eclipse.objectview.Activator;
import org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * @author victor
 *
 */
public class ExternalResources extends OverviewPageElement
{
   private Composite content;
   private List<Element> elements = new ArrayList<Element>();
   
   /**
    * @param parent
    * @param anchor
    * @param objectTab
    */
   public ExternalResources(Composite parent, OverviewPageElement anchor, ObjectTab objectTab)
   {
      super(parent, anchor, objectTab);
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#getTitle()
    */
   @Override
   protected String getTitle()
   {
      return "External Resources";
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#isApplicableForObject(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public boolean isApplicableForObject(AbstractObject object)
   {
      return object.hasUrls();
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#createClientArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createClientArea(Composite parent)
   {
      content = new Composite(parent, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      content.setLayout(layout);
      content.setBackground(SharedColors.getColor(SharedColors.OBJECT_TAB_BACKGROUND, parent.getDisplay()));
      return content;
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#onObjectChange()
    */
   @Override
   protected void onObjectChange()
   {
      for(Element e : elements)
         e.dispose();
      elements.clear();
      for(ObjectUrl u : getObject().getUrls())
         elements.add(new Element(content, u));
      content.layout();
   }
   
   /**
    * Open URL
    * 
    * @param url URL to open
    */
   private void openUrl(URL url)
   {
      try
      {
         PlatformUI.getWorkbench().getBrowserSupport().getExternalBrowser().openURL(url);
      }
      catch(PartInitException e)
      {
         Activator.log("Exception when trying to open URL " + url.toExternalForm(), e);
         MessageDialogHelper.openError(PlatformUI.getWorkbench().getActiveWorkbenchWindow().getShell(), "Error", "Cannot start external web browser");
      }
   }
   
   /**
    * Display element
    */
   private class Element
   {
      ObjectUrl url;
      Hyperlink link;
      Label description;
      
      Element(Composite parent, ObjectUrl url)
      {
         this.url = url;
         
         link = new Hyperlink(parent, SWT.NONE);
         link.setBackground(content.getBackground());
         link.setForeground(SharedColors.getColor(SharedColors.COMMAND_BOX_TEXT, link.getDisplay()));
         link.setText(url.getUrl().toExternalForm());
         link.addHyperlinkListener(new HyperlinkAdapter() {
            @Override
            public void linkActivated(HyperlinkEvent e)
            {
               openUrl(Element.this.url.getUrl());
            }
            
         });
         
         description = new Label(parent, SWT.NONE);
         description.setBackground(content.getBackground());
         description.setText(url.getDescription());
      }
      
      void dispose()
      {
         link.dispose();
         description.dispose();
      }
   }
}
