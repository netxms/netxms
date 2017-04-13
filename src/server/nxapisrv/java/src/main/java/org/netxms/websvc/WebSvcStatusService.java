/**
 * Web services API for NetXMS
 * Copyright (c) 2017 Raden Solutions
 */
package org.netxms.websvc;

import org.json.JSONException;
import org.json.JSONObject;
import org.netxms.base.CommonRCC;
import org.netxms.client.constants.RCC;
import org.restlet.Request;
import org.restlet.Response;
import org.restlet.data.MediaType;
import org.restlet.data.Status;
import org.restlet.representation.Representation;
import org.restlet.representation.StringRepresentation;
import org.restlet.resource.Resource;
import org.restlet.service.StatusService;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Status service
 */
public class WebSvcStatusService extends StatusService
{
   private Logger log = LoggerFactory.getLogger(WebSvcStatusService.class);
   
   /* (non-Javadoc)
    * @see org.restlet.service.StatusService#toStatus(java.lang.Throwable, org.restlet.Request, org.restlet.Response)
    */
   @Override
   public Status toStatus(Throwable throwable, Request request, Response response)
   {
      if (throwable != null)
      {
         if (throwable instanceof WebSvcException)
         {
            log.info("Request processing error: " + throwable.getMessage());
            return new Status(getStatusFromRCC(((WebSvcException)throwable).getErrorCode()), throwable);
         }
         log.error("Exception in request handler", throwable);
      }
      return super.toStatus(throwable, request, response);
   }

   /* (non-Javadoc)
    * @see org.restlet.service.StatusService#toStatus(java.lang.Throwable, org.restlet.resource.Resource)
    */
   @Override
   public Status toStatus(Throwable throwable, Resource resource)
   {
      if (throwable != null)
      {
         if (throwable instanceof WebSvcException)
         {
            log.info("Request processing error: " + throwable.getMessage());
            return new Status(getStatusFromRCC(((WebSvcException)throwable).getErrorCode()), throwable);
         }
         log.error("Exception in request handler", throwable);
      }
      return super.toStatus(throwable, resource);
   }
   
   /**
    * Get HTTP status code from NetXMS RCC
    * 
    * @param rcc NetXMS RCC
    * @return HTTP status
    */
   public static Status getStatusFromRCC(int rcc)
   {
      switch(rcc)
      {
         case RCC.ACCESS_DENIED:
            return Status.CLIENT_ERROR_UNAUTHORIZED;
         case RCC.INCOMPATIBLE_OPERATION:
            return Status.CLIENT_ERROR_METHOD_NOT_ALLOWED;
         case RCC.INTERNAL_ERROR:
            return Status.SERVER_ERROR_INTERNAL;
         case RCC.SUCCESS:
            return Status.SUCCESS_OK;
         case RCC.INVALID_ACTION_ID:
         case RCC.INVALID_ALARM_ID:
         case RCC.INVALID_ALARM_NOTE_ID:
         case RCC.INVALID_CONFIG_ID:
         case RCC.INVALID_DCI_ID:
         case RCC.INVALID_EVENT_CODE:
         case RCC.INVALID_EVENT_ID:
         case RCC.INVALID_GRAPH_ID:
         case RCC.INVALID_JOB_ID:
         case RCC.INVALID_MAP_ID:
         case RCC.INVALID_MAPPING_TABLE_ID:
         case RCC.INVALID_OBJECT_ID:
         case RCC.INVALID_PACKAGE_ID:
         case RCC.INVALID_POLICY_ID:
         case RCC.INVALID_SCRIPT_ID:
         case RCC.INVALID_SUMMARY_TABLE_ID:
         case RCC.INVALID_TOOL_ID:
         case RCC.INVALID_TRAP_ID:
         case RCC.INVALID_USER_ID:
         case RCC.INVALID_ZONE_ID:
            return Status.CLIENT_ERROR_NOT_FOUND;
         case RCC.NOT_IMPLEMENTED:
            return Status.SERVER_ERROR_NOT_IMPLEMENTED;
         default:
            return Status.CLIENT_ERROR_BAD_REQUEST;
      }
   }
   
   /**
    * Get message text for given RCC
    * 
    * @param rcc NetXMS RCC
    * @return message text
    */
   public static String getMessageFromRCC(int rcc)
   {
      return RCC.getText(rcc, "en", null);
   }

   /* (non-Javadoc)
    * @see org.restlet.service.StatusService#getRepresentation(org.restlet.data.Status, org.restlet.Request, org.restlet.Response)
    */
   @Override
   public Representation getRepresentation(Status status, Request request, Response response)
   {
      JSONObject json = new JSONObject();
      if (status.getThrowable() instanceof WebSvcException)
      {
         WebSvcException e = (WebSvcException)status.getThrowable();
         try
         {
            json.put("error", e.getErrorCode());
            json.put("description", e.getMessage());
         }
         catch(JSONException e1)
         {
         }
      }
      else if (status.getThrowable() instanceof JSONException)
      {
         try
         {
            json.put("error", CommonRCC.INVALID_ARGUMENT);
            json.put("description", status.getThrowable().getMessage());
         }
         catch(JSONException e1)
         {
         }
      }
      else
      {
         log.debug("Internal error", status.getThrowable());
         try
         {
            json.put("error", CommonRCC.INTERNAL_ERROR);
            json.put("description", (status.getThrowable() != null) ? status.getThrowable().getMessage() : "Internal error");
         }
         catch(JSONException e1)
         {
         }
      }
      return new StringRepresentation(json.toString(), MediaType.APPLICATION_JSON);
   }
}
