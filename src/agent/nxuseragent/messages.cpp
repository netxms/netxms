#include "nxuseragent.h"

/**
 * List of active user agent messages
 */
static HashMap<ServerObjectKey, UserAgentMessage> s_userMessages(true);
static Mutex s_userMessageLock;

/**
 * Update messages
 */
void UpdateUserMessages(NXCPMessage *request)
{
   s_userMessageLock.lock();

   s_userMessages.clear();
   int count = request->getFieldAsInt32(VID_USER_AGENT_MESSAGE_COUNT);
   int base = VID_USER_AGENT_MESSAGE_BASE;
   for (int i = 0; i < count; i++, base += 10)
   {
      UserAgentMessage *uam = new UserAgentMessage(request, base);
      s_userMessages.set(uam->getId(), uam);
      nxlog_debug(7, _T("User message [%u]: %s"), uam->getId().objectId, uam->getMessage());
   }

   s_userMessageLock.unlock();
}
