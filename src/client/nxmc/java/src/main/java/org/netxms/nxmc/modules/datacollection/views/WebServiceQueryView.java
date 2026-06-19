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
package org.netxms.nxmc.modules.datacollection.views;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.InputDialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.StackLayout;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.constants.HttpRequestMethod;
import org.netxms.client.constants.HttpStatus;
import org.netxms.client.constants.Severity;
import org.netxms.client.constants.WebServiceAuthType;
import org.netxms.client.datacollection.WebServiceCallResult;
import org.netxms.client.datacollection.WebServiceDefinition;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.JsonViewer;
import org.netxms.nxmc.base.widgets.LabeledCombo;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.base.widgets.PasswordInputField;
import org.netxms.nxmc.base.widgets.RoundedLabel;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.dialogs.HeaderEditDialog;
import org.netxms.nxmc.modules.objects.views.AdHocObjectView;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Interactive web service query tool. Allows ad-hoc requests (URL, authentication, headers, body) to be executed through a node's
 * web service proxy, pre-filling fields from an existing web service definition and optionally saving changes back.
 */
public class WebServiceQueryView extends AdHocObjectView
{
   private final I18n i18n = LocalizationHelper.getI18n(WebServiceQueryView.class);

   private List<WebServiceDefinition> definitions = new ArrayList<WebServiceDefinition>();
   private Map<String, String> headers = new HashMap<String, String>();

   private LabeledCombo definitionCombo;
   private LabeledText name;
   private LabeledText url;
   private Combo httpMethod;
   private Combo authType;
   private LabeledText login;
   private PasswordInputField password;
   private LabeledSpinner timeout;
   private Button checkVerifyCert;
   private Button checkVerifyHost;
   private Button checkFollowLocation;
   private LabeledText requestData;
   private TableViewer headersViewer;
   private Button headerAddButton;
   private Button headerEditButton;
   private Button headerDeleteButton;

   private RoundedLabel statusLabel;
   private Composite outputArea;
   private StackLayout outputLayout;
   private JsonViewer jsonViewer;
   private Text plainOutput;

   private Action actionExecute;
   private Action actionSave;
   private Action actionSaveAs;
   private Action actionClearOutput;

   /**
    * Create web service query view for given object.
    *
    * @param objectId object ID this view is intended for
    * @param contextId context object ID
    */
   public WebServiceQueryView(long objectId, long contextId)
   {
      super(LocalizationHelper.getI18n(WebServiceQueryView.class).tr("Web Service Query"), SharedIcons.URL, "objects.web-service-query", objectId, contextId, false);
   }

   /**
    * Constructor for view restoration.
    */
   protected WebServiceQueryView()
   {
      super(LocalizationHelper.getI18n(WebServiceQueryView.class).tr("Web Service Query"), SharedIcons.URL, "objects.web-service-query", 0, 0, false);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      parent.setLayout(layout);

      createRequestForm(parent);
      createOutputArea(parent);
      createActions();
   }

   /**
    * Create request form (top part).
    *
    * @param parent parent composite
    */
   private void createRequestForm(Composite parent)
   {
      Composite form = new Composite(parent, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      layout.makeColumnsEqualWidth = true;
      form.setLayout(layout);
      form.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));

      definitionCombo = new LabeledCombo(form, SWT.NONE, SWT.READ_ONLY);
      definitionCombo.setLabel(i18n.tr("Web service definition"));
      definitionCombo.add(i18n.tr("<Custom request>"));
      definitionCombo.select(0);
      definitionCombo.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            onDefinitionSelected();
         }
      });
      definitionCombo.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, true, false));

      name = new LabeledText(form, SWT.NONE);
      name.setLabel(i18n.tr("Name"));
      name.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, true, false));

      url = new LabeledText(form, SWT.NONE);
      url.setLabel(i18n.tr("URL"));
      url.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, true, false, 2, 1));

      Composite requestArea = new Composite(form, SWT.NONE);
      requestArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false, 2, 1));
      GridLayout requestLayout = new GridLayout();
      requestLayout.marginHeight = 0;
      requestLayout.marginWidth = 0;
      requestLayout.numColumns = 3;
      requestArea.setLayout(requestLayout);

      httpMethod = WidgetHelper.createLabeledCombo(requestArea, SWT.READ_ONLY, i18n.tr("HTTP request method"), new GridData(SWT.FILL, SWT.CENTER, true, false));
      for(HttpRequestMethod m : HttpRequestMethod.values())
         httpMethod.add(m.toString());
      httpMethod.select(HttpRequestMethod.GET.getValue());

      authType = WidgetHelper.createLabeledCombo(requestArea, SWT.READ_ONLY, i18n.tr("Authentication"), new GridData(SWT.FILL, SWT.CENTER, true, false));
      for(WebServiceAuthType t : WebServiceAuthType.values())
         authType.add(t.toString());
      authType.select(WebServiceAuthType.NONE.getValue());

      Composite optionsArea = new Composite(requestArea, SWT.NONE);
      optionsArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, false, false, 1, 3));
      GridLayout optionsAreaLayout = new GridLayout();
      optionsAreaLayout.marginHeight = 0;
      optionsAreaLayout.marginWidth = 0;
      optionsArea.setLayout(optionsAreaLayout);

      timeout = new LabeledSpinner(optionsArea, SWT.NONE);
      timeout.setLabel(i18n.tr("Request timeout (seconds)"));
      timeout.setRange(0, 300);
      timeout.setSelection(0);
      timeout.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, false, false));

      Group options = new Group(optionsArea, SWT.NONE);
      options.setText(i18n.tr("Options"));
      options.setLayout(new RowLayout(SWT.VERTICAL));
      options.setLayoutData(new GridData(SWT.FILL, SWT.FILL, false, true));
      checkVerifyCert = new Button(options, SWT.CHECK);
      checkVerifyCert.setText(i18n.tr("Verify peer's certificate"));
      checkVerifyCert.setSelection(true);
      checkVerifyHost = new Button(options, SWT.CHECK);
      checkVerifyHost.setText(i18n.tr("Verify host name in certificate"));
      checkVerifyHost.setSelection(true);
      checkFollowLocation = new Button(options, SWT.CHECK);
      checkFollowLocation.setText(i18n.tr("Follow location header in a 3xx response"));

      requestData = new LabeledText(requestArea, SWT.NONE, SWT.BORDER | SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL);
      requestData.setLabel(i18n.tr("Request data"));
      GridData gd = new GridData(SWT.FILL, SWT.FILL, true, false, 1, 2);
      requestData.setLayoutData(gd);

      login = new LabeledText(requestArea, SWT.NONE);
      login.setLabel(i18n.tr("Login"));
      login.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      password = new PasswordInputField(requestArea, SWT.NONE);
      password.setLabel(i18n.tr("Password"));
      password.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      createHeadersEditor(form);
   }

   /**
    * Create headers editor block.
    *
    * @param parent parent composite
    */
   private void createHeadersEditor(Composite parent)
   {
      Composite headersGroup = new Composite(parent, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      headersGroup.setLayout(layout);
      headersGroup.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false, 2, 1));

      Label label = new Label(headersGroup, SWT.NONE);
      label.setText(i18n.tr("Headers"));
      label.setLayoutData(new GridData(SWT.FILL, SWT.FILL, false, false, 2, 1));

      headersViewer = new TableViewer(headersGroup, SWT.BORDER | SWT.FULL_SELECTION | SWT.MULTI);
      Table table = headersViewer.getTable();
      table.setHeaderVisible(true);
      TableColumn tc = new TableColumn(table, SWT.LEFT);
      tc.setText(i18n.tr("Name"));
      tc.setWidth(200);
      tc = new TableColumn(table, SWT.LEFT);
      tc.setText(i18n.tr("Value"));
      tc.setWidth(500);
      headersViewer.setContentProvider(new ArrayContentProvider());
      headersViewer.setLabelProvider(new HeaderLabelProvider());
      headersViewer.setInput(headers.entrySet());
      headersViewer.addSelectionChangedListener((e) -> {
         IStructuredSelection selection = headersViewer.getStructuredSelection();
         headerEditButton.setEnabled(selection.size() == 1);
         headerDeleteButton.setEnabled(!selection.isEmpty());
      });
      headersViewer.addDoubleClickListener((e) -> editHeader());

      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.heightHint = 160;
      headersViewer.getTable().setLayoutData(gd);

      Composite buttonArea = new Composite(headersGroup, SWT.NONE);
      RowLayout btnLayout = new RowLayout(SWT.VERTICAL);
      btnLayout.fill = true;
      btnLayout.marginTop = 0;
      btnLayout.marginBottom = 0;
      btnLayout.marginLeft = 0;
      btnLayout.marginRight = 0;
      buttonArea.setLayout(btnLayout);
      buttonArea.setLayoutData(new GridData(SWT.LEFT, SWT.TOP, false, false));

      headerAddButton = new Button(buttonArea, SWT.PUSH);
      headerAddButton.setText(i18n.tr("&Add..."));
      headerAddButton.setLayoutData(new RowData(WidgetHelper.BUTTON_WIDTH_HINT, SWT.DEFAULT));
      headerAddButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            addHeader();
         }
      });

      headerEditButton = new Button(buttonArea, SWT.PUSH);
      headerEditButton.setText(i18n.tr("&Edit..."));
      headerEditButton.setEnabled(false);
      headerEditButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            editHeader();
         }
      });

      headerDeleteButton = new Button(buttonArea, SWT.PUSH);
      headerDeleteButton.setText(i18n.tr("&Delete"));
      headerDeleteButton.setEnabled(false);
      headerDeleteButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            deleteHeaders();
         }
      });
   }

   /**
    * Create output area (bottom part).
    *
    * @param parent parent composite
    */
   private void createOutputArea(Composite parent)
   {
      Composite container = new Composite(parent, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.verticalSpacing = 0;
      container.setLayout(layout);
      container.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

      GridData gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.verticalIndent = 5;
      new Label(container, SWT.SEPARATOR | SWT.HORIZONTAL).setLayoutData(gd);

      statusLabel = new RoundedLabel(container);
      gd = new GridData(SWT.LEFT, SWT.CENTER, true, false);
      gd.verticalIndent = 5;
      gd.horizontalIndent = 10;
      statusLabel.setLayoutData(gd);
      statusLabel.setBlendMode(true);

      gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
      gd.verticalIndent = 5;
      new Label(container, SWT.SEPARATOR | SWT.HORIZONTAL).setLayoutData(gd);

      outputArea = new Composite(container, SWT.NONE);
      outputLayout = new StackLayout();
      outputLayout.marginHeight = 0;
      outputLayout.marginWidth = 0;
      outputArea.setLayout(outputLayout);
      outputArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

      jsonViewer = new JsonViewer(outputArea, SWT.NONE);
      plainOutput = new Text(outputArea, SWT.MULTI | SWT.READ_ONLY | SWT.H_SCROLL | SWT.V_SCROLL);
      outputLayout.topControl = plainOutput;
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
      loadDefinitions();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionExecute = new Action(i18n.tr("E&xecute"), SharedIcons.EXECUTE) {
         @Override
         public void run()
         {
            executeQuery();
         }
      };
      addKeyBinding("F9", actionExecute);

      actionSave = new Action(i18n.tr("&Save"), SharedIcons.SAVE) {
         @Override
         public void run()
         {
            saveDefinition(false);
         }
      };
      addKeyBinding("M1+S", actionSave);

      actionSaveAs = new Action(i18n.tr("Save &as..."), SharedIcons.SAVE_AS) {
         @Override
         public void run()
         {
            saveDefinition(true);
         }
      };
      addKeyBinding("M1+M2+S", actionSaveAs);

      actionClearOutput = new Action(i18n.tr("Clear &output"), SharedIcons.CLEAR_LOG) {
         @Override
         public void run()
         {
            plainOutput.setText("");
            jsonViewer.setContent("", false);
            statusLabel.setText("");
            statusLabel.getParent().layout(true, true);
         }
      };
      addKeyBinding("M1+L", actionClearOutput);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionExecute);
      manager.add(actionClearOutput);
      manager.add(new Separator());
      manager.add(actionSave);
      manager.add(actionSaveAs);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionExecute);
      manager.add(actionClearOutput);
      manager.add(new Separator());
      manager.add(actionSave);
      manager.add(actionSaveAs);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      loadDefinitions();
   }

   /**
    * Load available web service definitions into the dropdown.
    */
   private void loadDefinitions()
   {
      new Job(i18n.tr("Loading web service definitions"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<WebServiceDefinition> list = session.getWebServiceDefinitions();
            list.sort((lhs, rhs) -> lhs.getName().compareToIgnoreCase(rhs.getName()));
            runInUIThread(() -> {
               if (definitionCombo.isDisposed())
                  return;
               String selection = definitionCombo.getText();
               definitions = list;
               definitionCombo.removeAll();
               definitionCombo.add(i18n.tr("<Custom request>"));
               for(WebServiceDefinition d : list)
                  definitionCombo.add(d.getName());
               int index = definitionCombo.indexOf(selection);
               definitionCombo.select((index >= 0) ? index : 0);
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load web service definitions");
         }
      }.start();
   }

   /**
    * Pre-fill form from selected definition.
    */
   private void onDefinitionSelected()
   {
      int index = definitionCombo.getSelectionIndex();
      if (index <= 0)
         return; // 0 = custom request

      WebServiceDefinition d = definitions.get(index - 1);
      name.setText(safeString(d.getName()));
      url.setText(safeString(d.getUrl()));
      httpMethod.select(d.getHttpRequestMethod().getValue());
      authType.select(d.getAuthenticationType().getValue());
      login.setText(safeString(d.getLogin()));
      password.setText(safeString(d.getPassword()));
      timeout.setSelection(d.getRequestTimeout());
      checkVerifyCert.setSelection(d.isVerifyCertificate());
      checkVerifyHost.setSelection(d.isVerifyHost());
      checkFollowLocation.setSelection(d.isFollowLocation());
      requestData.setText(safeString(d.getRequestData()));
      headers.clear();
      headers.putAll(d.getHeaders());
      headersViewer.setInput(headers.entrySet());
      headersViewer.refresh();
   }

   /**
    * Build web service request from current form content.
    *
    * @param requestName name to assign to the definition
    * @return web service definition built from form
    */
   private WebServiceDefinition buildRequest(String requestName)
   {
      WebServiceDefinition d = new WebServiceDefinition((requestName != null) ? requestName : "adhoc");
      d.setUrl(url.getText().trim());
      d.setHttpRequestMethod(HttpRequestMethod.getByValue(httpMethod.getSelectionIndex()));
      d.setRequestData(requestData.getText());
      d.setAuthenticationType(WebServiceAuthType.getByValue(authType.getSelectionIndex()));
      d.setLogin(login.getText().trim());
      d.setPassword(password.getText());
      d.setRequestTimeout(timeout.getSelection());
      d.setVerifyCertificate(checkVerifyCert.getSelection());
      d.setVerifyHost(checkVerifyHost.getSelection());
      d.setFollowLocation(checkFollowLocation.getSelection());
      d.getHeaders().clear();
      for(Entry<String, String> e : headers.entrySet())
         d.setHeader(e.getKey(), e.getValue());
      return d;
   }

   /**
    * Execute web service query.
    */
   private void executeQuery()
   {
      if (url.getText().trim().isEmpty())
      {
         MessageDialogHelper.openWarning(getWindow().getShell(), i18n.tr("Warning"), i18n.tr("URL cannot be empty"));
         return;
      }

      final long nodeId = getObjectId();
      final WebServiceDefinition request = buildRequest(null);
      actionExecute.setEnabled(false);
      clearMessages();
      new Job(i18n.tr("Querying web service"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final WebServiceCallResult result = session.queryWebService(nodeId, request);
            runInUIThread(() -> showResult(result));
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot query web service");
         }

         @Override
         protected void jobFinalize()
         {
            runInUIThread(() -> actionExecute.setEnabled(true));
         }
      }.start();
   }

   /**
    * Display query result.
    *
    * @param result web service call result
    */
   private void showResult(WebServiceCallResult result)
   {
      if (statusLabel.isDisposed())
         return;

      showStatus(result.getHttpResponseCode());

      String document = result.getDocument();
      if (document == null)
         document = "";

      String trimmed = document.trim();
      boolean shownAsJson = false;
      if (trimmed.startsWith("{") || trimmed.startsWith("["))
      {
         try
         {
            jsonViewer.setContent(document, true);
            outputLayout.topControl = jsonViewer;
            shownAsJson = true;
         }
         catch(Exception e)
         {
            shownAsJson = false;
         }
      }

      if (!shownAsJson)
      {
         plainOutput.setText(document);
         outputLayout.topControl = plainOutput;
      }
      outputArea.layout();
   }

   /**
    * Show status
    *
    * @param statusCode HTTP status code
    */
   void showStatus(int statusCode)
   {
      Color color;
      HttpStatus status = HttpStatus.of(statusCode);
      statusLabel.setText(status.toString());
      if (status.isSuccess())
         color = StatusDisplayInfo.getStatusColor(Severity.NORMAL);
      else if (status.isRedirection())
         color = StatusDisplayInfo.getStatusColor(Severity.UNKNOWN);
      else if (status.isError())
         color = StatusDisplayInfo.getStatusColor(Severity.CRITICAL);
      else
         color = StatusDisplayInfo.getStatusColor(Severity.MINOR);
      statusLabel.setLabelBackground(color);
      statusLabel.getParent().layout(true, true);
   }

   /**
    * Save current request as a web service definition.
    *
    * @param saveAs if true, prompt for a new name and create a new definition
    */
   private void saveDefinition(boolean saveAs)
   {
      String svcName;
      if (saveAs)
      {
         InputDialog dlg = new InputDialog(getWindow().getShell(), i18n.tr("Save As"), i18n.tr("Web service definition name"), name.getText().trim(), null);
         if (dlg.open() != Window.OK)
            return;
         svcName = dlg.getValue().trim();
      }
      else
      {
         svcName = name.getText().trim();
      }

      if (svcName.isEmpty())
      {
         MessageDialogHelper.openWarning(getWindow().getShell(), i18n.tr("Warning"), i18n.tr("Web service definition name cannot be empty"));
         return;
      }

      final WebServiceDefinition definition = buildRequest(svcName);
      if (!saveAs)
      {
         for(WebServiceDefinition existing : definitions)
         {
            if (existing.getName().equalsIgnoreCase(svcName))
            {
               definition.setId(existing.getId());
               definition.setGuid(existing.getGuid());
               break;
            }
         }
      }

      new Job(i18n.tr("Saving web service definition"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modifyWebServiceDefinition(definition);
            runInUIThread(() -> {
               name.setText(svcName);
               loadDefinitions();
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot save web service definition");
         }
      }.start();
   }

   /**
    * Add new header.
    */
   private void addHeader()
   {
      HeaderEditDialog dlg = new HeaderEditDialog(getWindow().getShell(), null, null);
      if (dlg.open() == Window.OK)
      {
         headers.put(dlg.getName(), dlg.getValue());
         headersViewer.refresh();
      }
   }

   /**
    * Edit selected header.
    */
   @SuppressWarnings("unchecked")
   private void editHeader()
   {
      IStructuredSelection selection = headersViewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      Entry<String, String> e = (Entry<String, String>)selection.getFirstElement();
      HeaderEditDialog dlg = new HeaderEditDialog(getWindow().getShell(), e.getKey(), e.getValue());
      if (dlg.open() == Window.OK)
      {
         headers.put(dlg.getName(), dlg.getValue());
         headersViewer.refresh();
      }
   }

   /**
    * Delete selected headers.
    */
   @SuppressWarnings("unchecked")
   private void deleteHeaders()
   {
      IStructuredSelection selection = headersViewer.getStructuredSelection();
      for(Object o : selection.toList())
         headers.remove(((Entry<String, String>)o).getKey());
      headersViewer.refresh();
   }

   /**
    * @param s string that may be null
    * @return non-null string
    */
   private static String safeString(String s)
   {
      return (s != null) ? s : "";
   }

   /**
    * Label provider for headers table.
    */
   private static class HeaderLabelProvider extends LabelProvider implements ITableLabelProvider
   {
      @Override
      public Image getColumnImage(Object element, int columnIndex)
      {
         return null;
      }

      @SuppressWarnings("unchecked")
      @Override
      public String getColumnText(Object element, int columnIndex)
      {
         return (columnIndex == 0) ? ((Entry<String, String>)element).getKey() : ((Entry<String, String>)element).getValue();
      }
   }
}
