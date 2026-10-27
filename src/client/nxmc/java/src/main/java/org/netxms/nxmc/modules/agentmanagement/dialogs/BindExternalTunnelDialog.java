/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.agentmanagement.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.AgentTunnel;
import org.netxms.client.constants.CertificateMappingMethod;
import org.netxms.client.objects.AbstractNode;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for binding tunnel with externally provisioned certificate. Such tunnels are attached to nodes via certificate mapping,
 * so user selects target node (optionally) and mapping method; actual mapping data is taken from the certificate presented by the
 * agent and shown next to each method.
 */
public class BindExternalTunnelDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(BindExternalTunnelDialog.class);

   private AgentTunnel tunnel;
   private boolean selectNode;
   private ObjectSelector nodeSelector;
   private Button[] methodButtons = new Button[4];
   private long nodeId = 0;
   private CertificateMappingMethod mappingMethod = CertificateMappingMethod.SUBJECT;

   /**
    * Create dialog.
    *
    * @param parentShell parent shell
    * @param tunnel tunnel to bind
    * @param selectNode true to show node selector (false when node will be created by caller)
    */
   public BindExternalTunnelDialog(Shell parentShell, AgentTunnel tunnel, boolean selectNode)
   {
      super(parentShell);
      this.tunnel = tunnel;
      this.selectNode = selectNode;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Bind Tunnel"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      dialogArea.setLayout(layout);

      Label hint = new Label(dialogArea, SWT.WRAP);
      hint.setText(i18n.tr("Agent certificate is externally provisioned. Tunnel will be attached to the node using certificate mapping set from the certificate presented by the agent."));
      GridData gd = new GridData(SWT.FILL, SWT.TOP, true, false);
      gd.widthHint = 500;
      hint.setLayoutData(gd);

      if (selectNode)
      {
         nodeSelector = new ObjectSelector(dialogArea, SWT.NONE, false);
         nodeSelector.setLabel(i18n.tr("Node"));
         nodeSelector.setObjectClass(AbstractNode.class);
         nodeSelector.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));
      }

      Group methodGroup = new Group(dialogArea, SWT.NONE);
      methodGroup.setText(i18n.tr("Certificate mapping method"));
      methodGroup.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));
      layout = new GridLayout();
      layout.numColumns = 2;
      layout.horizontalSpacing = WidgetHelper.DIALOG_SPACING;
      methodGroup.setLayout(layout);

      addMethodOption(methodGroup, CertificateMappingMethod.SUBJECT, i18n.tr("Subject"), tunnel.getCertificateSubject());
      addMethodOption(methodGroup, CertificateMappingMethod.COMMON_NAME, i18n.tr("Common name"), tunnel.getCertificateCommonName());
      addMethodOption(methodGroup, CertificateMappingMethod.TEMPLATE_ID, i18n.tr("Template ID"), tunnel.getCertificateTemplateId());
      addMethodOption(methodGroup, CertificateMappingMethod.PUBLIC_KEY, i18n.tr("Public key"), tunnel.getCertificatePublicKeyFingerprint());

      methodButtons[CertificateMappingMethod.SUBJECT.getValue()].setSelection(true);
      return dialogArea;
   }

   /**
    * Add mapping method option (radio button with value from certificate next to it).
    *
    * @param parent parent composite
    * @param method mapping method
    * @param name method display name
    * @param value mapping data value from certificate (null or empty if not available)
    */
   private void addMethodOption(Composite parent, CertificateMappingMethod method, String name, String value)
   {
      boolean available = (value != null) && !value.isEmpty();

      Button button = new Button(parent, SWT.RADIO);
      button.setText(name);
      button.setEnabled(available);
      button.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, false, false));
      methodButtons[method.getValue()] = button;

      Text text = new Text(parent, SWT.BORDER | SWT.READ_ONLY);
      text.setText(available ? value : i18n.tr("not available"));
      text.setEnabled(available);
      GridData gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.widthHint = 400;
      text.setLayoutData(gd);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      if (selectNode)
      {
         nodeId = nodeSelector.getObjectId();
         if (nodeId == 0)
         {
            MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Please select target node"));
            return;
         }
      }

      for(int i = 0; i < methodButtons.length; i++)
      {
         if (methodButtons[i].getSelection())
         {
            mappingMethod = CertificateMappingMethod.getByValue(i);
            break;
         }
      }
      super.okPressed();
   }

   /**
    * Get selected node ID (valid only if dialog was created with node selector).
    *
    * @return selected node ID
    */
   public long getNodeId()
   {
      return nodeId;
   }

   /**
    * Get selected certificate mapping method.
    *
    * @return selected certificate mapping method
    */
   public CertificateMappingMethod getMappingMethod()
   {
      return mappingMethod;
   }
}
