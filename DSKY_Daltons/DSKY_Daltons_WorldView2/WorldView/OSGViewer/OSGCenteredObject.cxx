/* ------------------------------------------------------------------   */
/*      item            : OSGStaticObject.cxx
        made by         : Rene' van Paassen
        date            : 100127
	category        : body file 
        description     : 
	changes         : 100127 first version
        language        : C++
*/

#define OSGCenteredObject_cxx
#include "OSGCenteredObject.hxx"
#include "RvPQuat.hxx"
#include "OSGObjectFactory.hxx"
#include "AxisTransform.hxx"
#include <cstring>
#include <algorithm>
#include <osg/PositionAttitudeTransform>


OSGCenteredObject::OSGCenteredObject(const WorldDataSpec &specification) :
  OSGStaticObject(specification)
{
  //
}
  
OSGCenteredObject::~OSGCenteredObject()
{
  //
}

void OSGCenteredObject::connect(const GlobalId& master_id, const NameSet& cname,
                                entryid_type entry_id,
                                Channel::EntryTimeAspect time_aspect)
{
  // no action
}


void OSGCenteredObject::iterate(TimeTickType ts,
                                const BaseObjectMotion& base,
                                double late)
{
  BaseObjectMotion obs(base);
  
  if (base.dt != 0.0 && late != 0.0) {
    obs.extrapolate(late);
  }

  // fixate 
  if(position[0] != 0.0) obs.xyz[0] = position[0];
  if(position[1] != 0.0) obs.xyz[1] = position[1];
  if(position[2] != 0.0) obs.xyz[2] = position[2];
 
  // set position
  transform->setPosition(AxisTransform::osgPos(obs.xyz));
  //transform->setAttitude(AxisTransform::osgQuat(obs.attitude_q));
  
}

static SubContractor<OSGObjectTypeKey, OSGCenteredObject> 
*OSGCenteredObject_maker =
  new SubContractor<OSGObjectTypeKey, OSGCenteredObject>("centered");
