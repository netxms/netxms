package org.netxms.webui.filters;

import java.io.IOException;
import javax.servlet.Filter;
import javax.servlet.FilterChain;
import javax.servlet.FilterConfig;
import javax.servlet.ServletException;
import javax.servlet.ServletRequest;
import javax.servlet.ServletResponse;
import javax.servlet.http.HttpServletResponse;

/**
 * Security filter for servlet
 */
public class SecurityFilter implements Filter
{
   /**
    * @see javax.servlet.Filter#init(javax.servlet.FilterConfig)
    */
   @Override
   public void init(FilterConfig filterConfig) throws ServletException
   {
   }

   /**
    * @see javax.servlet.Filter#doFilter(javax.servlet.ServletRequest, javax.servlet.ServletResponse, javax.servlet.FilterChain)
    */
   @Override
   public void doFilter(ServletRequest request, ServletResponse response, FilterChain chain) throws IOException, ServletException
   {
      ((HttpServletResponse)response).addHeader("X-Frame-Options", "DENY");
      chain.doFilter(request, response);
   }

   /**
    * @see javax.servlet.Filter#destroy()
    */
   @Override
   public void destroy()
   {
   }
}
