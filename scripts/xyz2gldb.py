import sys
import re
import lavavu

if len(sys.argv) < 3:
  print "Convert text xyz to LavaVu database"
  print "(basic comma/space/tab delimited X,Y,Z[,R,G,B] only)"
  print ""
  print "  Usage: "
  print ""
  print "    xyz2gldb.py input.txt output.gldb [subsample]"
  print ""
  print "      subsample: N, use every N'th point, skip others"
  print ""
  exit()

filePath = sys.argv[1] #cmds.fileDialog()
dbPath = sys.argv[2]
#Optional subsample arg
subsample = 0
if len(sys.argv) > 3:
  subsample = int(sys.argv[3])

#Create vis object (points)
points = lavavu.Points('points', None, size=5, props="colour=white")
#points = lavavu.Points('points', None, size=5, pointtype=lavavu.Points.ShinySphere, props="colour=white\ntexturefile=shiny.png")

#Create vis database for output
db = lavavu.Database(dbPath)

#Create a new timestep entry in database
db.timestep()

#Loop over lines in input file
count = 0
with open(filePath, 'r') as file:
  for line in file:
    #Subsample?
    count += 1
    if subsample > 1 and count % subsample != 1: continue

    if count % 10000 == 0:
      print count
      sys.stdout.flush()

    #Read particle position
    data = re.split(r'[,;\s]+', line.rstrip())
    #print data
    if len(data) < 3: continue
    x = float(data[0])
    y = float(data[1])
    z = float(data[2])
    
    #Write vertex position, Points...
    points.addVertex(x, y, z)

    #R,G,B[,A] colour if provided
    if len(data) >= 7:
      rgba = [int(data[3]), int(data[4]), int(data[5]), int(data[6])]
      points.addColour(rgba)
    elif len(data) == 6:
      rgba = [int(data[3]), int(data[4]), int(data[5]), 255]
      points.addColour(rgba)

  #Write saved data
  points.write(db)

  #Close and write database to disk
  db.close()


