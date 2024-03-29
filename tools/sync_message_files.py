#!/usr/bin/env python3

import sys
import os
import collections

locales = ['ar', 'cs', 'de', 'es', 'fr', 'iw', 'pt_BR', 'ru']

def readPropertyFile(name):
    print("Reading message file %s" % (name))
    data = {}
    try:
        f = open(name)
        for line in f.readlines():
            line = line.rstrip("\r\n")
            s = line.split("=", 1)
            if len(s) == 2:
                data[s[0].strip()] = s[1]
    except IOError:
        data = {}
    return data

def processDirectory(directory, prefix):
    source = readPropertyFile(os.path.join(directory, "%s.properties" % (prefix, )))
    for locale in locales:
        translationFileName = os.path.join(directory, "%s_%s.properties" % (prefix, locale, ))
        translation = readPropertyFile(translationFileName)
        # hack
        if prefix == 'messages':
            filteredTranslation = {}
            for e in translation.items():
                filteredTranslation[e[0].replace('.', '_')] = e[1]
            translation = filteredTranslation
        for k in set(translation.keys()) - set(source.keys()):
            del translation[k]
        for k in set(source.keys()) - set(translation.keys()):
            translation[k] = source[k]
        translation = collections.OrderedDict(sorted(translation.items()))
        f = open(translationFileName, 'w')
        for key in translation:
            f.write("%s=%s\n" % (key, translation[key]))
        f.close()

def main():
    for (root, dirs, files) in os.walk(sys.argv[1]):
        if '.svn' in dirs:
            dirs.remove('.svn')
        if "messages.properties" in files:
            processDirectory(root, "messages")
        if "bundle.properties" in files:
            processDirectory(root, "bundle")

if len(sys.argv) == 2:
    main()
else:
    print("Usage: ./sync_message_files.py <source root>")
