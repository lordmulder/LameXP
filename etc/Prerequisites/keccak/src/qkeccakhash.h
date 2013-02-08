/***************************************************************************
**                                                                        **
**  QKeccakHash, an API wrapper bringing the optimized implementation of  **
**  Keccak (http://keccak.noekeon.org/) to Qt.                            **
**  Copyright (C) 2013 Emanuel Eichhammer                                 **
**                                                                        **
**  This program is free software: you can redistribute it and/or modify  **
**  it under the terms of the GNU General Public License as published by  **
**  the Free Software Foundation, either version 3 of the License, or     **
**  (at your option) any later version.                                   **
**                                                                        **
**  This program is distributed in the hope that it will be useful,       **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of        **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
**  GNU General Public License for more details.                          **
**                                                                        **
**  You should have received a copy of the GNU General Public License     **
**  along with this program.  If not, see http://www.gnu.org/licenses/.   **
**                                                                        **
****************************************************************************
**           Author: Emanuel Eichhammer                                   **
**  Website/Contact: http://www.WorksLikeClockwork.com/                   **
**             Date: 12.01.12                                             **
****************************************************************************/

#ifndef QKECCAKHASH_H
#define QKECCAKHASH_H

#include <QString>
#include <QByteArray>
#include <QFile>

// Section from KeccakSponge.h
// needed here, since hashState needs to be explicitly 32-byte aligned and therefore can't be
// transformed into a class (in order to forward declarate) like in the other hash wrappers.
namespace KeccakImpl
{
	#define KeccakPermutationSize 1600
	#define KeccakPermutationSizeInBytes (KeccakPermutationSize/8)
	#define KeccakMaximumRate 1536
	#define KeccakMaximumRateInBytes (KeccakMaximumRate/8)

	#if defined(__GNUC__)
	#define ALIGN __attribute__ ((aligned(32)))
	#elif defined(_MSC_VER)
	#define ALIGN __declspec(align(32))
	#else
	#define ALIGN
	#endif

	ALIGN typedef struct spongeStateStruct {
		ALIGN unsigned char state[KeccakPermutationSizeInBytes];
		ALIGN unsigned char dataQueue[KeccakMaximumRateInBytes];
		unsigned int rate;
		unsigned int capacity;
		unsigned int bitsInQueue;
		unsigned int fixedOutputLength;
		int squeezing;
		unsigned int bitsAvailableForSqueezing;
	} spongeState;
	typedef spongeState hashState;
}
// End Section from KeccakSponge.h

class QKeccakHash
{
public:
	enum HashBits {hb224, hb256, hb384, hb512};
	
	QKeccakHash();
	~QKeccakHash();
	
	static bool selfTest(void);

	bool init(HashBits hashBits=hb256);
	bool addData(const QByteArray &data);
	bool addData(const char *data, int size);
	const QByteArray &finalize();

protected:
	bool m_initialized;
	KeccakImpl::hashState *m_state;
	QByteArray m_hashResult;
};

#endif // QKECCAKHASH_H
