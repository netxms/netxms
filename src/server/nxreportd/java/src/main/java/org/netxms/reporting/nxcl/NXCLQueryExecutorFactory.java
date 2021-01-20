/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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
package org.netxms.reporting.nxcl;

import net.sf.jasperreports.engine.JRDataset;
import net.sf.jasperreports.engine.JRException;
import net.sf.jasperreports.engine.JRValueParameter;
import net.sf.jasperreports.engine.JasperReportsContext;
import net.sf.jasperreports.engine.query.AbstractQueryExecuterFactory;
import net.sf.jasperreports.engine.query.JRQueryExecuter;

import java.util.Map;

/**
 * Query executor factory for providing custom query executors for NetXMS server API access.
 */
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
