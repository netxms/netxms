/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.modules.users.propertypages;

import java.util.List;
import java.util.stream.Collectors;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.NXCSession;
import org.netxms.client.users.AuthenticationToken;
import org.netxms.client.users.User;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.base.widgets.MessageAreaHolder;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.users.dialogs.IssueTokenDialog;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Authentication Tokens" property page for user object
 */
public class TokenManagement extends PropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(TokenManagement.class);

   private static final int COLUMN_ID = 0;
   private static final int COLUMN_DESCRIPTION = 1;
   private static final int COLUMN_CREATED = 2;
   private static final int COLUMN_EXPIRES = 3;

   private NXCSession session = Registry.getSession();
   private User user;
   private List<AuthenticationToken> tokens;
   private SortableTableViewer viewer;
   private Button revokeButton;

   /**
    * Constructor
    *
    * @param user user object
    * @param messageArea message area holder
    */
   public TokenManagement(User user, MessageAreaHolder messageArea)
   {
      super(LocalizationHelper.getI18n(TokenManagement.class).tr("Authentication Tokens"), messageArea);
      this.user = user;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Composite dialogArea = new Composite(parent, SWT.NONE);

      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      final String[] columnNames = { i18n.tr("ID"), i18n.tr("Description"), i18n.tr("Created"), i18n.tr("Expires") };
      final int[] columnWidths = { 80, 200, 150, 150 };
      viewer = new SortableTableViewer(dialogArea, columnNames, columnWidths, COLUMN_ID, SWT.UP, SWT.MULTI | SWT.FULL_SELECTION | SWT.BORDER);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 300;
      viewer.getControl().setLayoutData(gd);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new TokenListLabelProvider());
      viewer.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            return Integer.compare(((AuthenticationToken)e1).getId(), ((AuthenticationToken)e2).getId());
         }
      });

      Composite buttons = new Composite(dialogArea, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttonLayout.marginRight = 0;
      buttons.setLayout(buttonLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      buttons.setLayoutData(gd);

      Button issueButton = new Button(buttons, SWT.PUSH);
      issueButton.setText(i18n.tr("&Issue New..."));
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      issueButton.setLayoutData(rd);
      issueButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            issueToken();
         }
      });

      revokeButton = new Button(buttons, SWT.PUSH);
      revokeButton.setText(i18n.tr("&Revoke"));
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      revokeButton.setLayoutData(rd);
      revokeButton.setEnabled(false);
      revokeButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            revokeTokens();
         }
      });

      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = viewer.getStructuredSelection();
            revokeButton.setEnabled(!selection.isEmpty());
         }
      });

      loadTokens();

      return dialogArea;
   }

   /**
    * Load tokens from server
    */
   private void loadTokens()
   {
      new Job(i18n.tr("Loading authentication tokens"), null, getMessageArea(false)) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<AuthenticationToken> result = session.getAuthenticationTokens(user.getId())
                  .stream()
                  .filter(AuthenticationToken::isPersistent)
                  .collect(Collectors.toList());
            runInUIThread(() -> {
               tokens = result;
               viewer.setInput(tokens);
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load authentication tokens");
         }
      }.start();
   }

   /**
    * Issue new token
    */
   private void issueToken()
   {
      IssueTokenDialog dlg = new IssueTokenDialog(getShell(), user.getId());
      dlg.open();
      loadTokens();  // Refresh token list after dialog closes
   }

   /**
    * Revoke selected tokens
    */
   private void revokeTokens()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openQuestion(getShell(), i18n.tr("Confirm Revocation"),
            i18n.tr("Are you sure you want to revoke selected authentication token(s)?")))
         return;

      final int[] tokenIds = ((List<?>)selection.toList()).stream()
            .mapToInt(o -> ((AuthenticationToken)o).getId())
            .toArray();

      new Job(i18n.tr("Revoking authentication tokens"), null, getMessageArea(false)) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(int tokenId : tokenIds)
            {
               session.revokeAuthenticationToken(tokenId);
            }
            runInUIThread(() -> loadTokens());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot revoke authentication token");
         }
      }.start();
   }

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
   {
      // All changes are applied immediately
      return true;
   }

   /**
    * Label provider for token list
    */
   private class TokenListLabelProvider extends LabelProvider implements ITableLabelProvider
   {
      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
       */
      @Override
      public Image getColumnImage(Object element, int columnIndex)
      {
         return null;
      }

      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
       */
      @Override
      public String getColumnText(Object element, int columnIndex)
      {
         AuthenticationToken token = (AuthenticationToken)element;
         switch(columnIndex)
         {
            case COLUMN_ID:
               return Integer.toString(token.getId());
            case COLUMN_DESCRIPTION:
               return token.getDescription();
            case COLUMN_CREATED:
               return (token.getCreationTime() != null) ? DateFormatFactory.getDateTimeFormat().format(token.getCreationTime()) : "";
            case COLUMN_EXPIRES:
               return (token.getExpirationTime() != null) ? DateFormatFactory.getDateTimeFormat().format(token.getExpirationTime()) : "";
         }
         return "";
      }
   }
}
