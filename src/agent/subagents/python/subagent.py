# SubAgent code for Python side

import netxms

__parameters = {}
__lists = {}
__tables = {}
__actions = {}

def __add_plugin_parameter(name, options, f):
   __parameters[name] = (f, options.get('type', netxms.agent.DCI_DT_STRING), options.get('description', ''))

def __add_plugin_list(name, options, f):
   __lists[name] = (f, options.get('description', ''))

def __add_plugin_table(name, options, f):
   __tables[name] = (f, options.get('description', ''))

def __add_plugin_action(name, options, f):
   __actions[name] = (f, options.get('description', ''))

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
      p = (k, v[1], v[2])
      result.append(p)
   return result
