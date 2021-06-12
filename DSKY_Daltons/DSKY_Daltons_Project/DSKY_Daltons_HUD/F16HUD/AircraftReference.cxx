#include <GL/glut.h>
#include <iostream>
#include <math.h>
#include <cstdio>

#ifndef M_PI
#define M_PI 3.1415926535
#endif

#include "AircraftReference.hxx"

using namespace std;

AircraftReference::AircraftReference(double height, double vfov, double width):
   vert_fov      (  vfov),     // deg
   screenheight  (height),
   screenwidth   (width),
   x_screenpos   (   0.0),
   y_screenpos   (   0.0),
   lineWidth     (   1.0),
   theta0        (   0.0),     // rad
   _veh_yaw      (   0.0),
   _x_range      ( 1000.0),
   _y_range      ( 1000.0),
   nz            (   1.0),     // g's
   nz_max        (   1.0),     // g's
   steerpointno  (   1  ),
   steerpointdist(7408.0),     // m
   ETE           ( 100.0)      // seconds
{
   lineColor[0] = 0.0;
   lineColor[1] = 1.0;
   lineColor[2] = 0.0;
   cout << "AircraftReference: constructor called" << endl;
}


AircraftReference::~AircraftReference()
{
   cout << "AircraftReference: destructor called" << endl;
}


void AircraftReference::SetNz(double nz_g, bool sat)
{
   nz = nz_g;
   _sat = sat;
};

void AircraftReference::DrawDiamond(Eigen::Vector3f xyz, int i) {

   double y = screenwidth*clamp(xyz(1)/_y_range, -0.95, 0.95);
   double x = screenheight*clamp(xyz(0)/_x_range, -0.95, 0.95);

   double size = screenheight * 20.0 / _x_range;
   char temp[2];

   glPushMatrix();
      glRotatef(Rad2Deg(_veh_yaw), 0, 0, 1);
      glTranslatef(y, x, 0.0);
      glRotatef(-Rad2Deg(_veh_yaw), 0, 0, 1);
      glLineWidth(lineWidth);
      glColor3fv(lineColor);
      glBegin(GL_LINES);
         glVertex2f( -size,   0.0);
         glVertex2f(   0.0, -size);
         glVertex2f(   0.0, -size);
         glVertex2f(  size,   0.0);
         glVertex2f(  size,   0.0);
         glVertex2f(   0.0,  size);
         glVertex2f(   0.0,  size);
         glVertex2f( -size,   0.0);
      glEnd();
      if (i<7) {
         snprintf(temp, 2, "%d", (int) i );
      } else {
         snprintf(temp, 2, "F");
      }
      strokeString(-4.0, -4.0, temp, GLUT_STROKE_ROMAN, 0.10);
   glPopMatrix();

}

void AircraftReference::DrawDot(Eigen::Vector3f xyz, int i) {

   double y = screenwidth*clamp(xyz(1)/_y_range, -0.95, 0.95);
   double x = screenheight*clamp(xyz(0)/_x_range, -0.95, 0.95);

   double size = 0.5*0.03*screenheight;
   char temp[2];

   glPushMatrix();
      glRotatef(Rad2Deg(_veh_yaw), 0, 0, 1);
      glTranslatef(y, x, 0.0);
      glRotatef(-Rad2Deg(_veh_yaw), 0, 0, 1);
      glLineWidth(lineWidth);
      glColor3fv(lineColor);
      glBegin(GL_LINES);
         glVertex2f( -size,  size);
         glVertex2f(  size,  size);
         glVertex2f(  size,  size);
         glVertex2f(  size,  -size);
         glVertex2f(  size,  -size);
         glVertex2f(  -size, -size);
         glVertex2f(  -size, -size);
         glVertex2f( -size,  size);
      glEnd();
      snprintf(temp, 2, "%d", (int) i );
      strokeString(-4, -4, temp, GLUT_STROKE_ROMAN, 0.10);
   glPopMatrix();

}

void AircraftReference::DrawGL()
{
   char temp[10];
   double distToScreen = (screenheight/2.0) / tan(Deg2Rad(vert_fov)/2.0);

   // enable blending
   glEnable(GL_BLEND);
   glEnable(GL_LINE_SMOOTH);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   glPushMatrix();
      glTranslatef(x_screenpos, y_screenpos, 0.0);

      // gun cross
      glPushMatrix();
         glTranslatef(0.0, distToScreen * tan(theta0), 0.0);
         glLineWidth(lineWidth);
         glColor3fv(lineColor);
         glBegin(GL_LINES);
            glVertex2f( -11.0,   0.0);
            glVertex2f( -22.0,   0.0);
            glVertex2f(  11.0,   0.0);
            glVertex2f(  22.0,   0.0);
            glVertex2f(   0.0, -11.0);
            glVertex2f(   0.0, -22.0);
            glVertex2f(   0.0,  11.0);
            glVertex2f(   0.0,  22.0);
         glEnd();
      glPopMatrix();

      // landing sites
      int i = 1;
      for (auto const& sdiff: _sitesdiff) {
         DrawDiamond(sdiff, i++);
      }

      // other teams
      i = 1;
      for (auto const& gdiff: _groupsdiff) {
         DrawDot(gdiff, i++);
      }

      // speed vector
      glPushMatrix();
         glRotatef(Rad2Deg(_veh_yaw), 0, 0, 1);
         glLineWidth(3*lineWidth);
         glColor3fv(lineColor);
         glBegin(GL_LINES);
            glVertex2f( 0,  0);
            glVertex2f( 0.7*size_y/_y_range * _PI_proj(1),  0.7*size_x/_y_range * _PI_proj(0));
         glEnd();
      glPopMatrix();


      // nz indicator
      if (!_sat) {
         glColor3fv(lineColor);
      } else {
         GLfloat satColor[3];
         satColor[0] =  1.0;
         satColor[1] =  0.0;
         satColor[2] =  0.0;
         glColor3fv(satColor);
      }
      double mod = -nz/1000.0;
      glLineWidth(lineWidth);
      snprintf(temp, 10, "%1.1fkN", mod);
      strokeString(-339, 175.0, temp, GLUT_STROKE_ROMAN, 0.15);

      // THRUST
      glColor3fv(lineColor);
      glLineWidth(lineWidth);
      snprintf(temp, 10, "THRUST");
      strokeString(-339, 195, temp, GLUT_STROKE_ROMAN, 0.15);

      // FUEL MASS
      glColor3fv(lineColor);
      glLineWidth(lineWidth);
      snprintf(temp, 10, "%5.0fkg", _fuel);
      strokeString(-339, 155, temp, GLUT_STROKE_ROMAN, 0.15);

      // // nz_max indicator
      // glColor3fv(lineColor);
      // glLineWidth(lineWidth);
      // snprintf(temp, 10, "%1.1f", nz_max);
      // strokeString(-379, -235.0, temp, GLUT_STROKE_ROMAN, 0.15);

      // NAV indicator
      glColor3fv(lineColor);
      glLineWidth(lineWidth);
      snprintf(temp, 10, "NAV");
      strokeString(-379, -275.0, temp, GLUT_STROKE_ROMAN, 0.15);

      // Ref. announcment
      glColor3fv(lineColor);
      glLineWidth(lineWidth);
      char * c = (char *) _message.c_str(); // converts to the required char pointer
      snprintf(temp, 10, "MESSAGE");
      strokeString(-379, 235, c, GLUT_STROKE_ROMAN, 0.15);

      // leaderboard
      glColor3fv(lineColor);
      glLineWidth(lineWidth);
      char scores[38];
      snprintf(scores, 38, "G1: %7d  G2: %7d  G3: %7d", _g1, _g2, _g3);
      strokeString(-379, 255, scores, GLUT_STROKE_ROMAN, 0.15);

      // // Slant range
      // glColor3fv(lineColor);
      // glLineWidth(lineWidth);
      // snprintf(temp, 10, "B%05.1f", (steerpointdist/1852.0));
      // strokeString(259, -275.0, temp, GLUT_STROKE_ROMAN, 0.15);

      // // ETE
      // glColor3fv(lineColor);
      // glLineWidth(lineWidth);
      // char minussign = ' ';
      // double secs = 0.0;
      // double mins = 0.0;
      // if (ETE < 0.0)
      // {
      //    minussign = '-';
      //    secs = floor(fmod(ETE, 60.0));
      //    mins = (ETE-secs)/60.0;
      //    secs = -secs;
      // }
      // else
      // {
      //    secs = floor(fmod(ETE, 60.0));
      //    mins = (ETE-secs)/60.0;
      // }
      // snprintf(temp, 10, "%c%02.0f:%02.0f", minussign, mins, secs);
      // strokeString(259, -315.0, temp, GLUT_STROKE_ROMAN, 0.15);

      // // dist to steerpoint > steerpoint no
      // glColor3fv(lineColor);
      // glLineWidth(lineWidth);
      // snprintf(temp, 10, "%03.0f>%02d", (steerpointdist/1852.0), steerpointno);
      // strokeString(259, -355.0, temp, GLUT_STROKE_ROMAN, 0.15);

   glPopMatrix();

   glDisable(GL_BLEND);
   glDisable(GL_LINE_SMOOTH);
}


void AircraftReference::strokeString(double x, double y, char *msg, void *font, double size)
{
   glPushMatrix();
      GLboolean glBlendEnabled = glIsEnabled(GL_BLEND);
      GLboolean glLineSmoothEnabled = glIsEnabled(GL_LINE_SMOOTH);

      glTranslatef(x, y, 0.0);
      glScalef(size, size, size);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glEnable(GL_BLEND);
      glEnable(GL_LINE_SMOOTH);
      glLineWidth(1.5*lineWidth);
      while (*msg)
      {
         glutStrokeCharacter(font, *msg);
         msg++;
      }

      if (glBlendEnabled == GL_FALSE)
      {
         glDisable(GL_BLEND);
      }
      if (glLineSmoothEnabled == GL_FALSE)
      {
         glDisable(GL_LINE_SMOOTH);
      }

      glLineWidth(1.0*lineWidth);
   glPopMatrix();
}

void AircraftReference::strokeString(double x, double y, char *msg, void *font, double size, int align)
{
   glPushMatrix();
      GLboolean glBlendEnabled = glIsEnabled(GL_BLEND);
      GLboolean glLineSmoothEnabled = glIsEnabled(GL_LINE_SMOOTH);

      glTranslatef(x, y, 0);
      glScalef(size, size, size);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glEnable(GL_BLEND);
      glEnable(GL_LINE_SMOOTH);
      glLineWidth(1.5*lineWidth);

      char *msgtwo = msg;
      float textwidth = 0.0;
      while (*msgtwo)
      {
         textwidth = textwidth + 80.0;
         msgtwo++;
      }
      glTranslatef(-textwidth, 0.0, 0.0);

      float charwidth = 0.0;
      while (*msg)
      {
         charwidth = glutStrokeWidth(font, (*msg));
         glutStrokeCharacter(font, *msg);
         glTranslatef(-charwidth+80, 0.0, 0.0);
         msg++;
      }

      if (glBlendEnabled == GL_FALSE)
      {
         glDisable(GL_BLEND);
      }
      if (glLineSmoothEnabled == GL_FALSE)
      {
         glDisable(GL_LINE_SMOOTH);
      }
      glLineWidth(lineWidth);
   glPopMatrix();
}

double AircraftReference::Rad2Deg(double rad)
{
   return(rad*(180.0/M_PI));
}


double AircraftReference::Deg2Rad(double deg)
{
   return(deg*(M_PI/180.0));
}

