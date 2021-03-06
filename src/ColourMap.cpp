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

#include "ColourMap.h"

//Safe log function for scaling
#define LOG10(val) (val > FLT_MIN ? log10(val) : log10(FLT_MIN))

//This can stay global as never actually modified, if it needs to be then move to State
int ColourMap::samples = 4096;

std::ostream & operator<<(std::ostream &os, const ColourVal& cv)
{
  return os << cv.value << " --> " << cv.position << "=" << cv.colour;
}

ColourMap::ColourMap(DrawState& drawstate, std::string name, std::string props)
  : noValues(false), log(false), name(name), properties(drawstate.globals, drawstate.defaults),
    minimum(0), maximum(1), calibrated(false), opaque(true), texture(NULL)
{
  precalc = new Colour[samples];
  background.value = 0xff000000;

  properties.parseSet(props);

  if (properties.has("colours"))
  {
    loadPalette(properties["colours"]);
    //Erase the colour list? Or keep it for export?
    //properties.data.erase("colours");
  }
}

void ColourMap::parse(std::string colourMapString)
{
  noValues = false;
  const char *breakChars = " \t\n;";
  char *charPtr;
  char colourStr[64];
  char *colourString = new char[colourMapString.size()+1];
  strcpy(colourString, colourMapString.c_str());
  char *colourMap_ptr = colourString;
  colours.clear();
  charPtr = strtok(colourMap_ptr, breakChars);
  while (charPtr != NULL)
  {
    float value = 0.0;

    // Parse out value if provided, otherwise assign a default
    // input string: (OPTIONAL-VALUE)colour
    if (sscanf(charPtr, "(%f)%s", &value, colourStr) == 2)
    {
      //Add parsed colour to the map with value
      add(colourStr, value);
    }
    else
    {
      //Add parsed colour to the map
      add(charPtr);
    }

    charPtr = strtok(NULL, breakChars);
  }
  delete[] colourString;

  //Ensure at least two colours
  while (colours.size() < 2) add(0xff000000);
}

void ColourMap::addAt(Colour& colour, float position)
{
  //Adds by position, not "value"
  colours.push_back(ColourVal(colour));
  colours[colours.size()-1].value = HUGE_VAL;
  colours[colours.size()-1].position = position;
  //Flag positions provided
  noValues = true;
}

void ColourMap::add(std::string colour)
{
  Colour c(colour);
  colours.push_back(ColourVal(c));
  //std::cerr << colour << " : " << colours[colours.size()-1] << std::endl;
}

void ColourMap::add(std::string colour, float pvalue)
{
  Colour c(colour);
  colours.push_back(ColourVal(c, pvalue));
}

void ColourMap::add(unsigned int colour)
{
  Colour c;
  c.value = colour;
  colours.push_back(ColourVal(c));
}

void ColourMap::add(unsigned int* colours, int count)
{
  for (int i=0; i<count; i++)
    add(colours[i]);
}

void ColourMap::add(unsigned int colour, float pvalue)
{
  Colour c;
  c.value = colour;
  colours.push_back(ColourVal(c, pvalue));
}

void ColourMap::add(float *components, float pvalue)
{
  Colour colour;
  for (int c=0; c<4; c++)  //Convert components from floats to bytes
    colour.rgba[c] = 255 * components[c];

  add(colour.value, pvalue);
}

void ColourMap::calc()
{
  if (!colours.size()) return;

  //Check for transparency
  opaque = true;
  for (unsigned int i=0; i<colours.size(); i++)
  {
    if (colours[i].colour.a < 255)
    {
      opaque = false;
      break;
    }
  }

  //Precalculate colours
  if (log)
    for (int cv=0; cv<samples; cv++)
      precalc[cv] = get(pow(10, log10(minimum) + range * (float)cv/(samples-1)));
  else
    for (int cv=0; cv<samples; cv++)
      precalc[cv] = get(minimum + range * (float)cv/(samples-1));
}

void ColourMap::calibrate(float min, float max)
{
  //Skip calibration if min/max unchanged
  if (!noValues && calibrated && min == minimum && max == maximum) return;
  //No colours?
  if (colours.size() == 0) return;
  //Skip calibration when locked
  if (properties["locked"]) return;

  if (min == HUGE_VAL) min = max;
  if (max == HUGE_VAL) max = min;

  minimum = min;
  maximum = max;
  log = properties["logscale"];
  if (log)
    range = LOG10(maximum) - LOG10(minimum);
  else
    range = maximum - minimum;
  irange = 1.0 / range;

  //Calculates positions based on field values over range
  if (!noValues)
  {
    colours[0].position = 0;
    colours.back().position = 1;
    colours[0].value = minimum;
    colours.back().value = maximum;

    // Get scaled positions for colours - Scale the values to find colour bar positions [0,1]
    float inc;
    for (unsigned int i=1; i < colours.size()-1; i++)
    {
      // Found an empty value
      if (colours[i].value == HUGE_VAL)
      {
        // Search for next provided value
        //printf("Empty value at %d ", i);
        unsigned int j;
        for (j=i+1; j < colours.size(); j++)
        {
          if (colours[j].value != HUGE_VAL)
          {
            // Scale to get new position, unless at max pos
            if (j < colours.size()-1)
              colours[j].position = scaleValue(colours[j].value);

            //printf(", next value found at %d, ", j);
            inc = (colours[j].position - colours[i - 1].position) / (j - i + 1);
            for (unsigned int k = i; k < j; k++)
            {
              colours[k].position = colours[k-1].position + inc;
              //printf("Interpolating at %d from %f by %f to %f\n", k, colours[k-1].position, inc, colours[k].position);
            }
            break;
          }
        }
        // Continue search from j
        i = j;
      }
      else
        // Value found, scale to get position
        colours[i].position = scaleValue(colours[i].value);
    }
  }

  //Calc values now colours have been added
  calc();

  debug_print("ColourMap %s calibrated min %f, max %f, range %f ==> %d colours\n", name.c_str(), minimum, maximum, range, colours.size());
  //for (int i=0; i < colours.size(); i++)
  //   printf(" colour %d value %f pos %f\n", colours[i].colour, colours[i].value, colours[i].position);
  calibrated = true;
}

//Calibration from set "range" property or geom data set
void ColourMap::calibrate(FloatValues* dataValues)
{
  //Check has range property and is valid
  bool hasRange = properties.has("range");
  float range[2];
  Properties::toArray<float>(properties["range"], range, 2);
  if (range[0] >= range[1]) hasRange = false;

  //Has values and no fixed range, calibrate to data
  if (dataValues && !hasRange)
    calibrate(dataValues->minimum, dataValues->maximum);
  //Otherwise calibrate to fixed range if provided
  else if (hasRange)
    calibrate(range[0], range[1]);
  //Otherwise calibrate with existing values
  else
    calibrate(minimum, maximum);
}

Colour ColourMap::getfast(float value)
{
  //NOTE: value caching DOES NOT WORK for log scales!
  //If this is causing slow downs in future, need a better method
  int c = 0;
  if (log)
    c = (int)((samples-1) * irange * ((LOG10(value) - LOG10(minimum))));
  else
    c = (int)((samples-1) * irange * ((value - minimum)));
  if (c > samples - 1) c = samples - 1;
  if (c < 0) c = 0;
  //std::cerr << value << " range : " << range << " : min " << minimum << ", max " << maximum << ", pos " << c << ", Colour " << precalc[c] << " uncached " << get(value) << std::endl;
  //std::cerr << LOG10(value) << " range : " << range << " : Lmin " << LOG10(minimum) << ", max " << LOG10(maximum) << ", pos " << c << ", Colour " << precalc[c] << " uncached " << get(value) << std::endl;
  return precalc[c];
}

Colour ColourMap::get(float value)
{
  return getFromScaled(scaleValue(value));
}

float ColourMap::scaleValue(float value)
{
  float min = minimum, max = maximum;
  if (max == min) return 0.5;   // Force central value if no range
  if (value <= min) return 0.0;
  if (value >= max) return 1.0;

  if (log)
  {
    value = LOG10(value);
    min = LOG10(minimum);
    max = LOG10(maximum);
  }

  //Scale to range [0,1]
  return (value - min) / (max - min);
}

Colour ColourMap::getFromScaled(float scaledValue)
{
  //printf(" scaled %f ", scaledValue);
  if (colours.size() == 0) return Colour();
  // Check within range
  if (scaledValue >= 1.0)
    return colours.back().colour;
  else if (scaledValue >= 0)
  {
    // Find the colour/values our value lies between
    unsigned int i;
    for (i=0; i<colours.size(); i++)
    {
      if (fabs(colours[i].position - scaledValue) <= FLT_EPSILON)
        return colours[i].colour;

      if (colours[i].position > scaledValue) break;
    }

    if (i==0 || i==colours.size()) 
      abort_program("Colour position %f not in range [%f,%f]", scaledValue, colours[0].position, colours.back().position);

    // Calculate interpolation factor [0,1] between colour at index and previous colour
    float interpolate = (scaledValue - colours[i-1].position) / (colours[i].position - colours[i-1].position);

    //printf(" interpolate %f above %f below %f\n", interpolate, colours[i].position, colours[i-1].position);
    if (properties["discrete"])
    {
      //No interpolation
      if (interpolate < 0.5)
        return colours[i-1].colour;
      else
        return colours[i].colour;
    }
    else
    {
      //Linear interpolation between colours
      Colour colour0, colour1;
      colour0 = colours[i-1].colour;
      colour1 = colours[i].colour;

      for (int c=0; c<4; c++)
        colour0.rgba[c] += (colour1.rgba[c] - colour0.rgba[c]) * interpolate;

      return colour0;
    }
  }

  Colour c;
  c.value = 0;
  return c;
}

#define VERT2D(x, y, swap) swap ? glVertex2f(y, x) : glVertex2f(x, y);
//#define VERT2D(x, y, swap) swap ? glVertex2f(y+0.5, x+0.5) : glVertex2f(x+0.5, y+0.5);
#define RECT2D(x0, y0, x1, y1, s) {glBegin(GL_QUADS); VERT2D(x0, y0, s); VERT2D(x1, y0, s); VERT2D(x1, y1, s); VERT2D(x0, y1, s); glEnd();}

void ColourMap::draw(DrawState& drawstate, Properties& colourbarprops, int startx, int starty, int length, int breadth, Colour& printColour, bool vertical)
{
  glPushAttrib(GL_ENABLE_BIT);
  glDisable(GL_MULTISAMPLE);

  if (!calibrated) calibrate(minimum, maximum);

  // Draw larger background box for border, use font colour
  glColor4ubv(printColour.rgba);
  drawstate.fonts.setFont(colourbarprops);
  int border = colourbarprops["outline"];
  glDisable(GL_CULL_FACE);
  //glColor4ubv(printColour.rgba);
  if (border > 0)
    RECT2D(startx - border, starty - border, startx + length + border, starty + breadth + border, vertical);

  //Draw background (checked)
  Colour colour;
  for (int pixel_J = 0; pixel_J < breadth; pixel_J += 5)
  {
    colour.value = 0xff666666;
    if (pixel_J % 2 == 1) colour.invert();
    for (int pixel_I = 0 ; pixel_I <= length ; pixel_I += 5)
    {
      int end = startx + pixel_I + breadth;
      if (end > startx + length) end = startx + length;
      int endy = starty + pixel_J + 5;
      if (endy > starty + breadth) endy = starty + breadth;

      glColor4ubv(colour.rgba);
      RECT2D(startx + pixel_I, starty + pixel_J, end, endy, vertical);
      colour.invert();
    }
  }

  // Draw Colour Bar
  int count = colours.size();
  int idx, xpos;
  if (colourbarprops["discrete"])
  {
    glShadeModel(GL_FLAT);
    glBegin(GL_QUAD_STRIP);
    xpos = startx;
    VERT2D(xpos, starty, vertical);
    VERT2D(xpos, starty + breadth, vertical);
    for (idx = 1; idx < count+1; idx++)
    {
      int oldx = xpos;
      colour = getFromScaled(colours[idx-1].position);
      glColor4ubv(colour.rgba);
      if (idx == count)
      {
        VERT2D(xpos, starty, vertical);
        VERT2D(xpos, starty + breadth, vertical);
      }
      else
      {
        xpos = startx + length * colours[idx].position;
        VERT2D(oldx + 0.5 * (xpos - oldx), starty, vertical);
        VERT2D(oldx + 0.5 * (xpos - oldx), starty + breadth, vertical);
      }
    }
    glEnd();
    glShadeModel(GL_SMOOTH);
  }
  else
  {
    glShadeModel(GL_SMOOTH);
    glBegin(GL_QUAD_STRIP);
    for (idx = 0; idx < count; idx++)
    {
      colour = getFromScaled(colours[idx].position);
      glColor4ubv(colour.rgba);
      xpos = startx + length * colours[idx].position;
      VERT2D(xpos, starty, vertical);
      VERT2D(xpos, starty + breadth, vertical);
    }
    glEnd();
  }

  //Labels / tick marks
  glColor4ubv(printColour.rgba);
  drawstate.fonts.setFont(colourbarprops);
  float tickValue;
  unsigned int ticks = colourbarprops["ticks"];
  json tickValues = colourbarprops["tickvalues"];
  if (tickValues.size() > ticks) ticks = tickValues.size();
  bool printTicks = colourbarprops["printticks"];
  std::string format = colourbarprops["format"];
  float scaleval = colourbarprops["scalevalue"];

  //Always show at least two ticks on a log scale
  if (log && ticks < 2) ticks = 2;
  // No ticks if no range
  if (minimum == maximum) ticks = 0;
  for (unsigned int i = 0; i < ticks+2; i++)
  {
    /* Get tick value */
    float scaledPos;
    if (i==0)
    {
      /* Start tick */
      tickValue = minimum;
      scaledPos = 0;
      if (minimum == maximum) scaledPos = 0.5;
    }
    else if (i==ticks+1)
    {
      /* End tick */
      tickValue = maximum;
      scaledPos = 1;
      if (minimum == maximum) continue;
    }
    else
    {
      /* Calculate tick position */
      if (i > tickValues.size())  /* No fixed value provided */
      {
        /* First get scaled position 0-1 */
        if (log)
        {
          /* Space ticks based on a logarithmic scale of log(1) to log(11)
             shows non-linearity while keeping ticks spaced apart enough to read labels */
          float tickpos = 1.0f + (float)i * (10.0f / (ticks+1));
          scaledPos = (LOG10(tickpos) / LOG10(11.0f));
        }
        else
          /* Default linear scale evenly spaced ticks */
          scaledPos = (float)i / (ticks+1);

        /* Compute the tick value */
        if (log)
        {
          /* Reverse calc to find tick value at calculated position 0-1: */
          tickValue = LOG10(minimum) + scaledPos
                      * (LOG10(maximum) - LOG10(minimum));
          tickValue = pow( 10.0f, tickValue );
        }
        else
        {
          /* Reverse scale calc and find value of tick at position 0-1 */
          /* (Using even linear scale) */
          tickValue = minimum + scaledPos * (maximum - minimum);
        }
      }
      else
      {
        /* User specified value */
        /* Calculate scaled position from value */
        tickValue = tickValues[i-1];
        scaledPos = scaleValue(tickValue);
      }

      /* Skip middle ticks if not inside range */
      if (scaledPos == 0 || scaledPos == 1) continue;
    }

    /* Calculate pixel position */
    int xpos = startx + length * scaledPos;

    /* Draws the tick */
    int offset = 0;
    if (vertical) offset = breadth+5;
    int ts = starty-5+offset;
    int te = starty+offset;
    //Full breadth ticks at ends
    if (scaledPos != 0.5 && (i==0 || i==ticks+1))
    {
      if (vertical) ts-=breadth; else te+=breadth;
    }
    //Outline tweak
    if (i==0) xpos -= border;
    if (i==ticks+1) xpos += border-1; //-1 tweak or will be offset from edge

    RECT2D(xpos, ts, xpos+1, te, vertical);

    /* Always print end values, print others if flag set */
    char string[20];
    if (printTicks || i == 0 || i == ticks+1)
    {
      /* Apply any scaling factor  and show units on output */
      if (fabs(tickValue) <= FLT_MIN )
        sprintf( string, "0" );
      else
      {
        // For display purpose, scales the printed values if needed
        tickValue = scaleval * tickValue;
        sprintf(string, format.c_str(), tickValue);
      }

      if (drawstate.fonts.charset > FONT_VECTOR)
      {
        if (vertical)
          drawstate.fonts.rasterPrint(starty + breadth + 10, xpos,  string);
        else
          drawstate.fonts.rasterPrint(xpos - (int) (0.5 * (float)drawstate.fonts.rasterPrintWidth(string)),  starty - 10, string);
      }
      else
      {
        glEnable(GL_MULTISAMPLE);
        if (vertical)
          drawstate.fonts.print(starty + breadth + 10, xpos - (int) (0.5 * (float)drawstate.fonts.printWidth("W")),  string);
        else
          drawstate.fonts.print(xpos - (int) (0.5 * (float)drawstate.fonts.printWidth(string)),  starty - 5 - drawstate.fonts.printWidth("W"), string);
        glDisable(GL_MULTISAMPLE);
      }
    }
  }

  glEnable(GL_MULTISAMPLE);
  glPopAttrib();
}

void ColourMap::setComponent(int component_index)
{
  //Clear other components of colours
  for (unsigned int i=0; i<colours.size(); i++)
    for (int c=0; c<3; c++)
      if (c != component_index) colours[i].colour.rgba[c] = 0;
}

void ColourMap::loadTexture(bool repeat)
{
  if (!texture) texture = new TextureData();
  calibrate(0, 1);
  unsigned char paletteData[4*samples];
  Colour col;
  for (int i=0; i<samples; i++)
  {
    col = get(i / (float)(samples-1));
    //if (i%64==0) printf("RGBA %d %d %d %d\n", col.r, col.g, col.b, col.a);
    paletteData[i*4] = col.r;
    paletteData[i*4+1] = col.g;
    paletteData[i*4+2] = col.b;
    paletteData[i*4+3] = col.a;
  }

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture->id);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, samples, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, paletteData);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  if (repeat)
  {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  }
  else
  {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }
  glBindTexture(GL_TEXTURE_2D, 0);
}

void ColourMap::loadPalette(std::string data)
{
  //Two types of palette data accepted
  // - space separated colour list (process with parse())
  // - position=rgba colour palette (process here)
  if (data.find("=") == std::string::npos)
  {
     parse(data);
     return;
  }

  //Currently only support loading palettes with literal position data, not values to scale
  noValues = true;
  //Parse palette string into key/value pairs
  std::replace(data.begin(), data.end(), ';', '\n'); //Allow semi-colon separators
  std::stringstream is(data);
  colours.clear();
  std::string line;
  while(std::getline(is, line))
  {
    std::istringstream iss(line);
    float pos;
    char delim;
    std::string value;
    if (iss >> pos && pos >= 0.0 && pos <= 1.0)
    {
      iss >> delim;
      std::getline(iss, value); //Read rest of stream into value
      Colour colour(value);
      //Add to colourmap
      addAt(colour, pos);
    }
    else
    {
      //Background?
      std::size_t pos = line.find("=") + 1;
      if (line.substr(0, pos) == "Background=")
      {
        Colour c(line.substr(pos));
        background = c;
      }
    }
  }

  if (texture)
  {
    loadTexture();
    //delete texture;
    //texture = NULL;
  }

  //Ensure at least two colours
  while (colours.size() < 2) add(0xff000000);
}

void ColourMap::print()
{
  for (unsigned int idx = 0; idx < colours.size(); idx++)
  {
    //Colour colour = getFromScaled(colours[idx].position);
    std::cout << idx << " : " << colours[idx] << std::endl;
  }
}

