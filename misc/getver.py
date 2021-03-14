# Return a pretty-printed short git version (like hg/svnversion)
import os
def cmd(c): return os.popen(c).read().strip()

ver = cmd("echo $MULTIPIMVERSION")
print ver
