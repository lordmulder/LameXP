/***************************************************************************
**                                                                        **
**  Keccak (http://keccak.noekeon.org/) implementation file               **
**                                                                        **
**  This file implements the Keccak hash algorithm as provided by the     **
**  inventors, and submitted to the third round of the NIST SHA-3         **
**  competition.                                                          **
**  For the QKeccakHash wrapper, the originally multiple files were       **
**  combined in this one. Slight modifications have been made to          **
**  allow use in C++ as an independently included .cpp file.              **
**  It is not meant to be compiled into an object, hence, there is no     **
**  header.                                                               **
**                                                                        **
**  The code here is not part of the actual QKeccakHash implementation    **
**  and thus not licensed under the GPLv3 but under its respecitve        **
**  license given by the inventors. Typically, this means it is in the    **
**  public domain, if not noted otherwise.                                **
**                                                                        **
***************************************************************************/

namespace KeccakImpl {

typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef unsigned long long int UINT64;

// ================================================================================
// =================== brg_endian.h
// ================================================================================


/*
 ---------------------------------------------------------------------------
 Copyright (c) 1998-2008, Brian Gladman, Worcester, UK. All rights reserved.

 LICENSE TERMS

 The redistribution and use of this software (with or without changes)
 is allowed without the payment of fees or royalties provided that:

  1. source code distributions include the above copyright notice, this
     list of conditions and the following disclaimer;

  2. binary distributions include the above copyright notice, this list
     of conditions and the following disclaimer in their documentation;

  3. the name of the copyright holder is not used to endorse products
     built using this software without specific written permission.

 DISCLAIMER

 This software is provided 'as is' with no explicit or implied warranties
 in respect of its properties, including, but not limited to, correctness
 and/or fitness for purpose.
 ---------------------------------------------------------------------------
 Issue Date: 20/12/2007
 Changes for ARM 9/9/2010
*/

#ifndef _BRG_ENDIAN_H
#define _BRG_ENDIAN_H

#define IS_BIG_ENDIAN      4321 /* byte 0 is most significant (mc68k) */
#define IS_LITTLE_ENDIAN   1234 /* byte 0 is least significant (i386) */

#if 0
/* Include files where endian defines and byteswap functions may reside */
#if defined( __sun )
#  include <sys/isa_defs.h>
#elif defined( __FreeBSD__ ) || defined( __OpenBSD__ ) || defined( __NetBSD__ )
#  include <sys/endian.h>
#elif defined( BSD ) && ( BSD >= 199103 ) || defined( __APPLE__ ) || \
      defined( __CYGWIN32__ ) || defined( __DJGPP__ ) || defined( __osf__ )
#  include <machine/endian.h>
#elif defined( __linux__ ) || defined( __GNUC__ ) || defined( __GNU_LIBRARY__ )
#  if !defined( __MINGW32__ ) && !defined( _AIX )
#    include <endian.h>
#    if !defined( __BEOS__ )
#      include <byteswap.h>
#    endif
#  endif
#endif
#endif

/* Now attempt to set the define for platform byte order using any  */
/* of the four forms SYMBOL, _SYMBOL, __SYMBOL & __SYMBOL__, which  */
/* seem to encompass most endian symbol definitions                 */

#if defined( BIG_ENDIAN ) && defined( LITTLE_ENDIAN )
#  if defined( BYTE_ORDER ) && BYTE_ORDER == BIG_ENDIAN
#    define PLATFORM_BYTE_ORDER IS_BIG_ENDIAN
#  elif defined( BYTE_ORDER ) && BYTE_ORDER == LITTLE_ENDIAN
#    define PLATFORM_BYTE_ORDER IS_LITTLE_ENDIAN
#  endif
#elif defined( BIG_ENDIAN )
#  define PLATFORM_BYTE_ORDER IS_BIG_ENDIAN
#elif defined( LITTLE_ENDIAN )
#  define PLATFORM_BYTE_ORDER IS_LITTLE_ENDIAN
#endif

#if defined( _BIG_ENDIAN ) && defined( _LITTLE_ENDIAN )
#  if defined( _BYTE_ORDER ) && _BYTE_ORDER == _BIG_ENDIAN
#    define PLATFORM_BYTE_ORDER IS_BIG_ENDIAN
#  elif defined( _BYTE_ORDER ) && _BYTE_ORDER == _LITTLE_ENDIAN
#    define PLATFORM_BYTE_ORDER IS_LITTLE_ENDIAN
#  endif
#elif defined( _BIG_ENDIAN )
#  define PLATFORM_BYTE_ORDER IS_BIG_ENDIAN
#elif defined( _LITTLE_ENDIAN )
#  define PLATFORM_BYTE_ORDER IS_LITTLE_ENDIAN
#endif

#if defined( __BIG_ENDIAN ) && defined( __LITTLE_ENDIAN )
#  if defined( __BYTE_ORDER ) && __BYTE_ORDER == __BIG_ENDIAN
#    define PLATFORM_BYTE_ORDER IS_BIG_ENDIAN
#  elif defined( __BYTE_ORDER ) && __BYTE_ORDER == __LITTLE_ENDIAN
#    define PLATFORM_BYTE_ORDER IS_LITTLE_ENDIAN
#  endif
#elif defined( __BIG_ENDIAN )
#  define PLATFORM_BYTE_ORDER IS_BIG_ENDIAN
#elif defined( __LITTLE_ENDIAN )
#  define PLATFORM_BYTE_ORDER IS_LITTLE_ENDIAN
#endif

#if defined( __BIG_ENDIAN__ ) && defined( __LITTLE_ENDIAN__ )
#  if defined( __BYTE_ORDER__ ) && __BYTE_ORDER__ == __BIG_ENDIAN__
#    define PLATFORM_BYTE_ORDER IS_BIG_ENDIAN
#  elif defined( __BYTE_ORDER__ ) && __BYTE_ORDER__ == __LITTLE_ENDIAN__
#    define PLATFORM_BYTE_ORDER IS_LITTLE_ENDIAN
#  endif
#elif defined( __BIG_ENDIAN__ )
#  define PLATFORM_BYTE_ORDER IS_BIG_ENDIAN
#elif defined( __LITTLE_ENDIAN__ )
#  define PLATFORM_BYTE_ORDER IS_LITTLE_ENDIAN
#endif

/*  if the platform byte order could not be determined, then try to */
/*  set this define using common machine defines                    */
#if !defined(PLATFORM_BYTE_ORDER)

#if   defined( __alpha__ ) || defined( __alpha ) || defined( i386 )       || \
      defined( __i386__ )  || defined( _M_I86 )  || defined( _M_IX86 )    || \
      defined( __OS2__ )   || defined( sun386 )  || defined( __TURBOC__ ) || \
      defined( vax )       || defined( vms )     || defined( VMS )        || \
      defined( __VMS )     || defined( _M_X64 )
#  define PLATFORM_BYTE_ORDER IS_LITTLE_ENDIAN

#elif defined( AMIGA )   || defined( applec )    || defined( __AS400__ )  || \
      defined( _CRAY )   || defined( __hppa )    || defined( __hp9000 )   || \
      defined( ibm370 )  || defined( mc68000 )   || defined( m68k )       || \
      defined( __MRC__ ) || defined( __MVS__ )   || defined( __MWERKS__ ) || \
      defined( sparc )   || defined( __sparc)    || defined( SYMANTEC_C ) || \
      defined( __VOS__ ) || defined( __TIGCC__ ) || defined( __TANDEM )   || \
      defined( THINK_C ) || defined( __VMCMS__ ) || defined( _AIX )
#  define PLATFORM_BYTE_ORDER IS_BIG_ENDIAN

#elif defined(__arm__)
# ifdef __BIG_ENDIAN
#  define PLATFORM_BYTE_ORDER IS_BIG_ENDIAN
# else
#  define PLATFORM_BYTE_ORDER IS_LITTLE_ENDIAN
# endif
#elif 1     /* **** EDIT HERE IF NECESSARY **** */
#  define PLATFORM_BYTE_ORDER IS_LITTLE_ENDIAN
#elif 0     /* **** EDIT HERE IF NECESSARY **** */
#  define PLATFORM_BYTE_ORDER IS_BIG_ENDIAN
#else
#  error Please edit lines 132 or 134 in brg_endian.h to set the platform byte order
#endif

#endif

#endif

// ================================================================================
// =================== KeccakSponge.h
// ================================================================================

/*
The Keccak sponge function, designed by Guido Bertoni, Joan Daemen,
Michaël Peeters and Gilles Van Assche. For more information, feedback or
questions, please refer to our website: http://keccak.noekeon.org/

Implementation by the designers,
hereby denoted as "the implementer".

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/

#ifndef _KeccakSponge_h_
#define _KeccakSponge_h_

/**
  * Function to initialize the state of the Keccak[r, c] sponge function.
  * The sponge function is set to the absorbing phase.
  * @param  state       Pointer to the state of the sponge function to be initialized.
  * @param  rate        The value of the rate r.
  * @param  capacity    The value of the capacity c.
  * @pre    One must have r+c=1600 and the rate a multiple of 64 bits in this implementation.
  * @return Zero if successful, 1 otherwise.
  */
int InitSponge(spongeState *state, unsigned int rate, unsigned int capacity);
/**
  * Function to give input data for the sponge function to absorb.
  * @param  state       Pointer to the state of the sponge function initialized by InitSponge().
  * @param  data        Pointer to the input data. 
  *                     When @a databitLen is not a multiple of 8, the last bits of data must be
  *                     in the least significant bits of the last byte.
  * @param  databitLen  The number of input bits provided in the input data.
  * @pre    In the previous call to Absorb(), databitLen was a multiple of 8.
  * @pre    The sponge function must be in the absorbing phase,
  *         i.e., Squeeze() must not have been called before.
  * @return Zero if successful, 1 otherwise.
  */
int Absorb(spongeState *state, const unsigned char *data, unsigned long long databitlen);
/**
  * Function to squeeze output data from the sponge function.
  * If the sponge function was in the absorbing phase, this function 
  * switches it to the squeezing phase.
  * @param  state       Pointer to the state of the sponge function initialized by InitSponge().
  * @param  output      Pointer to the buffer where to store the output data.
  * @param  outputLength    The number of output bits desired.
  *                     It must be a multiple of 8.
  * @return Zero if successful, 1 otherwise.
  */
int Squeeze(spongeState *state, unsigned char *output, unsigned long long outputLength);

#endif

// ================================================================================
// =================== KeccakF-1600-int-set.h
// ================================================================================


#define ProvideFast576
#define ProvideFast832
#define ProvideFast1024
#define ProvideFast1088
#define ProvideFast1152
#define ProvideFast1344


// ================================================================================
// =================== KeccakF-1600-interface.h
// ================================================================================


/*
The Keccak sponge function, designed by Guido Bertoni, Joan Daemen,
Michaël Peeters and Gilles Van Assche. For more information, feedback or
questions, please refer to our website: http://keccak.noekeon.org/

Implementation by the designers,
hereby denoted as "the implementer".

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/

#ifndef _KeccakPermutationInterface_h_
#define _KeccakPermutationInterface_h_

//#include "KeccakF-1600-int-set.h"

void KeccakInitialize( void );
void KeccakInitializeState(unsigned char *state);
void KeccakPermutation(unsigned char *state);
#ifdef ProvideFast576
void KeccakAbsorb576bits(unsigned char *state, const unsigned char *data);
#endif
#ifdef ProvideFast832
void KeccakAbsorb832bits(unsigned char *state, const unsigned char *data);
#endif
#ifdef ProvideFast1024
void KeccakAbsorb1024bits(unsigned char *state, const unsigned char *data);
#endif
#ifdef ProvideFast1088
void KeccakAbsorb1088bits(unsigned char *state, const unsigned char *data);
#endif
#ifdef ProvideFast1152
void KeccakAbsorb1152bits(unsigned char *state, const unsigned char *data);
#endif
#ifdef ProvideFast1344
void KeccakAbsorb1344bits(unsigned char *state, const unsigned char *data);
#endif
void KeccakAbsorb(unsigned char *state, const unsigned char *data, unsigned int laneCount);
#ifdef ProvideFast1024
void KeccakExtract1024bits(const unsigned char *state, unsigned char *data);
#endif
void KeccakExtract(const unsigned char *state, unsigned char *data, unsigned int laneCount);

#endif


// ================================================================================
// =================== KeccakF-1600-opt32-settings.h
// ================================================================================


#define Unrolling 2
//#define UseBebigokimisa
//#define UseInterleaveTables
#define UseSchedule 3


// ================================================================================
// =================== KeccakF-1600-unrolling.macros
// ================================================================================

/*
The Keccak sponge function, designed by Guido Bertoni, Joan Daemen,
Michaël Peeters and Gilles Van Assche. For more information, feedback or
questions, please refer to our website: http://keccak.noekeon.org/

Implementation by the designers,
hereby denoted as "the implementer".

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/

#if (Unrolling == 24)
#define rounds \
    prepareTheta \
    thetaRhoPiChiIotaPrepareTheta( 0, A, E) \
    thetaRhoPiChiIotaPrepareTheta( 1, E, A) \
    thetaRhoPiChiIotaPrepareTheta( 2, A, E) \
    thetaRhoPiChiIotaPrepareTheta( 3, E, A) \
    thetaRhoPiChiIotaPrepareTheta( 4, A, E) \
    thetaRhoPiChiIotaPrepareTheta( 5, E, A) \
    thetaRhoPiChiIotaPrepareTheta( 6, A, E) \
    thetaRhoPiChiIotaPrepareTheta( 7, E, A) \
    thetaRhoPiChiIotaPrepareTheta( 8, A, E) \
    thetaRhoPiChiIotaPrepareTheta( 9, E, A) \
    thetaRhoPiChiIotaPrepareTheta(10, A, E) \
    thetaRhoPiChiIotaPrepareTheta(11, E, A) \
    thetaRhoPiChiIotaPrepareTheta(12, A, E) \
    thetaRhoPiChiIotaPrepareTheta(13, E, A) \
    thetaRhoPiChiIotaPrepareTheta(14, A, E) \
    thetaRhoPiChiIotaPrepareTheta(15, E, A) \
    thetaRhoPiChiIotaPrepareTheta(16, A, E) \
    thetaRhoPiChiIotaPrepareTheta(17, E, A) \
    thetaRhoPiChiIotaPrepareTheta(18, A, E) \
    thetaRhoPiChiIotaPrepareTheta(19, E, A) \
    thetaRhoPiChiIotaPrepareTheta(20, A, E) \
    thetaRhoPiChiIotaPrepareTheta(21, E, A) \
    thetaRhoPiChiIotaPrepareTheta(22, A, E) \
    thetaRhoPiChiIota(23, E, A) \
    copyToState(state, A)
#elif (Unrolling == 12)
#define rounds \
    prepareTheta \
    for(i=0; i<24; i+=12) { \
        thetaRhoPiChiIotaPrepareTheta(i   , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+ 1, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+ 2, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+ 3, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+ 4, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+ 5, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+ 6, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+ 7, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+ 8, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+ 9, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+10, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+11, E, A) \
    } \
    copyToState(state, A)
#elif (Unrolling == 8)
#define rounds \
    prepareTheta \
    for(i=0; i<24; i+=8) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+2, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+3, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+4, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+5, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+6, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+7, E, A) \
    } \
    copyToState(state, A)
#elif (Unrolling == 6)
#define rounds \
    prepareTheta \
    for(i=0; i<24; i+=6) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+2, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+3, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+4, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+5, E, A) \
    } \
    copyToState(state, A)
#elif (Unrolling == 4)
#define rounds \
    prepareTheta \
    for(i=0; i<24; i+=4) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+2, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+3, E, A) \
    } \
    copyToState(state, A)
#elif (Unrolling == 3)
#define rounds \
    prepareTheta \
    for(i=0; i<24; i+=3) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+2, A, E) \
        copyStateVariables(A, E) \
    } \
    copyToState(state, A)
#elif (Unrolling == 2)
#define rounds \
    prepareTheta \
    for(i=0; i<24; i+=2) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
    } \
    copyToState(state, A)
#elif (Unrolling == 1)
#define rounds \
    prepareTheta \
    for(i=0; i<24; i++) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        copyStateVariables(A, E) \
    } \
    copyToState(state, A)
#else
#error "Unrolling is not correctly specified!"
#endif


// ================================================================================
// =================== KeccakF-1600-32-rvk.macros
// ================================================================================

/*
The Keccak sponge function, designed by Guido Bertoni, Joan Daemen,
Michaël Peeters and Gilles Van Assche. For more information, feedback or
questions, please refer to our website: http://keccak.noekeon.org/

Implementation by Ronny Van Keer,
hereby denoted as "the implementer".

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/

static const UINT32 KeccakF1600RoundConstants_int2[2*24] =
{
    0x00000001UL,    0x00000000UL,
    0x00000000UL,    0x00000089UL,
    0x00000000UL,    0x8000008bUL,
    0x00000000UL,    0x80008080UL,
    0x00000001UL,    0x0000008bUL,
    0x00000001UL,    0x00008000UL,
    0x00000001UL,    0x80008088UL,
    0x00000001UL,    0x80000082UL,
    0x00000000UL,    0x0000000bUL,
    0x00000000UL,    0x0000000aUL,
    0x00000001UL,    0x00008082UL,
    0x00000000UL,    0x00008003UL,
    0x00000001UL,    0x0000808bUL,
    0x00000001UL,    0x8000000bUL,
    0x00000001UL,    0x8000008aUL,
    0x00000001UL,    0x80000081UL,
    0x00000000UL,    0x80000081UL,
    0x00000000UL,    0x80000008UL,
    0x00000000UL,    0x00000083UL,
    0x00000000UL,    0x80008003UL,
    0x00000001UL,    0x80008088UL,
    0x00000000UL,    0x80000088UL,
    0x00000001UL,    0x00008000UL,
    0x00000000UL,    0x80008082UL
};

#undef rounds

#define rounds \
{ \
    UINT32 Da0, De0, Di0, Do0, Du0; \
    UINT32 Da1, De1, Di1, Do1, Du1; \
    UINT32 Ba, Be, Bi, Bo, Bu; \
    UINT32 Aba0, Abe0, Abi0, Abo0, Abu0; \
    UINT32 Aba1, Abe1, Abi1, Abo1, Abu1; \
    UINT32 Aga0, Age0, Agi0, Ago0, Agu0; \
    UINT32 Aga1, Age1, Agi1, Ago1, Agu1; \
    UINT32 Aka0, Ake0, Aki0, Ako0, Aku0; \
    UINT32 Aka1, Ake1, Aki1, Ako1, Aku1; \
    UINT32 Ama0, Ame0, Ami0, Amo0, Amu0; \
    UINT32 Ama1, Ame1, Ami1, Amo1, Amu1; \
    UINT32 Asa0, Ase0, Asi0, Aso0, Asu0; \
    UINT32 Asa1, Ase1, Asi1, Aso1, Asu1; \
    UINT32 Cw, Cx, Cy, Cz; \
    UINT32 Eba0, Ebe0, Ebi0, Ebo0, Ebu0; \
    UINT32 Eba1, Ebe1, Ebi1, Ebo1, Ebu1; \
    UINT32 Ega0, Ege0, Egi0, Ego0, Egu0; \
    UINT32 Ega1, Ege1, Egi1, Ego1, Egu1; \
    UINT32 Eka0, Eke0, Eki0, Eko0, Eku0; \
    UINT32 Eka1, Eke1, Eki1, Eko1, Eku1; \
    UINT32 Ema0, Eme0, Emi0, Emo0, Emu0; \
    UINT32 Ema1, Eme1, Emi1, Emo1, Emu1; \
    UINT32 Esa0, Ese0, Esi0, Eso0, Esu0; \
    UINT32 Esa1, Ese1, Esi1, Eso1, Esu1; \
	const UINT32 * pRoundConstants = KeccakF1600RoundConstants_int2; \
    UINT32 i; \
\
    copyFromState(A, state) \
\
    for( i = 12; i != 0; --i ) { \
	    Cx = Abu0^Agu0^Aku0^Amu0^Asu0; \
	    Du1 = Abe1^Age1^Ake1^Ame1^Ase1; \
	    Da0 = Cx^ROL32(Du1, 1); \
	    Cz = Abu1^Agu1^Aku1^Amu1^Asu1; \
	    Du0 = Abe0^Age0^Ake0^Ame0^Ase0; \
	    Da1 = Cz^Du0; \
\
	    Cw = Abi0^Agi0^Aki0^Ami0^Asi0; \
	    Do0 = Cw^ROL32(Cz, 1); \
	    Cy = Abi1^Agi1^Aki1^Ami1^Asi1; \
	    Do1 = Cy^Cx; \
\
	    Cx = Aba0^Aga0^Aka0^Ama0^Asa0; \
	    De0 = Cx^ROL32(Cy, 1); \
	    Cz = Aba1^Aga1^Aka1^Ama1^Asa1; \
	    De1 = Cz^Cw; \
\
	    Cy = Abo1^Ago1^Ako1^Amo1^Aso1; \
	    Di0 = Du0^ROL32(Cy, 1); \
	    Cw = Abo0^Ago0^Ako0^Amo0^Aso0; \
	    Di1 = Du1^Cw; \
\
	    Du0 = Cw^ROL32(Cz, 1); \
	    Du1 = Cy^Cx; \
\
	    Aba0 ^= Da0; \
	    Ba = Aba0; \
	    Age0 ^= De0; \
	    Be = ROL32(Age0, 22); \
	    Aki1 ^= Di1; \
	    Bi = ROL32(Aki1, 22); \
	    Amo1 ^= Do1; \
	    Bo = ROL32(Amo1, 11); \
	    Asu0 ^= Du0; \
	    Bu = ROL32(Asu0, 7); \
	    Eba0 =   Ba ^((~Be)&  Bi ) ^ *(pRoundConstants++); \
	    Ebe0 =   Be ^((~Bi)&  Bo ); \
	    Ebi0 =   Bi ^((~Bo)&  Bu ); \
	    Ebo0 =   Bo ^((~Bu)&  Ba ); \
	    Ebu0 =   Bu ^((~Ba)&  Be ); \
\
	    Abo0 ^= Do0; \
	    Ba = ROL32(Abo0, 14); \
	    Agu0 ^= Du0; \
	    Be = ROL32(Agu0, 10); \
	    Aka1 ^= Da1; \
	    Bi = ROL32(Aka1, 2); \
	    Ame1 ^= De1; \
	    Bo = ROL32(Ame1, 23); \
	    Asi1 ^= Di1; \
	    Bu = ROL32(Asi1, 31); \
	    Ega0 =   Ba ^((~Be)&  Bi ); \
	    Ege0 =   Be ^((~Bi)&  Bo ); \
	    Egi0 =   Bi ^((~Bo)&  Bu ); \
	    Ego0 =   Bo ^((~Bu)&  Ba ); \
	    Egu0 =   Bu ^((~Ba)&  Be ); \
\
	    Abe1 ^= De1; \
	    Ba = ROL32(Abe1, 1); \
	    Agi0 ^= Di0; \
	    Be = ROL32(Agi0, 3); \
	    Ako1 ^= Do1; \
	    Bi = ROL32(Ako1, 13); \
	    Amu0 ^= Du0; \
	    Bo = ROL32(Amu0, 4); \
	    Asa0 ^= Da0; \
	    Bu = ROL32(Asa0, 9); \
	    Eka0 =   Ba ^((~Be)&  Bi ); \
	    Eke0 =   Be ^((~Bi)&  Bo ); \
	    Eki0 =   Bi ^((~Bo)&  Bu ); \
	    Eko0 =   Bo ^((~Bu)&  Ba ); \
	    Eku0 =   Bu ^((~Ba)&  Be ); \
\
	    Abu1 ^= Du1; \
	    Ba = ROL32(Abu1, 14); \
	    Aga0 ^= Da0; \
	    Be = ROL32(Aga0, 18); \
	    Ake0 ^= De0; \
	    Bi = ROL32(Ake0, 5); \
	    Ami1 ^= Di1; \
	    Bo = ROL32(Ami1, 8); \
	    Aso0 ^= Do0; \
	    Bu = ROL32(Aso0, 28); \
	    Ema0 =   Ba ^((~Be)&  Bi ); \
	    Eme0 =   Be ^((~Bi)&  Bo ); \
	    Emi0 =   Bi ^((~Bo)&  Bu ); \
	    Emo0 =   Bo ^((~Bu)&  Ba ); \
	    Emu0 =   Bu ^((~Ba)&  Be ); \
\
	    Abi0 ^= Di0; \
	    Ba = ROL32(Abi0, 31); \
	    Ago1 ^= Do1; \
	    Be = ROL32(Ago1, 28); \
	    Aku1 ^= Du1; \
	    Bi = ROL32(Aku1, 20); \
	    Ama1 ^= Da1; \
	    Bo = ROL32(Ama1, 21); \
	    Ase0 ^= De0; \
	    Bu = ROL32(Ase0, 1); \
	    Esa0 =   Ba ^((~Be)&  Bi ); \
	    Ese0 =   Be ^((~Bi)&  Bo ); \
	    Esi0 =   Bi ^((~Bo)&  Bu ); \
	    Eso0 =   Bo ^((~Bu)&  Ba ); \
	    Esu0 =   Bu ^((~Ba)&  Be ); \
\
	    Aba1 ^= Da1; \
	    Ba = Aba1; \
	    Age1 ^= De1; \
	    Be = ROL32(Age1, 22); \
	    Aki0 ^= Di0; \
	    Bi = ROL32(Aki0, 21); \
	    Amo0 ^= Do0; \
	    Bo = ROL32(Amo0, 10); \
	    Asu1 ^= Du1; \
	    Bu = ROL32(Asu1, 7); \
	    Eba1 =   Ba ^((~Be)&  Bi ); \
	    Eba1 ^= *(pRoundConstants++); \
	    Ebe1 =   Be ^((~Bi)&  Bo ); \
	    Ebi1 =   Bi ^((~Bo)&  Bu ); \
	    Ebo1 =   Bo ^((~Bu)&  Ba ); \
	    Ebu1 =   Bu ^((~Ba)&  Be ); \
\
	    Abo1 ^= Do1; \
	    Ba = ROL32(Abo1, 14); \
	    Agu1 ^= Du1; \
	    Be = ROL32(Agu1, 10); \
	    Aka0 ^= Da0; \
	    Bi = ROL32(Aka0, 1); \
	    Ame0 ^= De0; \
	    Bo = ROL32(Ame0, 22); \
	    Asi0 ^= Di0; \
	    Bu = ROL32(Asi0, 30); \
	    Ega1 =   Ba ^((~Be)&  Bi ); \
	    Ege1 =   Be ^((~Bi)&  Bo ); \
	    Egi1 =   Bi ^((~Bo)&  Bu ); \
	    Ego1 =   Bo ^((~Bu)&  Ba ); \
	    Egu1 =   Bu ^((~Ba)&  Be ); \
\
	    Abe0 ^= De0; \
	    Ba = Abe0; \
	    Agi1 ^= Di1; \
	    Be = ROL32(Agi1, 3); \
	    Ako0 ^= Do0; \
	    Bi = ROL32(Ako0, 12); \
	    Amu1 ^= Du1; \
	    Bo = ROL32(Amu1, 4); \
	    Asa1 ^= Da1; \
	    Bu = ROL32(Asa1, 9); \
	    Eka1 =   Ba ^((~Be)&  Bi ); \
	    Eke1 =   Be ^((~Bi)&  Bo ); \
	    Eki1 =   Bi ^((~Bo)&  Bu ); \
	    Eko1 =   Bo ^((~Bu)&  Ba ); \
	    Eku1 =   Bu ^((~Ba)&  Be ); \
\
	    Abu0 ^= Du0; \
	    Ba = ROL32(Abu0, 13); \
	    Aga1 ^= Da1; \
	    Be = ROL32(Aga1, 18); \
	    Ake1 ^= De1; \
	    Bi = ROL32(Ake1, 5); \
	    Ami0 ^= Di0; \
	    Bo = ROL32(Ami0, 7); \
	    Aso1 ^= Do1; \
	    Bu = ROL32(Aso1, 28); \
	    Ema1 =   Ba ^((~Be)&  Bi ); \
	    Eme1 =   Be ^((~Bi)&  Bo ); \
	    Emi1 =   Bi ^((~Bo)&  Bu ); \
	    Emo1 =   Bo ^((~Bu)&  Ba ); \
	    Emu1 =   Bu ^((~Ba)&  Be ); \
\
	    Abi1 ^= Di1; \
	    Ba = ROL32(Abi1, 31); \
	    Ago0 ^= Do0; \
	    Be = ROL32(Ago0, 27); \
	    Aku0 ^= Du0; \
	    Bi = ROL32(Aku0, 19); \
	    Ama0 ^= Da0; \
	    Bo = ROL32(Ama0, 20); \
	    Ase1 ^= De1; \
	    Bu = ROL32(Ase1, 1); \
	    Esa1 =   Ba ^((~Be)&  Bi ); \
	    Ese1 =   Be ^((~Bi)&  Bo ); \
	    Esi1 =   Bi ^((~Bo)&  Bu ); \
	    Eso1 =   Bo ^((~Bu)&  Ba ); \
	    Esu1 =   Bu ^((~Ba)&  Be ); \
\
	    Cx = Ebu0^Egu0^Eku0^Emu0^Esu0; \
	    Du1 = Ebe1^Ege1^Eke1^Eme1^Ese1; \
	    Da0 = Cx^ROL32(Du1, 1); \
	    Cz = Ebu1^Egu1^Eku1^Emu1^Esu1; \
	    Du0 = Ebe0^Ege0^Eke0^Eme0^Ese0; \
	    Da1 = Cz^Du0; \
\
	    Cw = Ebi0^Egi0^Eki0^Emi0^Esi0; \
	    Do0 = Cw^ROL32(Cz, 1); \
	    Cy = Ebi1^Egi1^Eki1^Emi1^Esi1; \
	    Do1 = Cy^Cx; \
\
	    Cx = Eba0^Ega0^Eka0^Ema0^Esa0; \
	    De0 = Cx^ROL32(Cy, 1); \
	    Cz = Eba1^Ega1^Eka1^Ema1^Esa1; \
	    De1 = Cz^Cw; \
\
	    Cy = Ebo1^Ego1^Eko1^Emo1^Eso1; \
	    Di0 = Du0^ROL32(Cy, 1); \
	    Cw = Ebo0^Ego0^Eko0^Emo0^Eso0; \
	    Di1 = Du1^Cw; \
\
	    Du0 = Cw^ROL32(Cz, 1); \
	    Du1 = Cy^Cx; \
\
	    Eba0 ^= Da0; \
	    Ba = Eba0; \
	    Ege0 ^= De0; \
	    Be = ROL32(Ege0, 22); \
	    Eki1 ^= Di1; \
	    Bi = ROL32(Eki1, 22); \
	    Emo1 ^= Do1; \
	    Bo = ROL32(Emo1, 11); \
	    Esu0 ^= Du0; \
	    Bu = ROL32(Esu0, 7); \
	    Aba0 =   Ba ^((~Be)&  Bi ); \
	    Aba0 ^= *(pRoundConstants++); \
	    Abe0 =   Be ^((~Bi)&  Bo ); \
	    Abi0 =   Bi ^((~Bo)&  Bu ); \
	    Abo0 =   Bo ^((~Bu)&  Ba ); \
	    Abu0 =   Bu ^((~Ba)&  Be ); \
\
	    Ebo0 ^= Do0; \
	    Ba = ROL32(Ebo0, 14); \
	    Egu0 ^= Du0; \
	    Be = ROL32(Egu0, 10); \
	    Eka1 ^= Da1; \
	    Bi = ROL32(Eka1, 2); \
	    Eme1 ^= De1; \
	    Bo = ROL32(Eme1, 23); \
	    Esi1 ^= Di1; \
	    Bu = ROL32(Esi1, 31); \
	    Aga0 =   Ba ^((~Be)&  Bi ); \
	    Age0 =   Be ^((~Bi)&  Bo ); \
	    Agi0 =   Bi ^((~Bo)&  Bu ); \
	    Ago0 =   Bo ^((~Bu)&  Ba ); \
	    Agu0 =   Bu ^((~Ba)&  Be ); \
\
	    Ebe1 ^= De1; \
	    Ba = ROL32(Ebe1, 1); \
	    Egi0 ^= Di0; \
	    Be = ROL32(Egi0, 3); \
	    Eko1 ^= Do1; \
	    Bi = ROL32(Eko1, 13); \
	    Emu0 ^= Du0; \
	    Bo = ROL32(Emu0, 4); \
	    Esa0 ^= Da0; \
	    Bu = ROL32(Esa0, 9); \
	    Aka0 =   Ba ^((~Be)&  Bi ); \
	    Ake0 =   Be ^((~Bi)&  Bo ); \
	    Aki0 =   Bi ^((~Bo)&  Bu ); \
	    Ako0 =   Bo ^((~Bu)&  Ba ); \
	    Aku0 =   Bu ^((~Ba)&  Be ); \
\
	    Ebu1 ^= Du1; \
	    Ba = ROL32(Ebu1, 14); \
	    Ega0 ^= Da0; \
	    Be = ROL32(Ega0, 18); \
	    Eke0 ^= De0; \
	    Bi = ROL32(Eke0, 5); \
	    Emi1 ^= Di1; \
	    Bo = ROL32(Emi1, 8); \
	    Eso0 ^= Do0; \
	    Bu = ROL32(Eso0, 28); \
	    Ama0 =   Ba ^((~Be)&  Bi ); \
	    Ame0 =   Be ^((~Bi)&  Bo ); \
	    Ami0 =   Bi ^((~Bo)&  Bu ); \
	    Amo0 =   Bo ^((~Bu)&  Ba ); \
	    Amu0 =   Bu ^((~Ba)&  Be ); \
\
	    Ebi0 ^= Di0; \
	    Ba = ROL32(Ebi0, 31); \
	    Ego1 ^= Do1; \
	    Be = ROL32(Ego1, 28); \
	    Eku1 ^= Du1; \
	    Bi = ROL32(Eku1, 20); \
	    Ema1 ^= Da1; \
	    Bo = ROL32(Ema1, 21); \
	    Ese0 ^= De0; \
	    Bu = ROL32(Ese0, 1); \
	    Asa0 =   Ba ^((~Be)&  Bi ); \
	    Ase0 =   Be ^((~Bi)&  Bo ); \
	    Asi0 =   Bi ^((~Bo)&  Bu ); \
	    Aso0 =   Bo ^((~Bu)&  Ba ); \
	    Asu0 =   Bu ^((~Ba)&  Be ); \
\
	    Eba1 ^= Da1; \
	    Ba = Eba1; \
	    Ege1 ^= De1; \
	    Be = ROL32(Ege1, 22); \
	    Eki0 ^= Di0; \
	    Bi = ROL32(Eki0, 21); \
	    Emo0 ^= Do0; \
	    Bo = ROL32(Emo0, 10); \
	    Esu1 ^= Du1; \
	    Bu = ROL32(Esu1, 7); \
	    Aba1 =   Ba ^((~Be)&  Bi ); \
	    Aba1 ^= *(pRoundConstants++); \
	    Abe1 =   Be ^((~Bi)&  Bo ); \
	    Abi1 =   Bi ^((~Bo)&  Bu ); \
	    Abo1 =   Bo ^((~Bu)&  Ba ); \
	    Abu1 =   Bu ^((~Ba)&  Be ); \
\
	    Ebo1 ^= Do1; \
	    Ba = ROL32(Ebo1, 14); \
	    Egu1 ^= Du1; \
	    Be = ROL32(Egu1, 10); \
	    Eka0 ^= Da0; \
	    Bi = ROL32(Eka0, 1); \
	    Eme0 ^= De0; \
	    Bo = ROL32(Eme0, 22); \
	    Esi0 ^= Di0; \
	    Bu = ROL32(Esi0, 30); \
	    Aga1 =   Ba ^((~Be)&  Bi ); \
	    Age1 =   Be ^((~Bi)&  Bo ); \
	    Agi1 =   Bi ^((~Bo)&  Bu ); \
	    Ago1 =   Bo ^((~Bu)&  Ba ); \
	    Agu1 =   Bu ^((~Ba)&  Be ); \
\
	    Ebe0 ^= De0; \
	    Ba = Ebe0; \
	    Egi1 ^= Di1; \
	    Be = ROL32(Egi1, 3); \
	    Eko0 ^= Do0; \
	    Bi = ROL32(Eko0, 12); \
	    Emu1 ^= Du1; \
	    Bo = ROL32(Emu1, 4); \
	    Esa1 ^= Da1; \
	    Bu = ROL32(Esa1, 9); \
	    Aka1 =   Ba ^((~Be)&  Bi ); \
	    Ake1 =   Be ^((~Bi)&  Bo ); \
	    Aki1 =   Bi ^((~Bo)&  Bu ); \
	    Ako1 =   Bo ^((~Bu)&  Ba ); \
	    Aku1 =   Bu ^((~Ba)&  Be ); \
\
	    Ebu0 ^= Du0; \
	    Ba = ROL32(Ebu0, 13); \
	    Ega1 ^= Da1; \
	    Be = ROL32(Ega1, 18); \
	    Eke1 ^= De1; \
	    Bi = ROL32(Eke1, 5); \
	    Emi0 ^= Di0; \
	    Bo = ROL32(Emi0, 7); \
	    Eso1 ^= Do1; \
	    Bu = ROL32(Eso1, 28); \
	    Ama1 =   Ba ^((~Be)&  Bi ); \
	    Ame1 =   Be ^((~Bi)&  Bo ); \
	    Ami1 =   Bi ^((~Bo)&  Bu ); \
	    Amo1 =   Bo ^((~Bu)&  Ba ); \
	    Amu1 =   Bu ^((~Ba)&  Be ); \
\
	    Ebi1 ^= Di1; \
	    Ba = ROL32(Ebi1, 31); \
	    Ego0 ^= Do0; \
	    Be = ROL32(Ego0, 27); \
	    Eku0 ^= Du0; \
	    Bi = ROL32(Eku0, 19); \
	    Ema0 ^= Da0; \
	    Bo = ROL32(Ema0, 20); \
	    Ese1 ^= De1; \
	    Bu = ROL32(Ese1, 1); \
	    Asa1 =   Ba ^((~Be)&  Bi ); \
	    Ase1 =   Be ^((~Bi)&  Bo ); \
	    Asi1 =   Bi ^((~Bo)&  Bu ); \
	    Aso1 =   Bo ^((~Bu)&  Ba ); \
	    Asu1 =   Bu ^((~Ba)&  Be ); \
    } \
    copyToState(state, A) \
}

#define copyFromState(X, state) \
    X##ba0 = state[ 0]; \
    X##ba1 = state[ 1]; \
    X##be0 = state[ 2]; \
    X##be1 = state[ 3]; \
    X##bi0 = state[ 4]; \
    X##bi1 = state[ 5]; \
    X##bo0 = state[ 6]; \
    X##bo1 = state[ 7]; \
    X##bu0 = state[ 8]; \
    X##bu1 = state[ 9]; \
    X##ga0 = state[10]; \
    X##ga1 = state[11]; \
    X##ge0 = state[12]; \
    X##ge1 = state[13]; \
    X##gi0 = state[14]; \
    X##gi1 = state[15]; \
    X##go0 = state[16]; \
    X##go1 = state[17]; \
    X##gu0 = state[18]; \
    X##gu1 = state[19]; \
    X##ka0 = state[20]; \
    X##ka1 = state[21]; \
    X##ke0 = state[22]; \
    X##ke1 = state[23]; \
    X##ki0 = state[24]; \
    X##ki1 = state[25]; \
    X##ko0 = state[26]; \
    X##ko1 = state[27]; \
    X##ku0 = state[28]; \
    X##ku1 = state[29]; \
    X##ma0 = state[30]; \
    X##ma1 = state[31]; \
    X##me0 = state[32]; \
    X##me1 = state[33]; \
    X##mi0 = state[34]; \
    X##mi1 = state[35]; \
    X##mo0 = state[36]; \
    X##mo1 = state[37]; \
    X##mu0 = state[38]; \
    X##mu1 = state[39]; \
    X##sa0 = state[40]; \
    X##sa1 = state[41]; \
    X##se0 = state[42]; \
    X##se1 = state[43]; \
    X##si0 = state[44]; \
    X##si1 = state[45]; \
    X##so0 = state[46]; \
    X##so1 = state[47]; \
    X##su0 = state[48]; \
    X##su1 = state[49]; \

#define copyToState(state, X) \
    state[ 0] = X##ba0; \
    state[ 1] = X##ba1; \
    state[ 2] = X##be0; \
    state[ 3] = X##be1; \
    state[ 4] = X##bi0; \
    state[ 5] = X##bi1; \
    state[ 6] = X##bo0; \
    state[ 7] = X##bo1; \
    state[ 8] = X##bu0; \
    state[ 9] = X##bu1; \
    state[10] = X##ga0; \
    state[11] = X##ga1; \
    state[12] = X##ge0; \
    state[13] = X##ge1; \
    state[14] = X##gi0; \
    state[15] = X##gi1; \
    state[16] = X##go0; \
    state[17] = X##go1; \
    state[18] = X##gu0; \
    state[19] = X##gu1; \
    state[20] = X##ka0; \
    state[21] = X##ka1; \
    state[22] = X##ke0; \
    state[23] = X##ke1; \
    state[24] = X##ki0; \
    state[25] = X##ki1; \
    state[26] = X##ko0; \
    state[27] = X##ko1; \
    state[28] = X##ku0; \
    state[29] = X##ku1; \
    state[30] = X##ma0; \
    state[31] = X##ma1; \
    state[32] = X##me0; \
    state[33] = X##me1; \
    state[34] = X##mi0; \
    state[35] = X##mi1; \
    state[36] = X##mo0; \
    state[37] = X##mo1; \
    state[38] = X##mu0; \
    state[39] = X##mu1; \
    state[40] = X##sa0; \
    state[41] = X##sa1; \
    state[42] = X##se0; \
    state[43] = X##se1; \
    state[44] = X##si0; \
    state[45] = X##si1; \
    state[46] = X##so0; \
    state[47] = X##so1; \
    state[48] = X##su0; \
    state[49] = X##su1; \


// ================================================================================
// =================== KeccakF-1600-32.macros
// ================================================================================

/*
The Keccak sponge function, designed by Guido Bertoni, Joan Daemen,
Michaël Peeters and Gilles Van Assche. For more information, feedback or
questions, please refer to our website: http://keccak.noekeon.org/

Implementation by the designers,
hereby denoted as "the implementer".

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/

#ifdef UseSchedule
    #if (UseSchedule == 1)
        #include "KeccakF-1600-32-s1.macros"
    #elif (UseSchedule == 2)
        #include "KeccakF-1600-32-s2.macros"
    #elif (UseSchedule == 3)
        //#include "KeccakF-1600-32-rvk.macros"
    #else
        #error "This schedule is not supported."
    #endif
#else
    #include "KeccakF-1600-32-s1.macros"
#endif

// ================================================================================
// =================== KeccakF-1600-opt32.c
// ================================================================================


/*
The Keccak sponge function, designed by Guido Bertoni, Joan Daemen,
Michaël Peeters and Gilles Van Assche. For more information, feedback or
questions, please refer to our website: http://keccak.noekeon.org/

Implementation by the designers,
hereby denoted as "the implementer".

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/

#include <string.h>
//#include "brg_endian.h"
//#include "KeccakF-1600-opt32-settings.h"
//#include "KeccakF-1600-interface.h"



#ifdef UseInterleaveTables
int interleaveTablesBuilt = 0;
UINT16 interleaveTable[65536];
UINT16 deinterleaveTable[65536];

void buildInterleaveTables()
{
    UINT32 i, j;
    UINT16 x;

    if (!interleaveTablesBuilt) {
        for(i=0; i<65536; i++) {
            x = 0;
            for(j=0; j<16; j++) {
                if (i & (1 << j))
                    x |= (1 << (j/2 + 8*(j%2)));
            }
            interleaveTable[i] = x;
            deinterleaveTable[x] = (UINT16)i;
        }
        interleaveTablesBuilt = 1;
    }
}

#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)

#define xor2bytesIntoInterleavedWords(even, odd, source, j) \
    i##j = interleaveTable[((const UINT16*)source)[j]]; \
    ((UINT8*)even)[j] ^= i##j & 0xFF; \
    ((UINT8*)odd)[j] ^= i##j >> 8;

#define setInterleavedWordsInto2bytes(dest, even, odd, j) \
    d##j = deinterleaveTable[((even >> (j*8)) & 0xFF) ^ (((odd >> (j*8)) & 0xFF) << 8)]; \
    ((UINT16*)dest)[j] = d##j;

#else // (PLATFORM_BYTE_ORDER == IS_BIG_ENDIAN)

#define xor2bytesIntoInterleavedWords(even, odd, source, j) \
    i##j = interleaveTable[source[2*j] ^ ((UINT16)source[2*j+1] << 8)]; \
    *even ^= (i##j & 0xFF) << (j*8); \
    *odd ^= ((i##j >> 8) & 0xFF) << (j*8);

#define setInterleavedWordsInto2bytes(dest, even, odd, j) \
    d##j = deinterleaveTable[((even >> (j*8)) & 0xFF) ^ (((odd >> (j*8)) & 0xFF) << 8)]; \
    dest[2*j] = d##j & 0xFF; \
    dest[2*j+1] = d##j >> 8;

#endif // Endianness

void xor8bytesIntoInterleavedWords(UINT32 *even, UINT32 *odd, const UINT8* source)
{
    UINT16 i0, i1, i2, i3;

    xor2bytesIntoInterleavedWords(even, odd, source, 0)
    xor2bytesIntoInterleavedWords(even, odd, source, 1)
    xor2bytesIntoInterleavedWords(even, odd, source, 2)
    xor2bytesIntoInterleavedWords(even, odd, source, 3)
}

#define xorLanesIntoState(laneCount, state, input) \
    { \
        int i; \
        for(i=0; i<(laneCount); i++) \
            xor8bytesIntoInterleavedWords(state+i*2, state+i*2+1, input+i*8); \
    }

void setInterleavedWordsInto8bytes(UINT8* dest, UINT32 even, UINT32 odd)
{
    UINT16 d0, d1, d2, d3;

    setInterleavedWordsInto2bytes(dest, even, odd, 0)
    setInterleavedWordsInto2bytes(dest, even, odd, 1)
    setInterleavedWordsInto2bytes(dest, even, odd, 2)
    setInterleavedWordsInto2bytes(dest, even, odd, 3)
}

#define extractLanes(laneCount, state, data) \
    { \
        int i; \
        for(i=0; i<(laneCount); i++) \
            setInterleavedWordsInto8bytes(data+i*8, ((UINT32*)state)[i*2], ((UINT32*)state)[i*2+1]); \
    }

#else // No interleaving tables

#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)

// Credit: Henry S. Warren, Hacker's Delight, Addison-Wesley, 2002
#define xorInterleavedLE(rateInLanes, state, input) \
	{ \
		const UINT32 * pI = (const UINT32 *)input; \
		UINT32 * pS = state; \
		UINT32 t, x0, x1; \
	    int i; \
	    for (i = (rateInLanes)-1; i >= 0; --i) \
		{ \
			x0 = *(pI++); \
			t = (x0 ^ (x0 >>  1)) & 0x22222222UL;  x0 = x0 ^ t ^ (t <<  1); \
			t = (x0 ^ (x0 >>  2)) & 0x0C0C0C0CUL;  x0 = x0 ^ t ^ (t <<  2); \
			t = (x0 ^ (x0 >>  4)) & 0x00F000F0UL;  x0 = x0 ^ t ^ (t <<  4); \
			t = (x0 ^ (x0 >>  8)) & 0x0000FF00UL;  x0 = x0 ^ t ^ (t <<  8); \
 			x1 = *(pI++); \
			t = (x1 ^ (x1 >>  1)) & 0x22222222UL;  x1 = x1 ^ t ^ (t <<  1); \
			t = (x1 ^ (x1 >>  2)) & 0x0C0C0C0CUL;  x1 = x1 ^ t ^ (t <<  2); \
			t = (x1 ^ (x1 >>  4)) & 0x00F000F0UL;  x1 = x1 ^ t ^ (t <<  4); \
			t = (x1 ^ (x1 >>  8)) & 0x0000FF00UL;  x1 = x1 ^ t ^ (t <<  8); \
			*(pS++) ^= (UINT16)x0 | (x1 << 16); \
			*(pS++) ^= (x0 >> 16) | (x1 & 0xFFFF0000); \
		} \
	}

#define xorLanesIntoState(laneCount, state, input) \
    xorInterleavedLE(laneCount, state, input)

#else // (PLATFORM_BYTE_ORDER == IS_BIG_ENDIAN)

// Credit: Henry S. Warren, Hacker's Delight, Addison-Wesley, 2002
UINT64 toInterleaving(UINT64 x) 
{
   UINT64 t;

   t = (x ^ (x >>  1)) & 0x2222222222222222ULL;  x = x ^ t ^ (t <<  1);
   t = (x ^ (x >>  2)) & 0x0C0C0C0C0C0C0C0CULL;  x = x ^ t ^ (t <<  2);
   t = (x ^ (x >>  4)) & 0x00F000F000F000F0ULL;  x = x ^ t ^ (t <<  4);
   t = (x ^ (x >>  8)) & 0x0000FF000000FF00ULL;  x = x ^ t ^ (t <<  8);
   t = (x ^ (x >> 16)) & 0x00000000FFFF0000ULL;  x = x ^ t ^ (t << 16);

   return x;
}

void xor8bytesIntoInterleavedWords(UINT32* evenAndOdd, const UINT8* source)
{
    // This can be optimized
    UINT64 sourceWord =
        (UINT64)source[0]
        ^ (((UINT64)source[1]) <<  8)
        ^ (((UINT64)source[2]) << 16)
        ^ (((UINT64)source[3]) << 24)
        ^ (((UINT64)source[4]) << 32)
        ^ (((UINT64)source[5]) << 40)
        ^ (((UINT64)source[6]) << 48)
        ^ (((UINT64)source[7]) << 56);
    UINT64 evenAndOddWord = toInterleaving(sourceWord);
    evenAndOdd[0] ^= (UINT32)evenAndOddWord;
    evenAndOdd[1] ^= (UINT32)(evenAndOddWord >> 32);
}

#define xorLanesIntoState(laneCount, state, input) \
    { \
        int i; \
        for(i=0; i<(laneCount); i++) \
            xor8bytesIntoInterleavedWords(state+i*2, input+i*8); \
    }

#endif // Endianness

// Credit: Henry S. Warren, Hacker's Delight, Addison-Wesley, 2002
UINT64 fromInterleaving(UINT64 x)
{
   UINT64 t;

   t = (x ^ (x >> 16)) & 0x00000000FFFF0000ULL;  x = x ^ t ^ (t << 16);
   t = (x ^ (x >>  8)) & 0x0000FF000000FF00ULL;  x = x ^ t ^ (t <<  8);
   t = (x ^ (x >>  4)) & 0x00F000F000F000F0ULL;  x = x ^ t ^ (t <<  4);
   t = (x ^ (x >>  2)) & 0x0C0C0C0C0C0C0C0CULL;  x = x ^ t ^ (t <<  2);
   t = (x ^ (x >>  1)) & 0x2222222222222222ULL;  x = x ^ t ^ (t <<  1);

   return x;
}

void setInterleavedWordsInto8bytes(UINT8* dest, UINT32* evenAndOdd)
{
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
    ((UINT64*)dest)[0] = fromInterleaving(*(UINT64*)evenAndOdd);
#else // (PLATFORM_BYTE_ORDER == IS_BIG_ENDIAN)
    // This can be optimized
    UINT64 evenAndOddWord = (UINT64)evenAndOdd[0] ^ ((UINT64)evenAndOdd[1] << 32);
    UINT64 destWord = fromInterleaving(evenAndOddWord);
    dest[0] = destWord & 0xFF;
    dest[1] = (destWord >> 8) & 0xFF;
    dest[2] = (destWord >> 16) & 0xFF;
    dest[3] = (destWord >> 24) & 0xFF;
    dest[4] = (destWord >> 32) & 0xFF;
    dest[5] = (destWord >> 40) & 0xFF;
    dest[6] = (destWord >> 48) & 0xFF;
    dest[7] = (destWord >> 56) & 0xFF;
#endif // Endianness
}

#define extractLanes(laneCount, state, data) \
    { \
        int i; \
        for(i=0; i<(laneCount); i++) \
            setInterleavedWordsInto8bytes(data+i*8, (UINT32*)state+i*2); \
    }

#endif // With or without interleaving tables

#if defined(_MSC_VER)
#define ROL32(a, offset) _rotl(a, offset)
#elif (defined (__arm__) && defined(__ARMCC_VERSION))
#define ROL32(a, offset) __ror(a, 32-(offset))
#else
#define ROL32(a, offset) ((((UINT32)a) << (offset)) ^ (((UINT32)a) >> (32-(offset))))
#endif

//#include "KeccakF-1600-unrolling.macros"
//#include "KeccakF-1600-32.macros"

#if (UseSchedule == 3)

#ifdef UseBebigokimisa
#error "No lane complementing with schedule 3."
#endif

#if (Unrolling != 2)
#error "Only unrolling 2 is supported by schedule 3."
#endif

void KeccakPermutationOnWords(UINT32 *state)
{
    rounds
}

void KeccakPermutationOnWordsAfterXoring(UINT32 *state, const UINT8 *input, unsigned int laneCount)
{
    xorLanesIntoState(laneCount, state, input)
    rounds
}

#ifdef ProvideFast576
void KeccakPermutationOnWordsAfterXoring576bits(UINT32 *state, const UINT8 *input)
{
    xorLanesIntoState(9, state, input)
    rounds
}
#endif

#ifdef ProvideFast832
void KeccakPermutationOnWordsAfterXoring832bits(UINT32 *state, const UINT8 *input)
{
    xorLanesIntoState(13, state, input)
    rounds
}
#endif

#ifdef ProvideFast1024
void KeccakPermutationOnWordsAfterXoring1024bits(UINT32 *state, const UINT8 *input)
{
    xorLanesIntoState(16, state, input)
    rounds
}
#endif

#ifdef ProvideFast1088
void KeccakPermutationOnWordsAfterXoring1088bits(UINT32 *state, const UINT8 *input)
{
    xorLanesIntoState(17, state, input)
    rounds
}
#endif

#ifdef ProvideFast1152
void KeccakPermutationOnWordsAfterXoring1152bits(UINT32 *state, const UINT8 *input)
{
    xorLanesIntoState(18, state, input)
    rounds
}
#endif

#ifdef ProvideFast1344
void KeccakPermutationOnWordsAfterXoring1344bits(UINT32 *state, const UINT8 *input)
{
    xorLanesIntoState(21, state, input)
    rounds
}
#endif

#else // (Schedule != 3)

void KeccakPermutationOnWords(UINT32 *state)
{
    declareABCDE
#if (Unrolling != 24)
    unsigned int i;
#endif

    copyFromState(A, state)
    rounds
}

void KeccakPermutationOnWordsAfterXoring(UINT32 *state, const UINT8 *input, unsigned int laneCount)
{
    declareABCDE
    unsigned int i;

    xorLanesIntoState(laneCount, state, input)
    copyFromState(A, state)
    rounds
}

#ifdef ProvideFast576
void KeccakPermutationOnWordsAfterXoring576bits(UINT32 *state, const UINT8 *input)
{
    declareABCDE
    unsigned int i;

    xorLanesIntoState(9, state, input)
    copyFromState(A, state)
    rounds
}
#endif

#ifdef ProvideFast832
void KeccakPermutationOnWordsAfterXoring832bits(UINT32 *state, const UINT8 *input)
{
    declareABCDE
    unsigned int i;

    xorLanesIntoState(13, state, input)
    copyFromState(A, state)
    rounds
}
#endif

#ifdef ProvideFast1024
void KeccakPermutationOnWordsAfterXoring1024bits(UINT32 *state, const UINT8 *input)
{
    declareABCDE
    unsigned int i;

    xorLanesIntoState(16, state, input)
    copyFromState(A, state)
    rounds
}
#endif

#ifdef ProvideFast1088
void KeccakPermutationOnWordsAfterXoring1088bits(UINT32 *state, const UINT8 *input)
{
    declareABCDE
    unsigned int i;

    xorLanesIntoState(17, state, input)
    copyFromState(A, state)
    rounds
}
#endif

#ifdef ProvideFast1152
void KeccakPermutationOnWordsAfterXoring1152bits(UINT32 *state, const UINT8 *input)
{
    declareABCDE
    unsigned int i;

    xorLanesIntoState(18, state, input)
    copyFromState(A, state)
    rounds
}
#endif

#ifdef ProvideFast1344
void KeccakPermutationOnWordsAfterXoring1344bits(UINT32 *state, const UINT8 *input)
{
    declareABCDE
    unsigned int i;

    xorLanesIntoState(21, state, input)
    copyFromState(A, state)
    rounds
}
#endif

#endif

void KeccakInitialize()
{
#ifdef UseInterleaveTables
    buildInterleaveTables();
#endif
}

void KeccakInitializeState(unsigned char *state)
{
    memset(state, 0, 200);
#ifdef UseBebigokimisa
    ((UINT32*)state)[ 2] = ~(UINT32)0;
    ((UINT32*)state)[ 3] = ~(UINT32)0;
    ((UINT32*)state)[ 4] = ~(UINT32)0;
    ((UINT32*)state)[ 5] = ~(UINT32)0;
    ((UINT32*)state)[16] = ~(UINT32)0;
    ((UINT32*)state)[17] = ~(UINT32)0;
    ((UINT32*)state)[24] = ~(UINT32)0;
    ((UINT32*)state)[25] = ~(UINT32)0;
    ((UINT32*)state)[34] = ~(UINT32)0;
    ((UINT32*)state)[35] = ~(UINT32)0;
    ((UINT32*)state)[40] = ~(UINT32)0;
    ((UINT32*)state)[41] = ~(UINT32)0;
#endif
}

void KeccakPermutation(unsigned char *state)
{
    // We assume the state is always stored as interleaved 32-bit words
    KeccakPermutationOnWords((UINT32*)state);
}

#ifdef ProvideFast576
void KeccakAbsorb576bits(unsigned char *state, const unsigned char *data)
{
    KeccakPermutationOnWordsAfterXoring576bits((UINT32*)state, data);
}
#endif

#ifdef ProvideFast832
void KeccakAbsorb832bits(unsigned char *state, const unsigned char *data)
{
    KeccakPermutationOnWordsAfterXoring832bits((UINT32*)state, data);
}
#endif

#ifdef ProvideFast1024
void KeccakAbsorb1024bits(unsigned char *state, const unsigned char *data)
{
    KeccakPermutationOnWordsAfterXoring1024bits((UINT32*)state, data);
}
#endif

#ifdef ProvideFast1088
void KeccakAbsorb1088bits(unsigned char *state, const unsigned char *data)
{
    KeccakPermutationOnWordsAfterXoring1088bits((UINT32*)state, data);
}
#endif

#ifdef ProvideFast1152
void KeccakAbsorb1152bits(unsigned char *state, const unsigned char *data)
{
    KeccakPermutationOnWordsAfterXoring1152bits((UINT32*)state, data);
}
#endif

#ifdef ProvideFast1344
void KeccakAbsorb1344bits(unsigned char *state, const unsigned char *data)
{
    KeccakPermutationOnWordsAfterXoring1344bits((UINT32*)state, data);
}
#endif

void KeccakAbsorb(unsigned char *state, const unsigned char *data, unsigned int laneCount)
{
    KeccakPermutationOnWordsAfterXoring((UINT32*)state, data, laneCount);
}

#ifdef ProvideFast1024
void KeccakExtract1024bits(const unsigned char *state, unsigned char *data)
{
    extractLanes(16, state, data)
#ifdef UseBebigokimisa
    ((UINT32*)data)[ 2] = ~((UINT32*)data)[ 2];
    ((UINT32*)data)[ 3] = ~((UINT32*)data)[ 3];
    ((UINT32*)data)[ 4] = ~((UINT32*)data)[ 4];
    ((UINT32*)data)[ 5] = ~((UINT32*)data)[ 5];
    ((UINT32*)data)[16] = ~((UINT32*)data)[16];
    ((UINT32*)data)[17] = ~((UINT32*)data)[17];
    ((UINT32*)data)[24] = ~((UINT32*)data)[24];
    ((UINT32*)data)[25] = ~((UINT32*)data)[25];
#endif
}
#endif

void KeccakExtract(const unsigned char *state, unsigned char *data, unsigned int laneCount)
{
    extractLanes((int)laneCount, state, data)
#ifdef UseBebigokimisa
    if (laneCount > 1) {
        ((UINT32*)data)[ 2] = ~((UINT32*)data)[ 2];
        ((UINT32*)data)[ 3] = ~((UINT32*)data)[ 3];
        if (laneCount > 2) {
            ((UINT32*)data)[ 4] = ~((UINT32*)data)[ 4];
            ((UINT32*)data)[ 5] = ~((UINT32*)data)[ 5];
            if (laneCount > 8) {
                ((UINT32*)data)[16] = ~((UINT32*)data)[16];
                ((UINT32*)data)[17] = ~((UINT32*)data)[17];
                if (laneCount > 12) {
                    ((UINT32*)data)[24] = ~((UINT32*)data)[24];
                    ((UINT32*)data)[25] = ~((UINT32*)data)[25];
                    if (laneCount > 17) {
                        ((UINT32*)data)[34] = ~((UINT32*)data)[34];
                        ((UINT32*)data)[35] = ~((UINT32*)data)[35];
                        if (laneCount > 20) {
                            ((UINT32*)data)[40] = ~((UINT32*)data)[40];
                            ((UINT32*)data)[41] = ~((UINT32*)data)[41];
                        }
                    }
                }
            }
        }
    }
#endif
}


// ================================================================================
// =================== KeccakSponge.c
// ================================================================================


/*
The Keccak sponge function, designed by Guido Bertoni, Joan Daemen,
Michaël Peeters and Gilles Van Assche. For more information, feedback or
questions, please refer to our website: http://keccak.noekeon.org/

Implementation by the designers,
hereby denoted as "the implementer".

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/

#include <string.h>
//#include "KeccakSponge.h"
//#include "KeccakF-1600-interface.h"
#ifdef KeccakReference
//#include "displayIntermediateValues.h"
#endif

int InitSponge(spongeState *state, unsigned int rate, unsigned int capacity)
{
    if (rate+capacity != 1600)
        return 1;
    if ((rate <= 0) || (rate >= 1600) || ((rate % 64) != 0))
        return 1;
    KeccakInitialize();
    state->rate = rate;
    state->capacity = capacity;
    state->fixedOutputLength = 0;
    KeccakInitializeState(state->state);
    memset(state->dataQueue, 0, KeccakMaximumRateInBytes);
    state->bitsInQueue = 0;
    state->squeezing = 0;
    state->bitsAvailableForSqueezing = 0;

    return 0;
}

void AbsorbQueue(spongeState *state)
{
    // state->bitsInQueue is assumed to be equal to state->rate
    #ifdef KeccakReference
    displayBytes(1, "Block to be absorbed", state->dataQueue, state->rate/8);
    #endif
#ifdef ProvideFast576
    if (state->rate == 576)
        KeccakAbsorb576bits(state->state, state->dataQueue);
    else 
#endif
#ifdef ProvideFast832
    if (state->rate == 832)
        KeccakAbsorb832bits(state->state, state->dataQueue);
    else 
#endif
#ifdef ProvideFast1024
    if (state->rate == 1024)
        KeccakAbsorb1024bits(state->state, state->dataQueue);
    else 
#endif
#ifdef ProvideFast1088
    if (state->rate == 1088)
        KeccakAbsorb1088bits(state->state, state->dataQueue);
    else
#endif
#ifdef ProvideFast1152
    if (state->rate == 1152)
        KeccakAbsorb1152bits(state->state, state->dataQueue);
    else 
#endif
#ifdef ProvideFast1344
    if (state->rate == 1344)
        KeccakAbsorb1344bits(state->state, state->dataQueue);
    else 
#endif
        KeccakAbsorb(state->state, state->dataQueue, state->rate/64);
    state->bitsInQueue = 0;
}

int Absorb(spongeState *state, const unsigned char *data, unsigned long long databitlen)
{
    unsigned long long i, j, wholeBlocks;
    unsigned int partialBlock, partialByte;
    const unsigned char *curData;

    if ((state->bitsInQueue % 8) != 0)
        return 1; // Only the last call may contain a partial byte
    if (state->squeezing)
        return 1; // Too late for additional input

    i = 0;
    while(i < databitlen) {
        if ((state->bitsInQueue == 0) && (databitlen >= state->rate) && (i <= (databitlen-state->rate))) {
            wholeBlocks = (databitlen-i)/state->rate;
            curData = data+i/8;
#ifdef ProvideFast576
            if (state->rate == 576) {
                for(j=0; j<wholeBlocks; j++, curData+=576/8) {
                    #ifdef KeccakReference
                    displayBytes(1, "Block to be absorbed", curData, state->rate/8);
                    #endif
                    KeccakAbsorb576bits(state->state, curData);
                }
            }
            else
#endif
#ifdef ProvideFast832
            if (state->rate == 832) {
                for(j=0; j<wholeBlocks; j++, curData+=832/8) {
                    #ifdef KeccakReference
                    displayBytes(1, "Block to be absorbed", curData, state->rate/8);
                    #endif
                    KeccakAbsorb832bits(state->state, curData);
                }
            }
            else
#endif
#ifdef ProvideFast1024
            if (state->rate == 1024) {
                for(j=0; j<wholeBlocks; j++, curData+=1024/8) {
                    #ifdef KeccakReference
                    displayBytes(1, "Block to be absorbed", curData, state->rate/8);
                    #endif
                    KeccakAbsorb1024bits(state->state, curData);
                }
            }
            else
#endif
#ifdef ProvideFast1088
            if (state->rate == 1088) {
                for(j=0; j<wholeBlocks; j++, curData+=1088/8) {
                    #ifdef KeccakReference
                    displayBytes(1, "Block to be absorbed", curData, state->rate/8);
                    #endif
                    KeccakAbsorb1088bits(state->state, curData);
                }
            }
            else
#endif
#ifdef ProvideFast1152
            if (state->rate == 1152) {
                for(j=0; j<wholeBlocks; j++, curData+=1152/8) {
                    #ifdef KeccakReference
                    displayBytes(1, "Block to be absorbed", curData, state->rate/8);
                    #endif
                    KeccakAbsorb1152bits(state->state, curData);
                }
            }
            else
#endif
#ifdef ProvideFast1344
            if (state->rate == 1344) {
                for(j=0; j<wholeBlocks; j++, curData+=1344/8) {
                    #ifdef KeccakReference
                    displayBytes(1, "Block to be absorbed", curData, state->rate/8);
                    #endif
                    KeccakAbsorb1344bits(state->state, curData);
                }
            }
            else
#endif
            {
                for(j=0; j<wholeBlocks; j++, curData+=state->rate/8) {
                    #ifdef KeccakReference
                    displayBytes(1, "Block to be absorbed", curData, state->rate/8);
                    #endif
                    KeccakAbsorb(state->state, curData, state->rate/64);
                }
            }
            i += wholeBlocks*state->rate;
        }
        else {
            partialBlock = (unsigned int)(databitlen - i);
            if (partialBlock+state->bitsInQueue > state->rate)
                partialBlock = state->rate-state->bitsInQueue;
            partialByte = partialBlock % 8;
            partialBlock -= partialByte;
            memcpy(state->dataQueue+state->bitsInQueue/8, data+i/8, partialBlock/8);
            state->bitsInQueue += partialBlock;
            i += partialBlock;
            if (state->bitsInQueue == state->rate)
                AbsorbQueue(state);
            if (partialByte > 0) {
                unsigned char mask = (1 << partialByte)-1;
                state->dataQueue[state->bitsInQueue/8] = data[i/8] & mask;
                state->bitsInQueue += partialByte;
                i += partialByte;
            }
        }
    }
    return 0;
}

void PadAndSwitchToSqueezingPhase(spongeState *state)
{
    // Note: the bits are numbered from 0=LSB to 7=MSB
    if (state->bitsInQueue + 1 == state->rate) {
        state->dataQueue[state->bitsInQueue/8 ] |= 1 << (state->bitsInQueue % 8);
        AbsorbQueue(state);
        memset(state->dataQueue, 0, state->rate/8);
    }
    else {
        memset(state->dataQueue + (state->bitsInQueue+7)/8, 0, state->rate/8 - (state->bitsInQueue+7)/8);
        state->dataQueue[state->bitsInQueue/8 ] |= 1 << (state->bitsInQueue % 8);
    }
    state->dataQueue[(state->rate-1)/8] |= 1 << ((state->rate-1) % 8);
    AbsorbQueue(state);

    #ifdef KeccakReference
    displayText(1, "--- Switching to squeezing phase ---");
    #endif
#ifdef ProvideFast1024
    if (state->rate == 1024) {
        KeccakExtract1024bits(state->state, state->dataQueue);
        state->bitsAvailableForSqueezing = 1024;
    }
    else
#endif
    {
        KeccakExtract(state->state, state->dataQueue, state->rate/64);
        state->bitsAvailableForSqueezing = state->rate;
    }
    #ifdef KeccakReference
    displayBytes(1, "Block available for squeezing", state->dataQueue, state->bitsAvailableForSqueezing/8);
    #endif
    state->squeezing = 1;
}

int Squeeze(spongeState *state, unsigned char *output, unsigned long long outputLength)
{
    unsigned long long i;
    unsigned int partialBlock;

    if (!state->squeezing)
        PadAndSwitchToSqueezingPhase(state);
    if ((outputLength % 8) != 0)
        return 1; // Only multiple of 8 bits are allowed, truncation can be done at user level

    i = 0;
    while(i < outputLength) {
        if (state->bitsAvailableForSqueezing == 0) {
            KeccakPermutation(state->state);
#ifdef ProvideFast1024
            if (state->rate == 1024) {
                KeccakExtract1024bits(state->state, state->dataQueue);
                state->bitsAvailableForSqueezing = 1024;
            }
            else
#endif
            {
                KeccakExtract(state->state, state->dataQueue, state->rate/64);
                state->bitsAvailableForSqueezing = state->rate;
            }
            #ifdef KeccakReference
            displayBytes(1, "Block available for squeezing", state->dataQueue, state->bitsAvailableForSqueezing/8);
            #endif
        }
        partialBlock = state->bitsAvailableForSqueezing;
        if ((unsigned long long)partialBlock > outputLength - i)
            partialBlock = (unsigned int)(outputLength - i);
        memcpy(output+i/8, state->dataQueue+(state->rate-state->bitsAvailableForSqueezing)/8, partialBlock/8);
        state->bitsAvailableForSqueezing -= partialBlock;
        i += partialBlock;
    }
    return 0;
}



// ================================================================================
// =================== KeccakNISTInterface.h
// ================================================================================


/*
The Keccak sponge function, designed by Guido Bertoni, Joan Daemen,
Michaël Peeters and Gilles Van Assche. For more information, feedback or
questions, please refer to our website: http://keccak.noekeon.org/

Implementation by the designers,
hereby denoted as "the implementer".

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/

#ifndef _KeccakNISTInterface_h_
#define _KeccakNISTInterface_h_

//#include "KeccakSponge.h"

typedef unsigned char BitSequence;
typedef unsigned long long DataLength;
typedef enum { SUCCESS = 0, FAIL = 1, BAD_HASHLEN = 2 } HashReturn;

typedef spongeState hashState;

/**
  * Function to initialize the state of the Keccak[r, c] sponge function.
  * The rate r and capacity c values are determined from @a hashbitlen.
  * @param  state       Pointer to the state of the sponge function to be initialized.
  * @param  hashbitlen  The desired number of output bits, 
  *                     or 0 for Keccak[] with default parameters
  *                     and arbitrarily-long output.
  * @pre    The value of hashbitlen must be one of 0, 224, 256, 384 and 512.
  * @return SUCCESS if successful, BAD_HASHLEN if the value of hashbitlen is incorrect.
  */
HashReturn Init(hashState *state, int hashbitlen);
/**
  * Function to give input data for the sponge function to absorb.
  * @param  state       Pointer to the state of the sponge function initialized by Init().
  * @param  data        Pointer to the input data. 
  *                     When @a databitLen is not a multiple of 8, the last bits of data must be
  *                     in the most significant bits of the last byte.
  * @param  databitLen  The number of input bits provided in the input data.
  * @pre    In the previous call to Absorb(), databitLen was a multiple of 8.
  * @return SUCCESS if successful, FAIL otherwise.
  */
HashReturn Update(hashState *state, const BitSequence *data, DataLength databitlen);
/**
  * Function to squeeze output data from the sponge function.
  * If @a hashbitlen was not 0 in the call to Init(), the number of output bits is equal to @a hashbitlen.
  * If @a hashbitlen was 0 in the call to Init(), the output bits must be extracted using the Squeeze() function.
  * @param  state       Pointer to the state of the sponge function initialized by Init().
  * @param  hashval     Pointer to the buffer where to store the output data.
  * @return SUCCESS if successful, FAIL otherwise.
  */
HashReturn Final(hashState *state, BitSequence *hashval);
/**
  * Function to compute a hash using the Keccak[r, c] sponge function.
  * The rate r and capacity c values are determined from @a hashbitlen.
  * @param  hashbitlen  The desired number of output bits.
  * @param  data        Pointer to the input data. 
  *                     When @a databitLen is not a multiple of 8, the last bits of data must be
  *                     in the most significant bits of the last byte.
  * @param  databitLen  The number of input bits provided in the input data.
  * @param  hashval     Pointer to the buffer where to store the output data.
  * @pre    The value of hashbitlen must be one of 224, 256, 384 and 512.
  * @return SUCCESS if successful, BAD_HASHLEN if the value of hashbitlen is incorrect.
  */
HashReturn Hash(int hashbitlen, const BitSequence *data, DataLength databitlen, BitSequence *hashval);

#endif


// ================================================================================
// =================== KeccakNISTInterface.c
// ================================================================================



/*
The Keccak sponge function, designed by Guido Bertoni, Joan Daemen,
Michaël Peeters and Gilles Van Assche. For more information, feedback or
questions, please refer to our website: http://keccak.noekeon.org/

Implementation by the designers,
hereby denoted as "the implementer".

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/

#include <string.h>
//#include "KeccakNISTInterface.h"
//#include "KeccakF-1600-interface.h"

HashReturn Init(hashState *state, int hashbitlen)
{
    switch(hashbitlen) {
        case 0: // Default parameters, arbitrary length output
            InitSponge((spongeState*)state, 1024, 576);
            break;
        case 224:
            InitSponge((spongeState*)state, 1152, 448);
            break;
        case 256:
            InitSponge((spongeState*)state, 1088, 512);
            break;
        case 384:
            InitSponge((spongeState*)state, 832, 768);
            break;
        case 512:
            InitSponge((spongeState*)state, 576, 1024);
            break;
        default:
            return BAD_HASHLEN;
    }
    state->fixedOutputLength = hashbitlen;
    return SUCCESS;
}

HashReturn Update(hashState *state, const BitSequence *data, DataLength databitlen)
{
    if ((databitlen % 8) == 0)
        return (HashReturn)Absorb((spongeState*)state, data, databitlen);
    else {
        HashReturn ret = (HashReturn)Absorb((spongeState*)state, data, databitlen - (databitlen % 8));
        if (ret == SUCCESS) {
            unsigned char lastByte; 
            // Align the last partial byte to the least significant bits
            lastByte = data[databitlen/8] >> (8 - (databitlen % 8));
            return (HashReturn)Absorb((spongeState*)state, &lastByte, databitlen % 8);
        }
        else
            return ret;
    }
}

HashReturn Final(hashState *state, BitSequence *hashval)
{
    return (HashReturn)Squeeze(state, hashval, state->fixedOutputLength);
}

HashReturn Hash(int hashbitlen, const BitSequence *data, DataLength databitlen, BitSequence *hashval)
{
    hashState state;
    HashReturn result;

    if ((hashbitlen != 224) && (hashbitlen != 256) && (hashbitlen != 384) && (hashbitlen != 512))
        return BAD_HASHLEN; // Only the four fixed output lengths available through this API
    result = Init(&state, hashbitlen);
    if (result != SUCCESS)
        return result;
    result = Update(&state, data, databitlen);
    if (result != SUCCESS)
        return result;
    result = Final(&state, hashval);
    return result;
}


} // end of namespace KeccakImpl

