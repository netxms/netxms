package com.radensolutions.reporting.custom;

import com.radensolutions.reporting.service.ServerSettings;
import net.sf.jasperreports.engine.*;
import net.sf.jasperreports.engine.query.JRAbstractQueryExecuter;

import java.util.Map;

public class NxclQueryExecutor extends JRAbstractQueryExecuter {

    private ServerSettings settings;

    protected NxclQueryExecutor(JasperReportsContext jasperReportsContext, JRDataset dataset, Map<String, ? extends JRValueParameter> parametersMap) {
        super(jasperReportsContext, dataset, parametersMap);
        parseQuery();
    }

    @Override
    protected String getParameterReplacement(String parameterName) {
//        System.out.println("getParameterReplacement: " + parameterName);
//        System.out.println(getParameterValue(parameterName));
        return null;
    }

    @Override
    public JRDataSource createDatasource() throws JRException {
        String queryString = getQueryString();
        if ("epp".equalsIgnoreCase(queryString)) {
            EppDataSource dataSource = new EppDataSource();
            dataSource.connect(settings.getNetxmsServer(), settings.getNetxmsLogin(), settings.getNetxmsPassword());
            return dataSource;
        }
        return null;
    }

    @Override
    public void close() {
    }

    @Override
    public boolean cancelQuery() throws JRException {
        System.out.println("cancelQuery");
        return false;
    }

    public void setSettings(ServerSettings settings) {
        this.settings = settings;
    }
}
