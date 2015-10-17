/***********************************************************************\
 *
 * File:           RngStream.cpp for multiple streams of Random Numbers
 * Language:       C++ (ISO 1998)
 * Copyright:      Pierre L'Ecuyer, University of Montreal
 * Notice:         This code can be used freely for personal, academic,
 *                 or non-commercial purposes. For commercial purposes, 
 *                 please contact P. L'Ecuyer at: lecuyer@iro.umontreal.ca
 * Date:           14 August 2001
 *
\***********************************************************************/


#include <cstdlib>
#include <iostream>
#include "RngStream.h"
using namespace std;

namespace
{
const double m1   =       4294967087.0;
const double m2   =       4294944443.0;
const double norm =       1.0 / (m1 + 1.0);
const double a12  =       1403580.0;
const double a13n =       810728.0;
const double a21  =       527612.0;
const double a23n =       1370589.0;
const double two17 =      131072.0;
const double two53 =      9007199254740992.0;

// The following are the transition matrices of the two MRG components
// (in matrix form), raised to the powers 2^127, resp.

const double A1p127[3][3] = {
       {    2427906178.0, 3580155704.0,  949770784.0 },
       {     226153695.0, 1230515664.0, 3580155704.0 },
       {    1988835001.0,  986791581.0, 1230515664.0 }
       };

const double A2p127[3][3] = {
       {    1464411153.0,  277697599.0, 1610723613.0 },
       {      32183930.0, 1464411153.0, 1022607788.0 },
       {    2824425944.0,   32183930.0, 2093834863.0 }
       };



//-------------------------------------------------------------------------
// Return (a*s + c) MOD m; a, s, c and m must be < 2^35
//
double MultModM (double a, double s, double c, double m)
{
    double v;
    long a1;

    v = a * s + c;

    if (v >= two53 || v <= -two53) {
        a1 = static_cast<long> (a / two17);    a -= a1 * two17;
        v  = a1 * s;
        a1 = static_cast<long> (v / m);     v -= a1 * m;
        v = v * two17 + a * s + c;
    }

    a1 = static_cast<long> (v / m);
    /* in case v < 0)*/
    if ((v -= a1 * m) < 0.0) return v += m;   else return v;
}


//-------------------------------------------------------------------------
// Compute the vector v = A*s MOD m. Assume that -m < s[i] < m.
// Works also when v = s.
//
void MatVecModM (const double A[3][3], const double s[3], double v[3],
                 double m)
{
    int i;
    double x[3];               // Necessary if v = s

    for (i = 0; i < 3; ++i) {
        x[i] = MultModM (A[i][0], s[0], 0.0, m);
        x[i] = MultModM (A[i][1], s[1], x[i], m);
        x[i] = MultModM (A[i][2], s[2], x[i], m);
    }
    for (i = 0; i < 3; ++i)
        v[i] = x[i];
}

} // end of anonymous namespace


//*************************************************************************
// Public members of the class start here


//-------------------------------------------------------------------------
// The default seed of the package; will be the seed of the first
// declared RngStream, unless SetPackageSeed is called.
//
double RngStream::nextSeed[6] =
{
   12345.0, 12345.0, 12345.0, 12345.0, 12345.0, 12345.0
};


//-------------------------------------------------------------------------
// constructor
//
RngStream::RngStream (const char *s) : name (s)
{
   /* Information on a stream. The arrays {Cg, Bg, Ig} contain the current
   state of the stream, the starting state of the current SubStream, and the
   starting state of the stream. This stream generates antithetic variates
   if anti = true. It also generates numbers with extended precision (53
   bits if machine follows IEEE 754 standard) if incPrec = true. nextSeed
   will be the seed of the next declared RngStream. */

   for (int i = 0; i < 6; ++i) {
      Bg[i] = Cg[i] = Ig[i] = nextSeed[i];
   }

   memcpy(digest, Cg, sizeof(digest));

   MatVecModM (A1p127, nextSeed, nextSeed, m1);
   MatVecModM (A2p127, &nextSeed[3], &nextSeed[3], m2);
}


//-------------------------------------------------------------------------
// Advance in seeds to skip streams
//
void RngStream::AdvanceStream(unsigned long n)
{
   for (unsigned long i = 0; i < n; ++i) {
      MatVecModM (A1p127, nextSeed, nextSeed, m1);
      MatVecModM (A2p127, &nextSeed[3], &nextSeed[3], m2);
   }
}

//-------------------------------------------------------------------------
// Generate the next random number.
//
double RngStream::RandU01 ()
{
    long k;
    double p1, p2, u;

    /* Component 1 */
    p1 = a12 * Cg[1] - a13n * Cg[0];
    k = static_cast<long> (p1 / m1);
    p1 -= k * m1;
    if (p1 < 0.0) p1 += m1;
    Cg[0] = Cg[1]; Cg[1] = Cg[2]; Cg[2] = p1;

    /* Component 2 */
    p2 = a21 * Cg[5] - a23n * Cg[3];
    k = static_cast<long> (p2 / m2);
    p2 -= k * m2;
    if (p2 < 0.0) p2 += m2;
    Cg[3] = Cg[4]; Cg[4] = Cg[5]; Cg[5] = p2;

    /* Combination */
    u = ((p1 > p2) ? (p1 - p2) * norm : (p1 - p2 + m1) * norm);

    return u;
}


//-------------------------------------------------------------------------
// Get digest
//
const unsigned char * RngStream::GetDigest() const
{
    return digest;
}
