
 
#ifndef RNGSTREAM_H
#define RNGSTREAM_H
 
#include <string>
#include <cstring>
#include "simulation.h"

#if __cplusplus <= 199711L
#define static_assert(e, m) typedef char __C_ASSERT__[(e)?1:-1]
#endif

class RngStream
{
public:

RngStream (const char *name = "");


double RandU01 ();


const unsigned char * GetDigest() const;

private:

double Cg[6], Bg[6], Ig[6];


unsigned char digest[ID_FIELD_SIZE];
static_assert(sizeof(Cg) == sizeof(digest), "Mismatching sizes");


std::string name;


static double nextSeed[6];


};
 
#endif
 


