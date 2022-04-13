/**
 * 
 */
package org.netxms.nxmc.modules.filemanager.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.AgentFileFingerprint;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Dialog to show file fingerprint in file manager
 */
public class FileFingerprintDialog extends Dialog
{
   private AgentFileFingerprint fp;

   /**
    * @param parent
    */
   public FileFingerprintDialog(Shell parent, AgentFileFingerprint fp)
   {
      super(parent);
      this.fp = fp;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      dialogArea.setLayout(layout);

      addField(dialogArea, "File size (in bytes)", Long.toString(fp.getSize()), false);
      addField(dialogArea, "CRC32", Long.toHexString(fp.getCRC32()), false);
      addField(dialogArea, "MD5", hexString(fp.getMD5()), false);
      addField(dialogArea, "SHA256", hexString(fp.getSHA256()), false);

      byte[] data = fp.getFileData();
      if ((data != null) && (data.length > 0))
      {
         StringBuilder sb = new StringBuilder();
         for(int offset = 0; offset < data.length;)
         {
            sb.append(String.format(" %04X  ", offset));

            char[] text = new char[16];
            int b;
            for(b = 0; (b < 16) && (offset < data.length); b++, offset++)
            {
               byte ch = data[offset];
               int ub = (int)ch & 0xFF;
               if (ub < 0x10)
                  sb.append('0');
               sb.append(Integer.toHexString(ub));
               sb.append(' ');
               text[b] = ((ch < 32) || (ch >= 127)) ? '.' : (char)ch;
            }
   
            for(; b < 16; b++)
            {
               sb.append("   ");
               text[b] = ' ';
            }

            sb.append("  ");
            sb.append(text);
            sb.append(" \n");
         }
         addField(dialogArea, String.format("First %d bytes of file", data.length), sb.toString(), true);
      }

      return dialogArea;
   }

   /**
    * Add display field
    *
    * @param parent parent composite
    * @param name field name
    * @param value field value
    */
   private void addField(Composite parent, String name, String value, boolean multiline)
   {
      LabeledText text = multiline ? new LabeledText(parent, SWT.NONE, SWT.MULTI | SWT.BORDER) : new LabeledText(parent, SWT.NONE);
      text.setLabel(name);
      text.setEditable(false);
      text.setText(value);
      text.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      text.getTextControl().setFont(JFaceResources.getTextFont());
   }

   /**
    * Convert byte array to hex string
    *
    * @param bytes input
    * @return input converted to hex string
    */
   private static String hexString(byte[] bytes)
   {
      StringBuilder sb = new StringBuilder();
      for(byte b : bytes)
      {
         int ub = (int)b & 0xFF;
         if (ub < 0x10)
            sb.append('0');
         sb.append(Integer.toHexString(ub));
      }
      return sb.toString();
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText("File Fingerprint");
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createButtonsForButtonBar(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createButtonsForButtonBar(Composite parent)
   {
      createButton(parent, IDialogConstants.OK_ID, "Close", true);
   }
}
