package com.radensolutions.reporting.custom;

import net.sf.jasperreports.engine.JRDataset;
import net.sf.jasperreports.engine.JRException;
import net.sf.jasperreports.engine.JRValueParameter;
import net.sf.jasperreports.engine.JasperReportsContext;
import net.sf.jasperreports.engine.query.AbstractQueryExecuterFactory;
import net.sf.jasperreports.engine.query.JRQueryExecuter;

import java.util.Map;

public class NXCLQueryExecutorFactoryDummy extends AbstractQueryExecuterFactory {
    @Override
    public Object[] getBuiltinParameters() {
        return new Object[0];
    }

    @Override
    public JRQueryExecuter createQueryExecuter(JasperReportsContext jasperReportsContext, JRDataset jrDataset, Map<String, ? extends JRValueParameter> map) throws JRException {
        return null;
    }

    @Override
    public boolean supportsQueryParameterType(String s) {
        return false;
    }
}
