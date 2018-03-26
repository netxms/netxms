# SubAgent code for Python side

import netxms

__parameters = {}
__lists = {}
__tables = {}
__actions = {}
__nextId = 1

def __generate_id():
   global __nextId
   handlerId = "PySubAgent_" + str(__nextId)
   __nextId = __nextId + 1
   return handlerId

def __add_plugin_parameter(name, options, f):
   __parameters[__generate_id()] = (name, f, options.get('type', netxms.agent.DCI_DT_STRING), options.get('description', ''))

def __add_plugin_list(name, options, f):
   __lists[__generate_id()] = (name, f, options.get('description', ''))

def __add_plugin_table(name, options, f):
   __tables[__generate_id()] = (name, f, options.get('description', ''))

def __add_plugin_action(name, options, f):
   __actions[__generate_id()] = (name, f, options.get('description', ''))

def ParameterHandler(name, **options):
   def decorator(f):
      __add_plugin_parameter(name, options, f)
      return f
   return decorator

def ListHandler(name, **options):
   def decorator(f):
      __add_plugin_list(name, options, f)
      return f
   return decorator

def TableHandler(name, **options):
   def decorator(f):
      __add_plugin_table(name, options, f)
      return f
   return decorator

def ActionHandler(name, **options):
   def decorator(f):
      __add_plugin_action(name, options, f)
      return f
   return decorator

def SubAgent_GetParameters():
   result = []
   for k in __parameters:
      v = __parameters[k]
      p = (k, v[0], v[2], v[3])
      result.append(p)
   return result

def SubAgent_GetLists():
   result = []
   for k in __lists:
      v = __lists[k]
      p = (k, v[0], v[2])
      result.append(p)
   return result

def SubAgent_CallParameterHandler(handlerId, name):
   p = __parameters[handlerId]
   return p[1](name)

def SubAgent_CallListHandler(handlerId, name):
   p = __lists[handlerId]
   return p[1](name)
