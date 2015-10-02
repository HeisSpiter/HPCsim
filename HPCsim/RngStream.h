
 
#ifndef RNGSTREAM_H
#define RNGSTREAM_H
 
#include <string>
#include <cstring>
#include "simulation.h"

#define STATIC_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]

class RngStream
{
public:

RngStream (const char *name = "");


static bool SetPackageSeed (const unsigned long seed[6]);


void ResetStartStream ();


void ResetStartSubstream ();


void ResetNextSubstream ();


void SetAntithetic (bool a);


void IncreasedPrecis (bool incp);


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
STATIC_ASSERT(sizeof(Cg) == sizeof(digest));


bool anti, incPrec;


std::string name;


static double nextSeed[6];


double U01 ();


double U01d ();


};
 
#endif
 


