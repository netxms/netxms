/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.serverconfig.dialogs;

import java.net.Inet4Address;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.UnknownHostException;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.base.InetAddressEx;
import org.netxms.client.InetAddressListElement;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.ObjectSelectionFilterFactory;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.modules.objects.widgets.ZoneSelector;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for adding or editing address list element
 */
public class AddressListElementEditDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(AddressListElementEditDialog.class);

	private Button radioSubnet;
	private Button radioRange;
	private LabeledText textAddr1;
	private LabeledText textAddr2;
   private LabeledText comments;
	private ZoneSelector zoneSelector;
	private ObjectSelector proxySelector;
	private InetAddressListElement element;
	private boolean enableProxySelection;
   private boolean warnOnWideMask;

	/**
    * Create IP address list element edit dialog
    * 
    * @param parentShell parent shell
    * @param enableProxySelection true to enable proxy selector
    * @param warnOnWideMask warn user when mask is too wide
    * @param element address list element to edit
    */
   public AddressListElementEditDialog(Shell parentShell, boolean enableProxySelection, boolean warnOnWideMask, InetAddressListElement element)
	{
		super(parentShell);
		this.enableProxySelection = enableProxySelection;
      this.warnOnWideMask = warnOnWideMask;
      this.element = element;
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
      newShell.setText((element != null) ? i18n.tr("Edit Address List Element") : i18n.tr("Add Address List Element"));
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);

		GridLayout layout = new GridLayout();
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
		dialogArea.setLayout(layout);

		radioSubnet = new Button(dialogArea, SWT.RADIO);
      radioSubnet.setText(i18n.tr("&Subnet"));
		radioSubnet.setSelection(true);
      radioSubnet.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
            textAddr1.setLabel(i18n.tr("Network address"));
            textAddr2.setLabel(i18n.tr("Network mask"));
            if (textAddr2.getText().equals("0.0.0.0"))
               textAddr2.setText("0");
			}
		});

		radioRange = new Button(dialogArea, SWT.RADIO);
      radioRange.setText(i18n.tr("Address &range"));
      radioRange.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
            textAddr1.setLabel(i18n.tr("Start address"));
            textAddr2.setLabel(i18n.tr("End address"));
            if (textAddr2.getText().equals("0"))
               textAddr2.setText("0.0.0.0");
			}
		});

		textAddr1 = new LabeledText(dialogArea, SWT.NONE);
      textAddr1.setLabel(i18n.tr("Network address"));
      textAddr1.setText("0.0.0.0");
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = 300;
		textAddr1.setLayoutData(gd);

		textAddr2 = new LabeledText(dialogArea, SWT.NONE);
      textAddr2.setLabel(i18n.tr("Network mask"));
      textAddr2.setText("0");
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		textAddr2.setLayoutData(gd);
		
		if (enableProxySelection)
		{
         if (Registry.getSession().isZoningEnabled())
		   {
		      zoneSelector = new ZoneSelector(dialogArea, SWT.NONE, true);
            zoneSelector.setLabel(i18n.tr("Zone"));
		      gd = new GridData();
		      gd.horizontalAlignment = SWT.FILL;
		      gd.grabExcessHorizontalSpace = true;
		      zoneSelector.setLayoutData(gd);
		   }

		   proxySelector = new ObjectSelector(dialogArea, SWT.NONE, true);
         proxySelector.setLabel(i18n.tr("Proxy node"));
         proxySelector.setEmptySelectionName(i18n.tr("Zone proxy"));
		   proxySelector.setClassFilter(ObjectSelectionFilterFactory.getInstance().createNodeSelectionFilter(false));
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         proxySelector.setLayoutData(gd);
		}

      comments = new LabeledText(dialogArea, SWT.NONE);
      comments.setLabel(i18n.tr("Comments"));
      comments.getTextControl().setTextLimit(255);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      comments.setLayoutData(gd);

      if (element != null)
      {
         radioSubnet.setSelection(element.isSubnet());
         radioRange.setSelection(!element.isSubnet());
         if (!element.isSubnet())
         {
            textAddr1.setLabel(i18n.tr("Start address"));
            textAddr2.setLabel(i18n.tr("End address"));
         }
         textAddr1.setText(element.getBaseAddress().getHostAddress());
         textAddr2.setText(element.isSubnet() ? Integer.toString(element.getMaskBits()) : element.getEndAddress().getHostAddress());
         if (enableProxySelection)
         {
            if (Registry.getSession().isZoningEnabled())
            {
               zoneSelector.setZoneUIN(element.getZoneUIN());
            }
            proxySelector.setObjectId(element.getProxyId());
         }
         comments.setText(element.getComment());
      }

		return dialogArea;
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
		try
		{
		   int zoneUIN = (zoneSelector != null) ? zoneSelector.getZoneUIN() : 0;
		   if (zoneUIN == -1)
		      zoneUIN = 0;
		   long proxyId = (proxySelector != null) ? proxySelector.getObjectId() : 0;
	      if (radioSubnet.getSelection())
	      {
	         InetAddress baseAddress = InetAddress.getByName(textAddr1.getText().trim());
            String maskText = textAddr2.getText().trim();
            int maskBits;
            if ((baseAddress instanceof Inet4Address) && (maskText.indexOf('.') > 0))
            {
               InetAddress maskAddress = InetAddress.getByName(maskText);
               if (maskAddress instanceof Inet4Address)
               {
                  maskBits = InetAddressEx.bitsInMask(maskAddress);
               }
               else
               {
                  throw new NumberFormatException("Invalid network mask");
               }
            }
            else
            {
               maskBits = Integer.parseInt(maskText);
            }
	         if ((maskBits < 0) ||
	             ((baseAddress instanceof Inet4Address) && (maskBits > 32)) ||
                ((baseAddress instanceof Inet6Address) && (maskBits > 128)))
            {
	            throw new NumberFormatException("Invalid network mask");
            }

            if (warnOnWideMask && (maskBits < 16))
            {
               if (!MessageDialogHelper.openQuestion(getShell(), i18n.tr("Warning"), i18n.tr("You have specified network mask that includes more than 65536 IP addresses. Do you really intend it?")))
                  return;
            }

            if (element == null)
	            element = new InetAddressListElement(baseAddress, maskBits, zoneUIN, proxyId, comments.getText());
	         else
	            element.update(baseAddress, maskBits, zoneUIN, proxyId, comments.getText());
	      }
	      else
	      {
            InetAddress addr1 = InetAddress.getByName(textAddr1.getText().trim());
            InetAddress addr2 = InetAddress.getByName(textAddr2.getText().trim());

            if (warnOnWideMask && (addr1 instanceof Inet4Address) && (addr2 instanceof Inet4Address))
            {
               byte[] bytes1 = addr1.getAddress();
               byte[] bytes2 = addr2.getAddress();

               if ((bytes1[0] != bytes2[0]) || (bytes1[1] != bytes2[1]))
               {
                  if (!MessageDialogHelper.openQuestion(getShell(), i18n.tr("Warning"),
                        i18n.tr("You have specified address range that includes more than 65536 IP addresses. Do you really intend it?")))
                     return;
               }
            }

            if (element == null)
               element = new InetAddressListElement(addr1, addr2, zoneUIN, proxyId, comments.getText());
	         else
               element.update(addr1, addr2, zoneUIN, proxyId, comments.getText());
	      }
		}
      catch(UnknownHostException | NumberFormatException e)
		{
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Please enter valid IP address/mask"));
			return;
		}
		super.okPressed();
	}

   /**
    * @return the element
    */
   public InetAddressListElement getElement()
   {
      return element;
   }
}
