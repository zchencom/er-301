#!/usr/bin/env python3
"""Add the 'sun' screensaver to an ER-301 (stolmine) source tree.
Usage: python3 apply_sun.py <er-301 source dir> <dir containing Sun.h/Sun.cpp>
"""
import os, shutil, sys

src, files = sys.argv[1], sys.argv[2]
ss = os.path.join(src, 'od/graphics/screensavers')
for f in ('Sun.h', 'Sun.cpp', 'Suns.h', 'Suns.cpp', 'NameBitmap.h'):
    shutil.copy(os.path.join(files, f), os.path.join(ss, f))

def edit(path, pairs):
    p = os.path.join(src, path)
    s = open(p, newline='').read()
    nl = '\r\n' if '\r\n' in s else '\n'
    for old, new in pairs:
        old, new = old.replace('\n', nl), new.replace('\n', nl)
        if new in s:
            continue
        assert s.count(old) == 1, f'{path}: anchor not found exactly once: {old!r}'
        s = s.replace(old, new)
    open(p, 'w', newline='').write(s)

edit('od/UIThread.cpp', [
    ('#include <od/graphics/screensavers/Bubbles.h>\n',
     '#include <od/graphics/screensavers/Bubbles.h>\n#include <od/graphics/screensavers/Sun.h>\n#include <od/graphics/screensavers/Suns.h>\n'),
    ('"perlin", "voronoi"};', '"perlin", "voronoi", "sun"};'),
    ('''    else if (tmp == "doom")
    {
      local->screenSaver = new Doom();
    }
''', '''    else if (tmp == "doom")
    {
      local->screenSaver = new Doom();
    }
    else if (tmp == "sun")
    {
      local->screenSaver = new Sun();
    }
    else if (tmp == "suns")
    {
      local->screenSaver = new Suns();
    }
'''),
])
edit('xroot/Settings/init.lua', [
    ('      "doom",\n      "cycle"\n', '      "doom",\n      "sun",\n      "suns",\n      "cycle"\n'),
])
print('sun + suns screensavers applied')
