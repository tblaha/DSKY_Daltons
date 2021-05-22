#ifndef Body_hxx
#define Body_hxx

#include "RigidBody.hxx"
#include "integrate_rungekutta.hxx"

class Body: public RigidBody
{
    private:
        RungeKuttaWorkspace workspace;
        double dt;

    public:
        Body();
        ~Body();
        void derivative(VectorE& xd, double dt_offset);
        void step();
};

#endif
