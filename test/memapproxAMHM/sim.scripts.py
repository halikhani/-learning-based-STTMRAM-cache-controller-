import sys
sys.argv = [ "/home/amir/sniper-mem/scripts/periodic-stats.py", "1000:2000" ]
execfile("/home/amir/sniper-mem/scripts/periodic-stats.py")
sys.argv = [ "/home/amir/sniper-mem/scripts/markers.py", "markers" ]
execfile("/home/amir/sniper-mem/scripts/markers.py")
