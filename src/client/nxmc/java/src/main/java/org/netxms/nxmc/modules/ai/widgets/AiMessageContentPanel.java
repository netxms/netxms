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
package org.netxms.nxmc.modules.ai.widgets;

import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.ai.AiMessage;
import org.netxms.client.constants.AiMessageStatus;
import org.netxms.nxmc.base.widgets.MarkdownViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Panel for displaying AI message content with Approve/Reject buttons
 */
public class AiMessageContentPanel extends Composite
{
   private final I18n i18n = LocalizationHelper.getI18n(AiMessageContentPanel.class);

   private static final int AUTO_READ_DELAY_MS = 30000; // 30 seconds

   private AiMessage currentMessage;
   private Label titleLabel;
   private MarkdownViewer contentViewer;
   private Composite buttonArea;
   private Button approveButton;
   private Button rejectButton;
   private AiMessageActionHandler actionHandler;
   private Runnable autoReadRunnable;

   /**
    * Action handler interface for message actions
    */
   public interface AiMessageActionHandler
   {
      void approveMessage(AiMessage message);
      void rejectMessage(AiMessage message);
      void markMessageAsRead(AiMessage message);
   }

   /**
    * Create AI message content panel
    *
    * @param parent parent composite
    * @param style widget style
    * @param actionHandler handler for approve/reject actions
    */
   public AiMessageContentPanel(Composite parent, int style, AiMessageActionHandler actionHandler)
   {
      super(parent, style);
      this.actionHandler = actionHandler;

      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = 0;
      setLayout(layout);

      Composite headerArea = new Composite(this, SWT.NONE);
      layout = new GridLayout();
      layout.numColumns = 3;
      headerArea.setLayout(layout);
      headerArea.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      // Title label
      titleLabel = new Label(headerArea, SWT.NONE);
      titleLabel.setFont(JFaceResources.getFontRegistry().getBold(JFaceResources.HEADER_FONT));
      titleLabel.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      Label spacer = new Label(headerArea, SWT.NONE);
      GridData gd = new GridData();
      gd.widthHint = 20;
      spacer.setLayoutData(gd);

      // Button area (for approve/reject)
      buttonArea = new Composite(headerArea, SWT.NONE);
      GridLayout buttonLayout = new GridLayout(2, true);
      buttonLayout.marginHeight = 0;
      buttonLayout.marginWidth = 0;
      buttonArea.setLayout(buttonLayout);
      buttonArea.setLayoutData(new GridData(SWT.RIGHT, SWT.CENTER, false, false));

      approveButton = new Button(buttonArea, SWT.PUSH);
      approveButton.setImage(SharedIcons.IMG_APPROVE);
      approveButton.setText(i18n.tr("Approve"));
      approveButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            if (currentMessage != null && actionHandler != null)
               actionHandler.approveMessage(currentMessage);
         }
      });
      gd = new GridData();
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      approveButton.setLayoutData(gd);

      rejectButton = new Button(buttonArea, SWT.PUSH);
      rejectButton.setImage(SharedIcons.IMG_REJECT);
      rejectButton.setText(i18n.tr("Reject"));
      rejectButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            if (currentMessage != null && actionHandler != null)
               actionHandler.rejectMessage(currentMessage);
         }
      });
      gd = new GridData();
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      rejectButton.setLayoutData(gd);

      // Initially hide button area and show placeholder
      buttonArea.setVisible(false);
      ((GridData)buttonArea.getLayoutData()).exclude = true;

      new Label(this, SWT.SEPARATOR | SWT.HORIZONTAL).setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      // Content viewer (markdown)
      contentViewer = new MarkdownViewer(this, SWT.NONE);
      contentViewer.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

      showPlaceholder();
   }

   /**
    * Show placeholder when no message is selected
    */
   private void showPlaceholder()
   {
      titleLabel.setText(i18n.tr("Select a message to view its content"));
      contentViewer.setText("");
   }

   /**
    * Set the message to display
    *
    * @param message message to display, or null to clear
    */
   public void setMessage(AiMessage message)
   {
      // Cancel any pending auto-read timer
      cancelAutoReadTimer();

      this.currentMessage = message;

      if (message == null)
      {
         showPlaceholder();
         setButtonsVisible(false);
         return;
      }

      titleLabel.setText(message.getTitle());
      contentViewer.setText(message.getText());

      // Show approve/reject buttons only for pending approval requests
      boolean showButtons = message.isApprovalRequest() && (message.getStatus() == AiMessageStatus.PENDING || message.getStatus() == AiMessageStatus.READ);
      System.out.println(message);
      setButtonsVisible(showButtons);

      // Start auto-read timer for pending messages
      if (message.getStatus() == AiMessageStatus.PENDING)
      {
         scheduleAutoRead(message.getId());
      }

      layout(true, true);
   }

   /**
    * Schedule auto-read timer for a message
    *
    * @param messageId message ID to mark as read after delay
    */
   private void scheduleAutoRead(final long messageId)
   {
      autoReadRunnable = () -> {
         if (!isDisposed() && currentMessage != null &&
             currentMessage.getId() == messageId &&
             currentMessage.getStatus() == AiMessageStatus.PENDING &&
             actionHandler != null)
         {
            actionHandler.markMessageAsRead(currentMessage);
         }
      };
      getDisplay().timerExec(AUTO_READ_DELAY_MS, autoReadRunnable);
   }

   /**
    * Cancel pending auto-read timer
    */
   private void cancelAutoReadTimer()
   {
      if (autoReadRunnable != null)
      {
         getDisplay().timerExec(-1, autoReadRunnable);
         autoReadRunnable = null;
      }
   }

   /**
    * Set visibility of approve/reject buttons
    *
    * @param visible true to show buttons
    */
   private void setButtonsVisible(boolean visible)
   {
      buttonArea.setVisible(visible);
      ((GridData)buttonArea.getLayoutData()).exclude = !visible;
      layout(true, true);
   }

   /**
    * Get current message
    *
    * @return current message or null
    */
   public AiMessage getCurrentMessage()
   {
      return currentMessage;
   }

   /**
    * Update display if the message was modified
    *
    * @param message updated message
    */
   public void updateMessage(AiMessage message)
   {
      if (currentMessage != null && currentMessage.getId() == message.getId())
      {
         setMessage(message);
      }
   }
}
