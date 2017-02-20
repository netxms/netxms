/**
 * 
 */
package org.netxms.websvc.handlers;

import org.json.JSONObject;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.websvc.SessionStore;
import org.netxms.websvc.SessionToken;
import org.restlet.data.CookieSetting;
import org.restlet.data.MediaType;
import org.restlet.ext.json.JsonRepresentation;
import org.restlet.representation.Representation;
import org.restlet.representation.StringRepresentation;
import org.restlet.resource.Post;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Session requests handler
 */
public class Sessions extends AbstractHandler
{
   private Logger log = LoggerFactory.getLogger(Sessions.class);
   
   /* (non-Javadoc)
    * @see org.netxms.websvc.handlers.AbstractHandler#onPost(org.restlet.representation.Representation)
    */
   @Override
   @Post
   public Representation onPost(Representation entity) throws Exception
   {
      if (entity == null)
      {
         log.warn("No POST data in login call");
         return new StringRepresentation(createErrorResponse(RCC.ACCESS_DENIED).toString(), MediaType.APPLICATION_JSON);
      }
      
      JSONObject request = new JsonRepresentation(entity).getJsonObject();
      String login = request.getString("login");
      String password = request.getString("password");
      if ((login == null) || (password == null))
      {
         log.warn("Login or password not specified in login call");
         return new StringRepresentation(createErrorResponse(RCC.ACCESS_DENIED).toString(), MediaType.APPLICATION_JSON);
      }
      
      NXCSession session = new NXCSession("127.0.0.1");
      session.connect();
      session.login(login, password);
      
      SessionToken token = SessionStore.getInstance(getServletContext()).registerSession(session);

      log.info("Logged in to NetXMS server, assigned session id " + token.getGuid());
      getCookieSettings().add(new CookieSetting("session_handle", token.getGuid().toString()));
      
      JSONObject response = new JSONObject();
      response.put("session", token.getGuid().toString());
      response.put("serverVersion", session.getServerVersion());
      return new StringRepresentation(response.toString(), MediaType.APPLICATION_JSON);
   }

   /* (non-Javadoc)
    * @see org.netxms.websvc.handlers.AbstractHandler#delete(java.lang.String)
    */
   @Override
   protected Object delete(String id) throws Exception
   {
      if (getSessionToken().getGuid().toString().equals(id))
      {
         log.info("Logout request for session " + id);
         getSession().disconnect();
         SessionStore.getInstance(getServletContext()).unregisterSession(getSessionToken().getGuid());
         return new JSONObject();
      }
      else
      {
         log.warn("Logout request for different session (" + getSessionToken().getGuid() + " -- " + id);
         return createErrorResponse(RCC.ACCESS_DENIED);
      }
   }
}
