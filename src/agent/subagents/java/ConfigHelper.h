/* 
 ** Java-Bridge NetXMS subagent
 ** Copyright (C) 2013 TEMPEST a.s.
 **
 ** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation; either version 2 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program; if not, write to the Free Software
 ** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 **
 ** File: ConfigHelper.hxx
 **
 **/

/* This is a helper class providing factory to instantiate Java org.netxms.agent.Config objects from native Config *
 ** It also contains implementation of native methods
 **/

#ifndef __ORG_NETXMS_AGENT_CONFIG_HELPER__
#define __ORG_NETXMS_AGENT_CONFIG_HELPER__

namespace org_netxms_agent
{
   class ConfigHelper
   {

      private:

      protected:

      public:
         // Factory
         /**
          * Create a wrapping of the object from a JNIEnv.
          * It will call the constructor
          * @param JEnv_ the Java Env
          * @param Config * pointer to native object
          */
         static jobject createInstance(JNIEnv *jniEnv, Config *config);

         // Methods

   };

}
#endif
