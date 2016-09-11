#ifndef State__
#define State__

#include "DrawState.h"
#include "Geometry.h"
#include "OutputInterface.h"
#include "InputInterface.h"

//Class to hold all global state
class State
{
public:
//DrawState
  DrawState drawstate;

//Model
  int now;
  //Current timestep geometry
  std::vector<Geometry*> geometry;
  //Type specific geometry pointers
  Geometry* labels;
  Points* points;
  Vectors* vectors;
  Tracers* tracers;
  QuadSurfaces* quadSurfaces;
  TriSurfaces* triSurfaces;
  Lines* lines;
  Shapes* shapes;
  Volumes* volumes;

//TimeStep
  //std::vector<TimeStep*> timesteps; //Active model timesteps
  //int gap;
  std::vector<Geometry*> fixed;     //Static geometry
  int cachesize;

//Shaders
  std::string path;

//OpenGLViewer
  int idle;
  int displayidle; //Redisplay when idle for # milliseconds
  std::vector<OutputInterface*> outputs; //Additional output attachments
  std::vector<InputInterface*> inputs; //Additional input attachments
  std::deque<std::string> commands;
  pthread_mutex_t cmd_mutex;

  State()
  {
    //Active geometry containers, shared by all models for fast switching/drawing
    labels = NULL;
    points = NULL;
    vectors = NULL;
    tracers = NULL;
    quadSurfaces = NULL;
    triSurfaces = NULL;
    lines = NULL;
    shapes = NULL;
    volumes = NULL;

    //timesteps; //Active model timesteps
    //gap = 0;
    cachesize = 0;
    now = -1;

    path = "";
#ifdef SHADER_PATH
    path = SHADER_PATH;
#endif

    idle = 0;
    displayidle = 0;
    /* Init mutex */
    pthread_mutex_init(&cmd_mutex, NULL);
  }

  void loadFixed()
  {
    //Insert fixed geometry records
    if (fixed.size() > 0) 
      for (unsigned int i=0; i<geometry.size(); i++)
        geometry[i]->insertFixed(fixed[i]);
  }
};

#endif // State__

