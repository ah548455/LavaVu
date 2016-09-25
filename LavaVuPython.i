/*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*
* LavaVu python interface
**~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*/

%module LavaVuPython
%include <std_string.i>
%include <std_vector.i>

%{
#include "src/LavaVu.h"
#include "src/ViewerTypes.h"
%}

%include "exception.i"
%exception {
    try {
        $action
    } catch (const std::runtime_error& e) {
        SWIG_exception(SWIG_RuntimeError, e.what());
    }
}

namespace std {
%template(Line)  vector <float>;
%template(ULine) vector <unsigned int> ;
%template(Array) vector < vector <float> >;
%template(List) vector <string>;
}

%include "src/ViewerTypes.h"

class LavaVu
{
public:
  Model* amodel;
  View* aview;
  DrawingObject* aobject;
  std::string binpath;

  LavaVu(std::string binary="");
  ~LavaVu();

  void run(std::vector<std::string> args={});

  bool loadFile(const std::string& file);
  bool parseCommands(std::string cmd);
  void render();
  void init();
  std::string image(std::string filename="", int width=0, int height=0, bool frame=false);
  std::string web(bool tofile=false);
  void defaultModel();
  void addObject(std::string name, std::string properties="");
  void setObject(std::string name, std::string properties);
  std::string getObject(std::string name);
  int colourMap(std::string name, std::string colours="");
  void setState(std::string state);
  std::string getState();
  std::string getFigures();
  std::string getTimeSteps();
  void loadVectors(std::vector< std::vector <float> > array, lucGeometryDataType type=lucVertexData);
  void loadScalars(std::vector <float> array, lucGeometryDataType type=lucColourValueData, std::string label="", float minimum=0, float maximum=0);
  void loadUnsigned(std::vector <unsigned int> array, lucGeometryDataType type=lucIndexData);
  void labels(std::vector <std::string> labels);
  void close();

};

