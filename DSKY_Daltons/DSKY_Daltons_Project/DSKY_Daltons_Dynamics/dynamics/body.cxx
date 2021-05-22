#define Body_cxx
#include "body.hxx"

Body::Body() :
    RigidBody(1, 1, 1, 1, 1, 1, 1, 0),
    workspace(13)
{
    dt = 0;
}

Body::~Body()
{

}

void Body::derivative(VectorE& xd, double dt_offset)
{
    return;
}

void Body::step()
{
    integrate_rungekutta(*this, workspace, dt);
}