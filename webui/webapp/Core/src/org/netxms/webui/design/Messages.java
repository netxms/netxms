/*******************************************************************************
 * Copyright (c) 2010, 2011 EclipseSource and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *    EclipseSource - initial API and implementation
 ******************************************************************************/
package org.netxms.webui.design;

import org.eclipse.rwt.RWT;

public class Messages {

  private static final String BUNDLE_NAME 
    = "org.netxms.webui.design.messages"; //$NON-NLS-1$
  public String ConfigurationDialog_Cancel;
  public String ConfigurationDialog_ConfigurationFor;
  public String ConfigurationDialog_Ok;
  public String ConfigurationDialog_ViewMenu;
  public String ConfigurationDialog_VisibleActions;
  public String PerspectiveSwitcherBuilder_Close;
  public String PerspectiveSwitcherBuilder_Other;
  public String ViewStackPresentation_ConfButtonToolTipDisabled;
  public String ViewStackPresentation_ConfButtonToolTipEnabled;
  
  public static Messages get() {
    Object result = RWT.NLS.getISO8859_1Encoded( BUNDLE_NAME, Messages.class );
    return ( Messages )result;
  }

  private Messages() {
    //prevent initialization
  }
}
