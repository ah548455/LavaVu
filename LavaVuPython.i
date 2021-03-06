/*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*
* LavaVu python interface
**~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*/
%module LavaVuPython

%{
#define SWIG_FILE_WITH_INIT
%}

%include <std_string.i>
%include <std_vector.i>
%include <numpy.i>

%init %{
  import_array();
%}

%{
#include "src/LavaVu.h"
#include "src/ViewerTypes.h"
#include "src/DrawingObject.h"
%}

%include "exception.i"
%exception {
  try {
    $action
  } catch (const std::runtime_error& e) {
    SWIG_exception(SWIG_RuntimeError, e.what());
  }
}

%init 
%{
import_array();
%}

namespace std 
{
%template(Line)  vector <float>;
%template(Array) vector <vector <float> >;
%template(List) vector <string>;
}

%apply (unsigned char* IN_ARRAY1, int DIM1) {(unsigned char* array, int len)};
%apply (unsigned int* IN_ARRAY1, int DIM1) {(unsigned int* array, int len)};
%apply (float* IN_ARRAY1, int DIM1) {(float* array, int len)};
%apply (float** ARGOUTVIEW_ARRAY1, int* DIM1) {(float** array, int* len)};
%apply (unsigned char** ARGOUTVIEW_ARRAY1, int* DIM1) {(unsigned char** array, int* len)};
%apply (unsigned int** ARGOUTVIEW_ARRAY1, int* DIM1) {(unsigned int** array, int* len)};
%apply (unsigned char* INPLACE_ARRAY3, int DIM1, int DIM2, int DIM3) {(unsigned char* array, int width, int height, int depth)};

%include "src/ViewerTypes.h"

class OpenGLViewer
{
  public:
    bool quitProgram;
};

class DrawingObject
{
public:
  DrawingObject(DrawState& drawstate, std::string name="", std::string props="", unsigned int id=0);
};

class GeomData
{
public:
  lucGeometryType type;   //Holds the object type
  GeomData(DrawingObject* draw, lucGeometryType type);
};

class LavaVu
{
public:
  OpenGLViewer* viewer;
  Model* amodel;
  View* aview;
  DrawingObject* aobject;
  std::string binpath;

  LavaVu(std::string binpath, bool omegalib=false);
  ~LavaVu();

  void run(std::vector<std::string> args={});

  bool loadFile(const std::string& file);
  bool parseCommands(std::string cmd);
  void render();
  void init();
  std::string image(std::string filename="", int width=0, int height=0, int jpegquality=0, bool transparent=false);
  std::string web(bool tofile=false);
  std::string video(std::string filename, int fps=30, int width=0, int height=0, int start=0, int end=0);
  void defaultModel();
  int colourMap(std::string name, std::string colours="", std::string properties="");
  DrawingObject* colourBar(DrawingObject* obj);
  void setState(std::string state);
  std::string getState();
  std::string getFigures();
  std::string getTimeSteps();

  void resetViews(bool autozoom=false);

  void setObject(DrawingObject* target, std::string properties);
  DrawingObject* createObject(std::string properties);
  DrawingObject* getObject(const std::string& name);
  DrawingObject* getObject(int id=-1);
  void reloadObject(DrawingObject* target);

  void loadTriangles(DrawingObject* target, std::vector< std::vector <float> > array, int split=0);
  void loadColours(DrawingObject* target, std::vector <std::string> list);
  void label(DrawingObject* target, std::vector <std::string> labels);

  void clearObject(DrawingObject* target);
  void clearValues(DrawingObject* target, std::string label="");
  void clearData(DrawingObject* target, lucGeometryDataType type);

  void arrayUChar(DrawingObject* target, unsigned char* array, int len, lucGeometryDataType type=lucRGBData);
  void arrayUInt(DrawingObject* target, unsigned int* array, int len, lucGeometryDataType type=lucRGBAData);
  void arrayFloat(DrawingObject* target, float* array, int len, lucGeometryDataType type=lucVertexData);
  void arrayFloat(DrawingObject* target, float* array, int len, std::string label);
  void textureUChar(DrawingObject* target, unsigned char* array, int len, unsigned int width, unsigned int height, unsigned int channels, bool flip=true);
  void textureUInt(DrawingObject* target, unsigned int* array, int len, unsigned int width, unsigned int height, unsigned int channels, bool flip=true);

  int getGeometryCount(DrawingObject* target);
  GeomData* getGeometry(DrawingObject* target, int index);
  void geometryArrayUChar(GeomData* geom, unsigned char* array, int len, lucGeometryDataType type);
  void geometryArrayUInt(GeomData* geom, unsigned int* array, int len, lucGeometryDataType type);
  void geometryArrayFloat(GeomData* geom, float* array, int len, lucGeometryDataType type);
  void geometryArrayFloat(GeomData* geom, float* array, int len, std::string label);

  void geometryArrayViewFloat(GeomData* geom, lucGeometryDataType dtype, float** array, int* len);
  void geometryArrayViewUInt(GeomData* geom, lucGeometryDataType dtype, unsigned int** array, int* len);
  void geometryArrayViewUChar(GeomData* geom, lucGeometryDataType dtype, unsigned char** array, int* len);

  void imageBuffer(unsigned char* array, int width, int height, int depth);
  std::string imageJPEG(int width, int height, int quality=95);
  std::string imagePNG(int width, int height, int depth);

  void isosurface(DrawingObject* target, DrawingObject* source, bool clearvol=false);
  void update(DrawingObject* target, bool compress=true);
  void update(DrawingObject* target, lucGeometryType type, bool compress=true);

  void close();
  std::vector<float> imageArray(std::string path="", int width=0, int height=0, int channels=3);
  float imageDiff(std::string path1, std::string path2="", int downsample=4);
  void queueCommands(std::string cmds);

};

