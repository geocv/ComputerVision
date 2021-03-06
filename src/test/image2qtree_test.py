#!/usr/bin/env python
# __BEGIN_LICENSE__
# Copyright (C) 2006-2011 United States Government as represented by
# the Administrator of the National Aeronautics and Space Administration.
# All Rights Reserved.
# __END_LICENSE__


import os
import os.path as P
import subprocess

TEST_DIR  = P.abspath(P.dirname(__file__))
TOOLS_DIR = P.abspath(P.join(TEST_DIR, '..', 'vw', 'tools'))

BUILD_DIR = P.join(TEST_DIR, 'image2qtree.out')
QTREE     = P.join(TOOLS_DIR, 'image2qtree')

if not os.path.exists(BUILD_DIR):
    os.mkdir(BUILD_DIR)

def run(*args, **kw):
    print 'Running: %s' % ' '.join(args)
    subprocess.check_call(args, cwd=kw.get('cwd', None))

run(P.join(TEST_DIR, 'geotif-generate'), cwd=BUILD_DIR)

# Create the image2qtree results
files = os.listdir(BUILD_DIR)
for i in files:
    run(QTREE, '-m', 'kml', i, cwd=BUILD_DIR)

# Create master kml
with file(P.join(BUILD_DIR, 'master.kml'), 'w') as mkml:
    mkml.write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
    mkml.write("<kml xmlns=\"http://earth.google.com/kml/2.2\">\n")
    mkml.write("<Document>\n")
    mkml.write("  <name>ListStyle radiofolder</name>\n")
    mkml.write("  <Folder>\n")
    mkml.write("    <name>Image2qtree results</name>\n")
    mkml.write("    <Style>\n")
    mkml.write("      <ListStyle>\n")
    mkml.write("        <listItemType>radioFolder</listItemType>\n")
    mkml.write("      </ListStyle>\n")
    mkml.write("    </Style>\n")

    for i in files:
        i = P.basename(i)
        prefix = i[:-4]
        mkml.write("    <NetworkLink>\n")
        mkml.write("      <name>%s</name><refreshVisibility>1</refreshVisibility>\n" % i)
        mkml.write("      <Link><href>%s/%s.kml</href></Link>\n" % (prefix, prefix))
        mkml.write("    </NetworkLink>\n")

    mkml.write("  </Folder>\n")
    mkml.write("</Document>\n")
    mkml.write("</kml>\n")
