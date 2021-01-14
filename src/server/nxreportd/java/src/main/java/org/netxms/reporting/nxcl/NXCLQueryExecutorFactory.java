package org.netxms.reporting.nxcl;

import net.sf.jasperreports.engine.JRDataset;
import net.sf.jasperreports.engine.JRException;
import net.sf.jasperreports.engine.JRValueParameter;
import net.sf.jasperreports.engine.JasperReportsContext;
import net.sf.jasperreports.engine.query.AbstractQueryExecuterFactory;
import net.sf.jasperreports.engine.query.JRQueryExecuter;

import java.util.Map;

public class NXCLQueryExecutorFactory extends AbstractQueryExecuterFactory
{
   public NXCLQueryExecutorFactory()
   {
   }

   /**
    * @see net.sf.jasperreports.engine.query.QueryExecuterFactory#getBuiltinParameters()
    */
   @Override
   public Object[] getBuiltinParameters()
   {
      return new Object[0];
   }

   /**
    * @see net.sf.jasperreports.engine.query.QueryExecuterFactory#createQueryExecuter(net.sf.jasperreports.engine.JasperReportsContext,
    *      net.sf.jasperreports.engine.JRDataset, java.util.Map)
    */
   @Override
   public JRQueryExecuter createQueryExecuter(JasperReportsContext jasperReportsContext, JRDataset dataset,
         Map<String, ? extends JRValueParameter> parameters) throws JRException
   {
      return new NXCLQueryExecutor(jasperReportsContext, dataset, parameters);
   }

   /**
    * @see net.sf.jasperreports.engine.query.QueryExecuterFactory#supportsQueryParameterType(java.lang.String)
    */
   @Override
   public boolean supportsQueryParameterType(String className)
   {
      return true;
   }
}
