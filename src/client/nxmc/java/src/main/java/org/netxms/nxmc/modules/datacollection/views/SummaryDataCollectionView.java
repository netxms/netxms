/**
 * NetXMS - open source network management system
 * Copyright (C) 2021-2024 Raden Solutions
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
package org.netxms.nxmc.modules.datacollection.views;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.fieldassist.ContentProposalAdapter;
import org.eclipse.jface.fieldassist.IContentProposal;
import org.eclipse.jface.fieldassist.IContentProposalListener;
import org.eclipse.jface.fieldassist.TextContentAdapter;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.KeyEvent;
import org.eclipse.swt.events.KeyListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Collector;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.EntireNetwork;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.client.objects.Subnet;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.base.widgets.helpers.SearchQueryAttribute;
import org.netxms.nxmc.base.widgets.helpers.SearchQueryContentProposalProvider;
import org.netxms.nxmc.base.windows.MainWindow;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.SharedIcons;
import org.xnap.commons.i18n.I18n;

/**
 * View to search Data Collection elements on containers
 */
public class SummaryDataCollectionView extends BaseDataCollectionView
{
   private final I18n i18n = LocalizationHelper.getI18n(SummaryDataCollectionView.class);

   public static final int LV_COLUMN_SOURCE = 0;   

   private final SearchQueryAttribute[] attributeProposals = {
         new SearchQueryAttribute("Description:"),
         new SearchQueryAttribute("Guid:"),
         new SearchQueryAttribute("Id:"),
         new SearchQueryAttribute("Name:"),
         new SearchQueryAttribute("NOT"),
         new SearchQueryAttribute("PollingInterval:"),
         new SearchQueryAttribute("RetentionTime:"),
         new SearchQueryAttribute("SourceNode:")
   };

   private LabeledText queryEditor; 
   private Button startButton;
   private String searchFilter;
   private ContentProposalAdapter proposalAdapter;
   private Action actionShowOnNode;

   /**
    * Constructor
    */
   public SummaryDataCollectionView()
   {
      super(LocalizationHelper.getI18n(SummaryDataCollectionView.class).tr("Data Collection"), "objects.data-collection-summary", false);
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View view)
   {
      super.postClone(view);
      SummaryDataCollectionView origin = (SummaryDataCollectionView)view;
      queryEditor.setText(origin.queryEditor.getText());
      searchFilter = origin.searchFilter;
      viewer.setInput(origin.viewer.getInput()); 
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      parent.setLayout(new FormLayout());
      
      final Composite searchBar = new Composite(parent, SWT.NONE);
      GridLayout gridLayout = new GridLayout();
      gridLayout.numColumns = 2;
      searchBar.setLayout(gridLayout);
      
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      searchBar.setLayoutData(fd);
      
      queryEditor = new LabeledText(searchBar, SWT.NONE);
      queryEditor.setLabel("Search string");
      queryEditor.getTextControl().addKeyListener(new KeyListener() {
         @Override
         public void keyReleased(KeyEvent e)
         {
         }

         @Override
         public void keyPressed(KeyEvent e)
         {
            if (e.stateMask == 0 && e.keyCode == 13 && !proposalAdapter.isProposalPopupOpen())
            {
               searchFilter = queryEditor.getText(); //Save string for autorefresh
               getDataFromServer(null);
            }
         }
      });
      queryEditor.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      startButton = new Button(searchBar, SWT.PUSH);
      startButton.setImage(SharedIcons.IMG_EXECUTE);
      startButton.setText("Start");
      startButton.setToolTipText("Start search");
      startButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            searchFilter = queryEditor.getText(); //Save string for autorefresh
            getDataFromServer(null);
         }
      });
      startButton.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, false, false));
      
      final Composite mainContent = new Composite(parent, SWT.NONE);
      mainContent.setLayout(new FillLayout());
      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(searchBar);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      mainContent.setLayoutData(fd);
      
      super.createContent(mainContent);
      
      proposalAdapter = new ContentProposalAdapter(queryEditor.getControl(), new TextContentAdapter(), new SearchQueryContentProposalProvider(attributeProposals), null, null);
      proposalAdapter.setPropagateKeys(true);
      proposalAdapter.setProposalAcceptanceStyle(ContentProposalAdapter.PROPOSAL_IGNORE);
      proposalAdapter.addContentProposalListener(new IContentProposalListener() {
         @Override
         public void proposalAccepted(IContentProposal proposal)
         {
            Text textControl = queryEditor.getTextControl();
            String content = queryEditor.getText();
            if (content.isEmpty())
            {
               textControl.append(proposal.getContent());
               return;
            }
            int pos = textControl.getCaretPosition() - 1;
            while((pos > 0) && !isStopCharacter(content.charAt(pos)))
               pos--;
            textControl.setSelection(isStopCharacter(content.charAt(pos)) ? pos + 1 : pos, textControl.getCaretPosition());
            textControl.insert(proposal.getContent());
         }
      });
      enableFilterAutocomplete(new SearchQueryContentProposalProvider(attributeProposals));
   }

   /**
    * Check if given character is a stop character for autocomplete element search.
    *
    * @param ch character to test
    * @return true if given character is a stop character for autocomplete element search
    */
   private static boolean isStopCharacter(char ch)
   {
      return (ch == ':') || (ch == ',') || Character.isWhitespace(ch);
   }
   
   /**
    * Get data from server
    */
   @Override
   protected void getDataFromServer(Runnable callback)
   {
      if (getObject() == null || searchFilter == null || searchFilter.isEmpty())
      {
         viewer.setInput(new DciValue[0]);
         return;
      }
      final AbstractObject obj = getObject();
      final String searchString = searchFilter;

      Job job = new Job(String.format(i18n.tr("Get DCI values for node %s"), obj.getObjectName()), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final DciValue[] data = session.findDCI(obj.getObjectId(), searchString);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (!viewer.getTable().isDisposed() && (getObject() != null) && (getObject().getObjectId() == obj.getObjectId()))
                  {
                     viewer.setInput(data);
                     clearMessages();
                  }
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot get DCI values for node %s"), obj.getObjectName());
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   @Override
   protected void fillContextMenu(final IMenuManager manager)
   {
      super.fillContextMenu(manager);
      manager.add(new Separator());
      manager.add(actionShowOnNode);
   }
   


   /**
    * Create actions
    */
   @Override
   protected void createActions()
   {
      super.createActions();      

      actionShowOnNode = new Action(i18n.tr("Go to &Nodes DCI")) {
         @Override
         public void run()
         {
            showOnNode();
         }
      };
   }
   
   /**
    * Open template and it's DCI
    */
   private void showOnNode()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      DciValue dci = (DciValue)selection.getFirstElement();      
      MainWindow.switchToObject(dci.getNodeId(), dci.getId());
   }
   
   /**
    * If owner column should be hidden
    * 
    * @return true if should be hidden
    */
   @Override
   protected boolean isHideOwner()
   {
      return false;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      if (context == null)
         return false;
      
      if (context instanceof Container || context instanceof Collector || context instanceof EntireNetwork ||
            context instanceof ServiceRoot || context instanceof Subnet) 
         return true;
      
      return false;
   }
}
