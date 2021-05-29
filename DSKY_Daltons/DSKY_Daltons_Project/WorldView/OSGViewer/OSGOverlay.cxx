/* ------------------------------------------------------------------   */
/*      item            : OSGOverlay.cxx
        made by         : Rene' van Paassen
        date            : 100127
	category        : body file 
        description     : 
	changes         : 100127 first version
        language        : C++
*/

#define OSGOverlay_cxx
#include "OSGOverlay.hxx"
#include "RvPQuat.hxx"
#include "OSGObjectFactory.hxx"
#include <osg/Geode>
#include <osg/Camera>
#include <osg/Texture2D>
#include <osgDB/ReadFile>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <debug.h>
#include "OSGViewer.hxx"

static const double deg2rad = M_PI / 180.0;

OSGOverlay::OSGOverlay(const WorldDataSpec &specification) :
  OSGObject()
{
  if (specification.filename.size() > 0) modelfile = specification.filename[0];
  this->name = specification.name;
  window_name = specification.filename.size() > 1 ?
    specification.filename[1] : std::string();
  view_name = specification.filename.size() > 2 ?
    specification.filename[3] : std::string();
}

OSGOverlay::~OSGOverlay()
{

}

void OSGOverlay::init(osg::ref_ptr<osg::Group>& root, OSGViewer* master)
{
#if 1
  /* Specific camera, render last, no lighting */
  orthocam = new osg::Camera;
  orthocam->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
  orthocam->setRenderOrder(osg::Camera::POST_RENDER);
  orthocam->setClearMask(GL_DEPTH_BUFFER_BIT);
  orthocam->setViewMatrix(osg::Matrix());
  orthocam->setProjectionMatrix(osg::Matrix::ortho2D(0, 1, 0, 1));
  orthocam->getOrCreateStateSet()->setMode
    (GL_LIGHTING, osg::StateAttribute::OFF);

  /* create a textured quad, add to geode */
  osg::Geometry *geom = osg::createTexturedQuadGeometry
    (osg::Vec3(), osg::Vec3(1.0f, 0.0f, 0.0f), osg::Vec3(0.0f, 1.0f, 0.0f),
     0.0f, 0.0f, 1.0f, 1.0f);
  osg::Geode * quad = new osg::Geode;
  quad->addDrawable(geom);

  /* create and load a texture */
  osg::Texture2D* ovltexture = new osg::Texture2D();
  osg::Image* image = osgDB::readImageFile(modelfile);
  if (!image) {
    E_MOD("Could not load image from " << modelfile);
    return;
  }
  ovltexture->setImage(image);

  /* attach the texture to the quad */
  osg::StateSet *stateset = quad->getOrCreateStateSet();
  stateset->setTextureAttributeAndModes
    (0, ovltexture, osg::StateAttribute::ON);
  stateset->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

  /* master camera? */
  osg::ref_ptr<osg::Camera> mastercam = master->getMainCamera
    (window_name, view_name);
  
  /* set gc and vieport, and add as slave to the relevant viewer */
  orthocam->setGraphicsContext(mastercam->getGraphicsContext());
  mastercam->getView()->addSlave(orthocam);
    
  /* connect to the camera */
  orthocam->addChild(quad);

#endif
}

void OSGOverlay::connect(const GlobalId& master_id, const NameSet& cname,
                       entryid_type entry_id,
                       Channel::EntryTimeAspect time_aspect)
{
  // todo
}

void OSGOverlay::iterate(TimeTickType ts,
                       const BaseObjectMotion& base, double late)
{
  // todo
}


static SubContractor<OSGObjectTypeKey, OSGOverlay> 
*OSGOverlay_maker = 
  new SubContractor<OSGObjectTypeKey, OSGOverlay>("overlay");

//states->setMode(GL_BLEND,osg::StateAttribute::ON);
//states->setRenderingHint( osg::StateSet::TRANSPARENT_BIN );

// http://delta3dengine.org/forum/viewtopic.php?showtopic=11603
// http://trac.openscenegraph.org/projects/osg//wiki/Support/Tutorials/HudsAndText
// https://stackoverflow.com/questions/13773916/openscenegraph-huds-textured-qt-widget-problems
/* To do (more or less):

   - create osg node
   - give it an ortho projection matrix, size of screen? / location
   - identity view matrix
   - create quad covering the overlay size
   - set texture coords on quad
   - load texture2D
   - set normals
   - state set, with depth disabled, transparency, and get into last
   - renderbin


   http://forum.openscenegraph.org/viewtopic.php?t=6831&view=previous
   

   https://github.com/ThermalPixel/osgdemos/tree/master
*/
