/**
 * Shutdown handler
 */

#include "nxcore.h"

#ifdef WITH_ZMQ

#include "zmq.h"

static void *context;
static void *sender;

void ZmqInit()
{
   DbgPrintf(4, _T("ZmqInit: ZMQ init."));
   void *context = zmq_ctx_new ();
   void *sender = zmq_socket (context, ZMQ_PUSH);
   int rc = zmq_bind (sender, "tcp://127.0.0.1:5559");
   if(rc != 0)
      DbgPrintf(1, _T("ZmqInit: sender start failed."));
}

void ZmsStop()
{
   DbgPrintf(1, _T("ZmsStop: ZeroMQ shutdown."));
   zmq_close(sender);
   zmq_ctx_destroy(context);
}

static char *FormatEventToJson(Event *event)
{
   return strdup("Event");
}

static bool ZmqSendEvent(Event *event)
{
   DbgPrintf(4, _T("ZmqInit: Send event."));
   char *msg = FormatEventToJson(event);
   zmq_send(sender, msg, strlen(msg), 0);
   safe_free(msg);
   //push data to server
   return true;
}

/**
 * Test function
 */
void ZmqTest()
{
   ZmqInit();

   void *ct1 = zmq_ctx_new ();
   void *rec = zmq_socket (ct1, ZMQ_PULL);
   int rc = zmq_bind (rec, "tcp://*:5559");
   if(rc != 0)
      DbgPrintf(1, _T("ZmqTest: reciever start failed."));

   ZmqSendEvent(NULL);
   char str[512];
   rc = zmq_recv(rec, str, 512, 0);
   if(rc != 0)
      DbgPrintf(1, _T("ZmqTest: read failed start failed."));
   DbgPrintf(1, _T("ZmqTest: recieved: %hs"), str);

   ZmsStop();
}


#else

void ZmqInit()
{
   DbgPrintf(4, _T("ZmqInit: ZeroMQ is not enabed for your build."));
   //empty function for non ZMQ build
}

void ZmsStop()
{
   DbgPrintf(4, _T("ZmsStop: ZeroMQ is not enabed for your build."));
    //empty function for non ZMQ build
}

#endif // WITH_ZMQ
