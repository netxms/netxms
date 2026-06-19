package org.netxms.client.constants;

import java.util.HashMap;
import java.util.Map;

/**
 * HTTP status code per RFC 9110 (HTTP Semantics).
 * Carries the numeric code, a reason phrase (well-known phrase for recognised codes, empty otherwise), and a category.
 * Unlike a fixed enumeration, this class can represent any numeric status code, including ones with no defined reason phrase.
 *
 * Usage:
 *   HttpStatus.of(404).code()       → 404
 *   HttpStatus.of(404).reason()     → "Not Found"
 *   HttpStatus.of(404).category()   → Category.CLIENT_ERROR
 *   HttpStatus.of(404).isError()    → true
 *   HttpStatus.of(599).reason()     → "" (unrecognised code)
 */
public final class HttpStatus
{
   public static enum Category
   {
      INFORMATIONAL(1), SUCCESS(2), REDIRECTION(3), CLIENT_ERROR(4), SERVER_ERROR(5), UNKNOWN(0);

      private final int series;

      Category(int series)
      {
         this.series = series;
      }

      public int series()
      {
         return series;
      }

      public static Category of(int code)
      {
         switch(code / 100)
         {
            case 1:
               return INFORMATIONAL;
            case 2:
               return SUCCESS;
            case 3:
               return REDIRECTION;
            case 4:
               return CLIENT_ERROR;
            case 5:
               return SERVER_ERROR;
            default:
               return UNKNOWN;
         }
      }
   }

   private static final Map<Integer, HttpStatus> WELL_KNOWN = new HashMap<Integer, HttpStatus>();

   private static HttpStatus register(int code, String reason)
   {
      HttpStatus status = new HttpStatus(code, reason);
      WELL_KNOWN.put(code, status);
      return status;
   }

   // ── 1xx Informational ───────────────────────────────────────────────────
   public static final HttpStatus CONTINUE = register(100, "Continue");
   public static final HttpStatus SWITCHING_PROTOCOLS = register(101, "Switching Protocols");
   public static final HttpStatus PROCESSING = register(102, "Processing");                        // RFC 2518 (WebDAV)
   public static final HttpStatus EARLY_HINTS = register(103, "Early Hints");                      // RFC 8297

   // ── 2xx Success ─────────────────────────────────────────────────────────
   public static final HttpStatus OK = register(200, "OK");
   public static final HttpStatus CREATED = register(201, "Created");
   public static final HttpStatus ACCEPTED = register(202, "Accepted");
   public static final HttpStatus NON_AUTHORITATIVE_INFORMATION = register(203, "Non-Authoritative Information");
   public static final HttpStatus NO_CONTENT = register(204, "No Content");
   public static final HttpStatus RESET_CONTENT = register(205, "Reset Content");
   public static final HttpStatus PARTIAL_CONTENT = register(206, "Partial Content");
   public static final HttpStatus MULTI_STATUS = register(207, "Multi-Status");                    // RFC 4918 (WebDAV)
   public static final HttpStatus ALREADY_REPORTED = register(208, "Already Reported");            // RFC 5842
   public static final HttpStatus IM_USED = register(226, "IM Used");                              // RFC 3229

   // ── 3xx Redirection ─────────────────────────────────────────────────────
   public static final HttpStatus MULTIPLE_CHOICES = register(300, "Multiple Choices");
   public static final HttpStatus MOVED_PERMANENTLY = register(301, "Moved Permanently");
   public static final HttpStatus FOUND = register(302, "Found");
   public static final HttpStatus SEE_OTHER = register(303, "See Other");
   public static final HttpStatus NOT_MODIFIED = register(304, "Not Modified");
   public static final HttpStatus USE_PROXY = register(305, "Use Proxy");                          // Deprecated
   public static final HttpStatus TEMPORARY_REDIRECT = register(307, "Temporary Redirect");
   public static final HttpStatus PERMANENT_REDIRECT = register(308, "Permanent Redirect");        // RFC 7538

   // ── 4xx Client Error ────────────────────────────────────────────────────
   public static final HttpStatus BAD_REQUEST = register(400, "Bad Request");
   public static final HttpStatus UNAUTHORIZED = register(401, "Unauthorized");
   public static final HttpStatus PAYMENT_REQUIRED = register(402, "Payment Required");
   public static final HttpStatus FORBIDDEN = register(403, "Forbidden");
   public static final HttpStatus NOT_FOUND = register(404, "Not Found");
   public static final HttpStatus METHOD_NOT_ALLOWED = register(405, "Method Not Allowed");
   public static final HttpStatus NOT_ACCEPTABLE = register(406, "Not Acceptable");
   public static final HttpStatus PROXY_AUTHENTICATION_REQUIRED = register(407, "Proxy Authentication Required");
   public static final HttpStatus REQUEST_TIMEOUT = register(408, "Request Timeout");
   public static final HttpStatus CONFLICT = register(409, "Conflict");
   public static final HttpStatus GONE = register(410, "Gone");
   public static final HttpStatus LENGTH_REQUIRED = register(411, "Length Required");
   public static final HttpStatus PRECONDITION_FAILED = register(412, "Precondition Failed");
   public static final HttpStatus CONTENT_TOO_LARGE = register(413, "Content Too Large");          // RFC 9110 (replaces 413 Payload Too Large)
   public static final HttpStatus URI_TOO_LONG = register(414, "URI Too Long");
   public static final HttpStatus UNSUPPORTED_MEDIA_TYPE = register(415, "Unsupported Media Type");
   public static final HttpStatus RANGE_NOT_SATISFIABLE = register(416, "Range Not Satisfiable");
   public static final HttpStatus EXPECTATION_FAILED = register(417, "Expectation Failed");
   public static final HttpStatus IM_A_TEAPOT = register(418, "I'm a Teapot");                     // RFC 2324 — yes, it's real
   public static final HttpStatus MISDIRECTED_REQUEST = register(421, "Misdirected Request");      // RFC 7540
   public static final HttpStatus UNPROCESSABLE_CONTENT = register(422, "Unprocessable Content");  // RFC 9110 (WebDAV / REST validation)
   public static final HttpStatus LOCKED = register(423, "Locked");                                // RFC 4918
   public static final HttpStatus FAILED_DEPENDENCY = register(424, "Failed Dependency");          // RFC 4918
   public static final HttpStatus TOO_EARLY = register(425, "Too Early");                          // RFC 8470
   public static final HttpStatus UPGRADE_REQUIRED = register(426, "Upgrade Required");
   public static final HttpStatus PRECONDITION_REQUIRED = register(428, "Precondition Required");  // RFC 6585
   public static final HttpStatus TOO_MANY_REQUESTS = register(429, "Too Many Requests");          // RFC 6585
   public static final HttpStatus REQUEST_HEADER_FIELDS_TOO_LARGE = register(431, "Request Header Fields Too Large"); // RFC 6585
   public static final HttpStatus UNAVAILABLE_FOR_LEGAL_REASONS = register(451, "Unavailable For Legal Reasons");     // RFC 7725

   // ── 5xx Server Error ────────────────────────────────────────────────────
   public static final HttpStatus INTERNAL_SERVER_ERROR = register(500, "Internal Server Error");
   public static final HttpStatus NOT_IMPLEMENTED = register(501, "Not Implemented");
   public static final HttpStatus BAD_GATEWAY = register(502, "Bad Gateway");
   public static final HttpStatus SERVICE_UNAVAILABLE = register(503, "Service Unavailable");
   public static final HttpStatus GATEWAY_TIMEOUT = register(504, "Gateway Timeout");
   public static final HttpStatus HTTP_VERSION_NOT_SUPPORTED = register(505, "HTTP Version Not Supported");
   public static final HttpStatus VARIANT_ALSO_NEGOTIATES = register(506, "Variant Also Negotiates");   // RFC 2295
   public static final HttpStatus INSUFFICIENT_STORAGE = register(507, "Insufficient Storage");         // RFC 4918
   public static final HttpStatus LOOP_DETECTED = register(508, "Loop Detected");                       // RFC 5842
   public static final HttpStatus NOT_EXTENDED = register(510, "Not Extended");                         // RFC 2774
   public static final HttpStatus NETWORK_AUTHENTICATION_REQUIRED = register(511, "Network Authentication Required"); // RFC 6585

   private final int code;
   private final String reason;
   private final Category category;

   private HttpStatus(int code, String reason)
   {
      this.code = code;
      this.reason = reason;
      this.category = Category.of(code);
   }

   /**
    * Get {@code HttpStatus} for the given numeric code. Returns a well-known instance (with reason phrase) for recognised codes,
    * or a new instance with an empty reason phrase for any other code.
    *
    * @param code numeric HTTP status code
    * @return status instance for the given code (never {@code null})
    */
   public static HttpStatus of(int code)
   {
      HttpStatus status = WELL_KNOWN.get(code);
      return (status != null) ? status : new HttpStatus(code, "");
   }

   /** The numeric status code, e.g. {@code 404}. */
   public int code()
   {
      return code;
   }

   /** The RFC reason phrase (e.g. {@code "Not Found"}), or an empty string for unrecognised codes. */
   public String reason()
   {
      return reason;
   }

   /** True if this code has a well-known reason phrase. */
   public boolean isWellKnown()
   {
      return !reason.isEmpty();
   }

   /** The category this code belongs to. */
   public Category category()
   {
      return category;
   }

   /** True for 4xx and 5xx codes. */
   public boolean isError()
   {
      return category == Category.CLIENT_ERROR || category == Category.SERVER_ERROR;
   }

   /** True for 2xx codes. */
   public boolean isSuccess()
   {
      return category == Category.SUCCESS;
   }

   /** True for 3xx codes. */
   public boolean isRedirection()
   {
      return category == Category.REDIRECTION;
   }

   /**
    * @see java.lang.Object#hashCode()
    */
   @Override
   public int hashCode()
   {
      return code;
   }

   /**
    * @see java.lang.Object#equals(java.lang.Object)
    */
   @Override
   public boolean equals(Object obj)
   {
      return (obj instanceof HttpStatus) && (((HttpStatus)obj).code == code);
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return reason.isEmpty() ? Integer.toString(code) : (code + " " + reason);
   }
}
