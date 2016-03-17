/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
** Copyright (c) 2010, Monash University
** All rights reserved.
** Redistribution and use in source and binary forms, with or without modification,
** are permitted provided that the following conditions are met:
**
**       * Redistributions of source code must retain the above copyright notice,
**          this list of conditions and the following disclaimer.
**       * Redistributions in binary form must reproduce the above copyright
**         notice, this list of conditions and the following disclaimer in the
**         documentation and/or other materials provided with the distribution.
**       * Neither the name of the Monash University nor the names of its contributors
**         may be used to endorse or promote products derived from this software
**         without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
** THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
** PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
** BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
** HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
** OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
**
** Contact:
*%  Owen Kaluza - Owen.Kaluza(at)monash.edu
*%
*% Development Team :
*%  http://www.underworldproject.org/aboutus.html
**
**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include "Util.h"
#include <string.h>
#include <math.h>

json Properties::globals;
json Properties::defaults;

FILE* infostream = NULL;

long membytes__ = 0;
long mempeak__ = 0;

bool FileExists(const std::string& name)
{
  FILE *file = fopen(name.c_str(), "r");
  if (file)
  {
    fclose(file);
    return true;
  }
  return false;
}

std::string GetBinaryPath(const char* argv0, const char* progname)
{
  //Try the PATH env var if argv0 contains no path info
  FilePath xpath;
  if (!argv0 || strlen(argv0) == 0 || strcmp(argv0, progname) == 0)
  {
    std::stringstream path(getenv("PATH"));
    std::string line;
    while (std::getline(path, line, ':'))
    {
      std::stringstream pathentry;
      pathentry << line << "/" << argv0;
      std::string pstr = pathentry.str();
      const char* pathstr = pstr.c_str();
#ifdef _WIN32
      if (strstr(pathstr, ".exe"))
#else
      if (access(pathstr, X_OK) == 0)
#endif
      {
        xpath.parse(pathstr);
        break;
      }
    }
  }
  else
  {
    xpath.parse(argv0);
  }
  return xpath.path;
}

json& Properties::global(const std::string& key)
{
  if (globals.count(key) > 0 && !globals[key].is_null()) return globals[key];
  return defaults[key];
}

bool Properties::has(const std::string& key) {return data.count(key) > 0 && !data[key].is_null();}

json& Properties::operator[](const std::string& key)
{
  //std::cout << key << std::endl;
  json& val = defaults["default"];
  try
  {
    if (data.count(key)) val = data[key];
    else if (globals.count(key)) val = globals[key];
    //std::cout << key << " :: DATA\n" << data << std::endl;
    //std::cout << key << " :: DEFAULTS\n" << defaults << std::endl;
    else val = defaults[key];
    //return defaults[key];
  }
  catch (std::exception& e)
  //catch (std::domain_error& e)
  {
    std::cerr << key << " : key error : " << e.what() << std::endl;
    //std::cerr << key << " : key error : " << std::endl;
  }
  //return defaults["default"];
  return val;
}

//Functions to get values with provided defaults
Colour Properties::getColour(const std::string& key, unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha)
{
  Colour colour = {red, green, blue, alpha};
  if (data.count(key) == 0) return colour;
  return Colour_FromJson(data[key], red, green, blue, alpha);
}

float Properties::getFloat(const std::string& key, float def)
{
  if (data.count(key) == 0) return def;
  return data[key];
}

int Properties::getInt(const std::string& key, int def)
{
  if (data.count(key) == 0) return def;
  return data[key];
}

bool Properties::getBool(const std::string& key, bool def)
{
  if (data.count(key) == 0) return def;
  return data[key];
}

//Parse multi-line string
void Properties::parseSet(const std::string& properties)
{
  //Process all lines
  std::stringstream ss(properties);
  std::string line;
  while (std::getline(ss, line))
    parse(line);
}

//Property containers now using json
void Properties::parse(const std::string& property, bool global)
{
  //Parse a key=value property where value is a json object
  json& dest = global ? globals : data; //Parse into data by default
  std::string key, value;
  std::istringstream iss(property);
  std::getline(iss, key, '=');
  std::getline(iss, value, '=');

  if (value.length() > 0)
  {
    //Ignore case
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    //std::cerr << "Key " << key << " == " << value << std::endl;
    std::string valuel = value;
    std::transform(valuel.begin(), valuel.end(), valuel.begin(), ::tolower);
    
    try
    {
      //Parse simple increments and decrements
      int end = key.length()-1;
      char prev = key.at(end);
      if (prev == '+' || prev == '-')
      {
        std::string mkey = key.substr(0,end);
        std::stringstream ss(value);
        float parsedval;
        ss >> parsedval;
        float val = dest[mkey];
        if (prev == '+')
          dest[mkey] = val + parsedval;
        else if (prev == '-')
          dest[mkey] = val - parsedval;

      }
      else if (valuel == "true")
      {
        dest[key] = true;
      }
      else if (valuel == "false")
      {
        dest[key] = false;
      }
      else
      {
        dest[key] = json::parse(value);
      }
    }
    catch (std::exception& e)
    {
      //std::cerr << "[" << key << "] " << data << " : " << e.what();
      //Treat as a string value
      dest[key] = value;
    }
  }
}

void Properties::merge(json& other)
{
  //Merge: keep existing values, replace any imported
  for (json::iterator it = other.begin() ; it != other.end(); ++it)
    data[it.key()] = it.value();
}

void Properties::convertBools(std::vector<std::string> list)
{
  //Converts a list of properties from int to boolean
  for (int i=0; i<list.size(); i++)
  {
    if (has(list[i]) && data[list[i]].is_number())
      data[list[i]] = ((int)data[list[i]] != 0);
  }
}

void debug_print(const char *fmt, ...)
{
  if (fmt == NULL || infostream == NULL) return;
  va_list args;
  va_start(args, fmt);
  //vprintf(fmt, args);
  vfprintf(infostream, fmt, args);
  //debug_print("\n");
  va_end(args);
}

