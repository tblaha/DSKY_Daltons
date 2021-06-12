#include <GL/glut.h>
#include <iostream>
#include <math.h>
#include <cstdio>

#include "AltTape.hxx"

using namespace std;


AltTape::AltTape():
   x_screenpos (359.0),
   y_screenpos (  0.0),
   lineWidth   (  1.0),
   altitude    (  0.0),
   altitude_bug(  0.0)
{
   lineColor[0] = 0.0;
   lineColor[1] = 1.0;
   lineColor[2] = 0.0;
   cout << "AltTape: constructor called" << endl;
}


AltTape::~AltTape()
{
   cout << "AltTape: destructor called" << endl;
}


void AltTape::DrawGL()
{
   // convert meters to feet:
   // GLdouble alt_ft = altitude*3.28084;
   // reverted back to meters
   GLdouble alt_ft = altitude;

   char temp[10];
   int icount,jcount;

   // save current matrix
   glPushMatrix();
      glTranslatef(x_screenpos, y_screenpos, 0.0);

      // enable clipping planes
      GLdouble eqn[4];

      // set bottom clipping plane, bottom of alt tape
      eqn[0] =   0.0;   //   x
      eqn[1] =   1.0;   //   y
      eqn[2] =   0.0;   //   z
      eqn[3] = 143.0;   //   D
      glClipPlane(GL_CLIP_PLANE0,eqn);

      // set top clipping plane, top of alt tape
      eqn[0] =   0.0;   //   x
      eqn[1] =  -1.0;   //   y
      eqn[2] =   0.0;   //   z
      eqn[3] = 143.0;   //   D
      glClipPlane(GL_CLIP_PLANE1,eqn);

      // enable clipping planes
      // set bottom clipping plane, just above alt indicator
      eqn[0] =   0.0;   //   x
      eqn[1] =   1.0;   //   y
      eqn[2] =   0.0;   //   z
      eqn[3] = -20.0;   //   D
      glClipPlane(GL_CLIP_PLANE2,eqn);

      // enable clipping planes
      // set top clipping plane, just below alt indicator
      eqn[0] =   0.0;   //   x
      eqn[1] =  -1.0;   //   y
      eqn[2] =   0.0;   //   z
      eqn[3] = -20.0;   //   D
      glClipPlane(GL_CLIP_PLANE3,eqn);


      // enable blending
      glEnable(GL_BLEND);
      glEnable(GL_LINE_SMOOTH);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      
      // draw tape
      // big ticks every 10m
      // small ticks every 2m

      // get modulus of alt_ft:
      double mod = fmod(alt_ft, 10.0);
      //double fivehundredft = (alt_ft - mod)/10.0;

      // cout << "altitude:      " << altitude << endl;
      // cout << "alt_ft:        " << alt_ft << endl;
      // cout << "mod:           " << mod << endl;
      // cout << "fivehundredft: " << fivehundredft << endl;

      // store current matrix
      glPushMatrix();

         // draw tape ticks
         // units per altitude interval: 5 units cover 10m
         // hence 0.5 units per m

         glEnable(GL_CLIP_PLANE0);
         glEnable(GL_CLIP_PLANE1);

         //glTranslatef(0.0, -mod*12.5, 0.0);
         glTranslatef(0.0, -alt_ft*13.02083, 0.0);
         
         glLineWidth(lineWidth);
         glColor3fv(lineColor);
         glBegin(GL_LINES);
            for (icount=0; icount<11; icount++)
            {
               glVertex2f( 0.0, icount*130.0);
               glVertex2f(15.0, icount*130.0);
               for (jcount=0; jcount<5; jcount++)
               {
                  glVertex2f( 0.0, icount*130.0+jcount*26.0);
                  glVertex2f( 7.5, icount*130.0+jcount*26.0);
               }
            }
         glEnd();

         // draw floor
         //glTranslatef(0.0, -mod*0.26, 0.0);
         glLineWidth(5*lineWidth);
         float red[3] {1.0, 0, 0};
         glColor3fv(red);
         glBegin(GL_LINES);
            glVertex2f(0.0, (_floor_h)*13.02083); // draw floor
            glVertex2f(50.0, (_floor_h)*13.02083); // draw floor
         glEnd();

         // draw term alt estimate
         //glTranslatef(0.0, -mod*0.26, 0.0);
         glLineWidth(2*lineWidth);
         glColor3fv(lineColor);
         glBegin(GL_LINES);
            glVertex2f(0.0, (_term_alt)*13.02083);
            glVertex2f(-20.0, 10+(_term_alt)*13.02083); 
            glVertex2f(-20.0, 10+(_term_alt)*13.02083);
            glVertex2f(-20.0, -10+(_term_alt)*13.02083);
            glVertex2f(-20.0, -10+(_term_alt)*13.02083);
            glVertex2f(0.0, (_term_alt)*13.02083);
         glEnd();

         // disable clipping planes
         glDisable(GL_CLIP_PLANE0);
         glDisable(GL_CLIP_PLANE1);


         // draw tape digits
         // each fivehundred kts
         // 2000 ft -> 02,0

         glEnable(GL_CLIP_PLANE1);
         glEnable(GL_CLIP_PLANE2);
         
         for (icount=0; icount<11; icount++)
         {
            //double firstdigits = (alt_ft + icount*500.0 - 1000.0 - fmod(alt_ft + icount*500.0 - 1000.0, 1000.0))/1000.0;
            //double lastdigit = fmod((alt_ft + icount*500.0 - 1000.0 - fmod(alt_ft + icount*500.0 - 1000.0, 500.0)), 1000.0)/100.0;
            double firstdigits = (10*icount);
            //snprintf(temp, 10, "%2d,%1d", (int)firstdigits, (int)lastdigit );
            snprintf(temp, 10, "%2d", (int)firstdigits);
            strokeString(24.0, icount*130.0, temp, GLUT_STROKE_ROMAN, 0.15);
         }

         glDisable(GL_CLIP_PLANE1);
         glDisable(GL_CLIP_PLANE2);


         glEnable(GL_CLIP_PLANE0);
         glEnable(GL_CLIP_PLANE3);

         for (icount=0; icount<11; icount++)
         {
            //double firstdigits = (alt_ft + icount*500.0 - 1000.0 - fmod(alt_ft + icount*500.0 - 1000.0, 1000.0))/1000.0;
            //double lastdigit = fmod((alt_ft + icount*500.0 - 1000.0 - fmod(alt_ft + icount*500.0 - 1000.0, 500.0)), 1000.0)/100.0;
            double firstdigits = (10*icount);
            //snprintf(temp, 10, "%2d,%1d", (int)firstdigits, (int)lastdigit );
            snprintf(temp, 10, "%2d", (int)firstdigits);
            strokeString(24.0, icount*130.0, temp, GLUT_STROKE_ROMAN, 0.15);
         }

         // disable clipping planes
         glDisable(GL_CLIP_PLANE0);
         glDisable(GL_CLIP_PLANE3);

      glPopMatrix();   // restore
      
      glPushMatrix();

         // draw altitude indicator outline
         glLineWidth(lineWidth);
         glColor3fv(lineColor);
         glBegin(GL_LINE_LOOP);
            glVertex2f(103.0,  14.0);
            glVertex2f(29.0,  14.0);
            glVertex2f(20.0,   0.0);
            glVertex2f(29.0, -14.0);
            glVertex2f(103.0, -14.0);
            glVertex2f(103.0,  14.0);
         glEnd();

   
         // draw altitude indicator zero line
         glLineWidth(lineWidth);
         glColor3fv(lineColor);
         glBegin(GL_LINE_LOOP);
            glVertex2f(-17.0,   0.0);
            glVertex2f(-51.0,   0.0);
         glEnd();


         // show altitude
         // mod = fmod(alt_ft, 1000.0);
         // double thousands = (alt_ft-mod)/1000.0;
         // mod = mod - fmod(mod, 10.0);
         glColor3fv(lineColor);
         snprintf(temp, 10, "%3.1f", (float)alt_ft);
         strokeString(100.0, -9.0, temp, GLUT_STROKE_ROMAN, 0.15, 1);


         // draw radio altitude outline
         // glLineWidth(lineWidth);
         // glColor3fv(lineColor);
         // glBegin(GL_LINE_LOOP);
         //    glVertex2f( -22.0, -158.0);
         //    glVertex2f(  44.0, -158.0);
         //    glVertex2f(  44.0, -189.0);
         //    glVertex2f( -22.0, -189.0);
         //    glVertex2f( -22.0, -158.0);
         // glEnd();
         // strokeString(-50.0, -182.0, "AR", GLUT_STROKE_ROMAN, 0.15);
         // snprintf(temp, 10, "%4.0f", (alt_ft-fmod(alt_ft, 10.0)));
         // strokeString( 40.0, -182.0, temp, GLUT_STROKE_ROMAN, 0.15, 1);
         //strokeString(-50.0, -182.0, "R", GLUT_STROKE_ROMAN, 0.15);

         // floor height
         snprintf(temp, 10, "%2d", (int)_floor_h);
         strokeString(100.0, -40.0, temp, GLUT_STROKE_ROMAN, 0.15, 1);

         // vertical rate
         snprintf(temp, 10, "R%3.1f ", (float)_rate);
         strokeString( 40.0, -182.0, temp, GLUT_STROKE_ROMAN, 0.15, 1);

         // max commandable rate in current mode
         snprintf(temp, 10, "M%3.0f", (float)_max_rate);
         strokeString( 100.0, -182.0, temp, GLUT_STROKE_ROMAN, 0.15, 1);


         // draw altitude low indicator
         // strokeString(-50.0, -222.0, "AL", GLUT_STROKE_ROMAN, 0.15);
         // snprintf(temp, 10, "%4.0f", 300.0);
         // strokeString( 40.0, -222.0, temp, GLUT_STROKE_ROMAN, 0.15, 1);
   
      glPopMatrix();   // restore

      //   disable blending
      glDisable(GL_BLEND);
      glDisable(GL_LINE_SMOOTH);

   // final restore of MODELVIEW matrix
   glPopMatrix();   // restore
}


void AltTape::strokeString(double x, double y, char *msg, void *font, double size)
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
      glLineWidth(lineWidth);
   glPopMatrix();
}


void AltTape::strokeString(double x, double y, char *msg, void *font, double size, int align)
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
