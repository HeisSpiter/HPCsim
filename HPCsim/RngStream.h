
 
#ifndef RNGSTREAM_H
#define RNGSTREAM_H
 
#include <string>
#include "sha2.h"

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


unsigned char digest[SHA384_DIGEST_LENGTH];


bool anti, incPrec;


std::string name;


static double nextSeed[6];


double U01 ();


double U01d ();


};
 
#endif
 


