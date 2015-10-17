
 
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


static bool SetPackageSeed (const unsigned long seed[6]);


void ResetStartStream ();


void ResetStartSubstream ();


void ResetNextSubstream ();


bool SetSeed (const unsigned long seed[6]);


void AdvanceState (long e, long c);


void GetState (unsigned long seed[6]) const;


void WriteState () const;


void WriteStateFull () const;


double RandU01 ();


int RandInt (int i, int j);


double RandDouble (double i, double j);


const unsigned char * GetDigest() const;

private:

double Cg[6], Bg[6], Ig[6];


unsigned char digest[ID_FIELD_SIZE];
static_assert(sizeof(Cg) == sizeof(digest), "Mismatching sizes");


std::string name;


static double nextSeed[6];


};
 
#endif
 


