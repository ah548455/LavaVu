
#Viewer utils
import control

enabled = True
viewer = None

try:
    import sys
    import os
    sys.path.append(os.path.join(os.path.dirname(control.__file__), 'bin'))
    from LavaVuPython import *
    viewer = load()
except:
    print "WARNING: LavaVu not found, inline visualisation disabled"
    enabled = False


