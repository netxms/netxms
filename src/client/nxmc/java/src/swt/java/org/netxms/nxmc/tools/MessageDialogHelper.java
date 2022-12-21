/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
package org.netxms.nxmc.tools;

import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Helper class for displaying message dialogs
 */
public class MessageDialogHelper
{
   private static final Logger logger = LoggerFactory.getLogger(MessageDialogHelper.class);

   /**
    * Status code returned by <code>openQuestionWithCancel</code> if the user pressed the <b>Yes</b> button.
    */
   public static final int YES = 0;

   /**
    * Status code returned by <code>openQuestionWithCancel</code> if the user pressed the <b>No</b> button.
    */
   public static final int NO = 1;

   /**
    * Status code returned by <code>openQuestionWithCancel</code> if the user pressed the <b>Cancel</b> button or closed dialog by
    * any other means.
    */
   public static final int CANCEL = 2;

	/**
	 * Convenience method to open a simple dialog as specified by the
	 * <code>kind</code> flag.
	 * 
	 * @param kind
	 *            the kind of dialog to open, one of {@link MessageDialog#ERROR},
	 *            {@link MessageDialog#INFORMATION}, {@link MessageDialog#QUESTION}, {@link MessageDialog#WARNING},
	 *            {@link MessageDialog#CONFIRM}, or {@link MessageDialog#QUESTION_WITH_CANCEL}.
	 * @param parent
	 *            the parent shell of the dialog, or <code>null</code> if none
	 * @param title
	 *            the dialog's title, or <code>null</code> if none
	 * @param message
	 *            the message
	 * @return <code>true</code> if the user presses the OK or Yes button,
	 *         <code>false</code> otherwise
	 */
	public static boolean open(int kind, Shell parent, String title, String message)
	{
		return MessageDialog.open(kind, parent, title, message, SWT.SHEET);
	}

	/**
	 * Convenience method to open a simple confirm (OK/Cancel) dialog.
	 * 
	 * @param parent the parent shell of the dialog, or <code>null</code> if none
	 * @param title the dialog's title, or <code>null</code> if none
	 * @param message the message
	 * @return <code>true</code> if the user presses the OK button, <code>false</code> otherwise
	 */
	public static boolean openConfirm(Shell parent, String title, String message)
	{
		return open(MessageDialog.CONFIRM, parent, title, message);
	}
	
   /**
    * Convenience method to open a simple confirm (OK/Cancel) dialog with a check box
    * to remember selection. 
    * 
    * @param parent the parent shell of the dialog, or <code>null</code> if none
    * @param title the dialog's title, or <code>null</code> if none
    * @param label the label for the check box
    * @param message the message
    * @return 
    */
   public static DialogData openConfirmWithCheckbox(Shell parent, String title, String label, String message)
   {
      MessageDialogWithCheckbox msg = new MessageDialogWithCheckbox(MessageDialog.CONFIRM, new String[] { IDialogConstants.OK_LABEL, IDialogConstants.CANCEL_LABEL }, parent, title, label, message);
      return msg.openMsg();
   }

	/**
	 * Convenience method to open a standard error dialog.
	 * 
	 * @param parent the parent shell of the dialog, or <code>null</code> if none
	 * @param title the dialog's title, or <code>null</code> if none
	 * @param message the message
	 */
	public static void openError(Shell parent, String title, String message)
	{
		open(MessageDialog.ERROR, parent, title, message);
      logger.info("Error dialog shown to user (title=\"" + title + "\", message=\"" + message + "\"");
	}

	/**
	 * Convenience method to open a standard information dialog.
	 * 
	 * @param parent the parent shell of the dialog, or <code>null</code> if none
	 * @param title the dialog's title, or <code>null</code> if none
	 * @param message the message
	 */
	public static void openInformation(Shell parent, String title, String message)
	{
		open(MessageDialog.INFORMATION, parent, title, message);
	}

	/**
	 * Convenience method to open a simple Yes/No question dialog.
	 * 
	 * @param parent the parent shell of the dialog, or <code>null</code> if none
	 * @param title the dialog's title, or <code>null</code> if none
	 * @param message the message
	 * @return <code>true</code> if the user presses the Yes button, <code>false</code> otherwise
	 */
	public static boolean openQuestion(Shell parent, String title, String message)
	{
		return open(MessageDialog.QUESTION, parent, title, message);
	}

   /**
    * Convenience method to open a simple Yes/No/Cancel question dialog.
    * 
    * @param parent the parent shell of the dialog, or <code>null</code> if none
    * @param title the dialog's title, or <code>null</code> if none
    * @param message the message
    * @return <code>MessageDialogHelper.YES</code> if the user presses the <b>Yes</b> button, <code>MessageDialogHelper.NO</code> if
    *         the user presses the <b>No</b> button, and <code>MessageDialogHelper.CANCEL</code> otherwise
    */
   public static int openQuestionWithCancel(Shell parent, String title, String message)
   {
      int selection = MessageDialog.open(MessageDialog.QUESTION_WITH_CANCEL, parent, title, message, SWT.SHEET,
            new String[] { IDialogConstants.YES_LABEL, IDialogConstants.NO_LABEL, IDialogConstants.CANCEL_LABEL });
      return (selection == SWT.DEFAULT) ? CANCEL : selection;
   }

	/**
	 * Convenience method to open a standard warning dialog.
	 * 
	 * @param parent the parent shell of the dialog, or <code>null</code> if none
	 * @param title the dialog's title, or <code>null</code> if none
	 * @param message the message
	 */
	public static void openWarning(Shell parent, String title, String message)
	{
		open(MessageDialog.WARNING, parent, title, message);
      logger.info("Warning dialog shown to user (title=\"" + title + "\", message=\"" + message + "\"");
	}

   /**
    * Convenience method to open a standard warning dialog with a check box to remember selection.
    * 
    * @param parent the parent shell of the dialog, or <code>null</code> if none
    * @param title the dialog's title, or <code>null</code> if none
    * @param checkboxLabel the label for the check box
    * @param message the message
    * @return dialog execution result
    */
   public static DialogData openWarningWithCheckbox(Shell parent, String title, String checkboxLabel, String message)
   {
      MessageDialogWithCheckbox msg = new MessageDialogWithCheckbox(MessageDialog.WARNING, new String[] { IDialogConstants.OK_LABEL, IDialogConstants.CANCEL_LABEL }, parent, title, checkboxLabel, message);
      return msg.openMsg();
   }

   /**
    * Convenience method to open a warning dialog with custom button labels and a check box to remember selection.
    * 
    * @param parent the parent shell of the dialog, or <code>null</code> if none
    * @param title the dialog's title, or <code>null</code> if none
    * @param checkboxLabel the label for the check box
    * @param message the message
    * @param buttonLabels button labels
    * @return dialog execution result
    */
   public static DialogData openWarningWithCheckbox(Shell parent, String title, String checkboxLabel, String message, String[] buttonLabels)
   {
      MessageDialogWithCheckbox msg = new MessageDialogWithCheckbox(MessageDialog.WARNING, buttonLabels, parent, title, checkboxLabel, message);
      return msg.openMsg();
   }

   /**
    * Convenience method to open a standard one button (OK) warning dialog with a check box to remember selection.
    * 
    * @param parent the parent shell of the dialog, or <code>null</code> if none
    * @param title the dialog's title, or <code>null</code> if none
    * @param checkboxLabel the label for the check box
    * @param message the message
    * @return dialog execution result
    */
   public static DialogData openOneButtonWarningWithCheckbox(Shell parent, String title, String checkboxLabel, String message)
   {
      MessageDialogWithCheckbox msg = new MessageDialogWithCheckbox(MessageDialog.WARNING, new String[] { IDialogConstants.OK_LABEL }, parent, title, checkboxLabel, message);
      return msg.openMsg();
   }

	/**
	 * Helper class to show message dialog with check box (for example to add "do not show again" option)
	 */
	private static class MessageDialogWithCheckbox extends MessageDialog
	{
	   private String label;
	   private boolean saveSelection = false;
	   
	   public MessageDialogWithCheckbox(int kind, String[] buttonLabels, Shell parent, String title, String label, String message)
	   {
	      super(parent, title, null, message, kind, buttonLabels, 0);
	      this.label = label;
	   }
	   
	   @Override
	   protected Control createDialogArea(Composite parent)
	   {
	      Composite container = (Composite) super.createDialogArea(parent);
	      GridLayout layout = new GridLayout();
	      container.setLayout(layout);
	      
         GridData gd = new GridData(SWT.LEFT, SWT.BOTTOM, false, false);
	      gd.horizontalSpan = 2;
	      Button button = new Button(container, SWT.CHECK);
	      button.setLayoutData(gd);
	      button.setText(label);
	      button.addSelectionListener(new SelectionListener() {
            
            @Override
            public void widgetSelected(SelectionEvent e)
            {
               if (saveSelection)
                  saveSelection = false;
               else
                  saveSelection = true;                  
            }
            
            @Override
            public void widgetDefaultSelected(SelectionEvent e)
            {
               widgetSelected(e);               
            }
         });
	      
         return container;	      
	   }

	   /**
	    * @return MessageReturn object with dialog exit states
	    */
	   public DialogData openMsg()
	   {
	      return new DialogData(open(), saveSelection);
	   }	   
	}
}
