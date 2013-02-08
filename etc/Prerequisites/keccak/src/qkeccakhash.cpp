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

#include "qkeccakhash.h"
#include <QDebug>

#include "keccakimpl.cpp"

QKeccakHash::QKeccakHash()
{
	m_initialized = false;
	m_state = new KeccakImpl::hashState;
	memset(m_state, 0, sizeof(KeccakImpl::hashState));
	m_hashResult.clear();
}

QKeccakHash::~QKeccakHash()
{
	m_hashResult.clear();

	if(m_state)
	{
		delete m_state;
		m_state = NULL;
	}
}

bool QKeccakHash::init(HashBits hashBits)
{
	if(m_initialized)
	{
		qWarning("QKeccakHash has already been initialized!");
		return false;
	}

	m_hashResult.clear();
	memset(m_state, 0, sizeof(KeccakImpl::hashState));
	int hashBitLength = 0;

	switch (hashBits)
	{
		case hb224: hashBitLength = 224; break;
		case hb256: hashBitLength = 256; break;
		case hb384: hashBitLength = 384; break;
		case hb512: hashBitLength = 512; break;
		default: throw "Invalid hash length!!";
	}

	if(KeccakImpl::Init(m_state, hashBitLength) != KeccakImpl::SUCCESS)
	{
		qWarning("KeccakImpl::Init() has failed unexpectedly!");
		return false;
	}
	
	m_hashResult.fill(char(0), hashBitLength/8);
	m_initialized = true;

	return true;
}

bool QKeccakHash::addData(const QByteArray &data)
{
	return addData(data.constData(), data.size());
}

bool QKeccakHash::addData(const char *data, int size)
{
	if(!m_initialized)
	{
		qWarning("QKeccakHash has not been initialized yet!");
		return false;
	}
	
	if(KeccakImpl::Update(m_state, (KeccakImpl::BitSequence*)data, size*8) != KeccakImpl::SUCCESS)
	{
		qWarning("KeccakImpl::Update() has failed unexpectedly!");
		m_hashResult.clear();
		m_initialized = false;
		return false;
	}
	
	return true;
}

const QByteArray &QKeccakHash::finalize()
{
	if(!m_initialized)
	{
		qWarning("QKeccakHash has not been initialized yet!");
		m_hashResult.clear();
		return m_hashResult;
	}

	if(KeccakImpl::Final(m_state, (KeccakImpl::BitSequence*)m_hashResult.data()) != KeccakImpl::SUCCESS)
	{
		qWarning("KeccakImpl::Final() has failed unexpectedly!");
		m_hashResult.clear();
	}

	m_initialized = false;
	return m_hashResult;
}

bool QKeccakHash::selfTest(void)
{
	QKeccakHash hash;
	const QByteArray input("The quick brown fox jumps over the lazy dog");
	bool passed[4] = {false, false, false, false};

	if(hash.init(QKeccakHash::hb224))
	{
		if(hash.addData(input))
		{
			QByteArray result = hash.finalize();
			if(!result.isEmpty())
			{
				passed[0] = (_stricmp(result.toHex().constData(), "310aee6b30c47350576ac2873fa89fd190cdc488442f3ef654cf23fe") == 0);
				if(!passed[0]) qWarning("QKeccakHash self-test: Test #1 failed !!!");
			}
		}
	}

	if(hash.init(QKeccakHash::hb256))
	{
		if(hash.addData(input))
		{
			QByteArray result = hash.finalize();
			if(!result.isEmpty())
			{
				passed[1] = (_stricmp(result.toHex().constData(), "4d741b6f1eb29cb2a9b9911c82f56fa8d73b04959d3d9d222895df6c0b28aa15") == 0);
				if(!passed[1]) qWarning("QKeccakHash self-test: Test #2 failed !!!");
			}
		}
	}
	
	if(hash.init(QKeccakHash::hb384))
	{
		if(hash.addData(input))
		{
			QByteArray result = hash.finalize();
			if(!result.isEmpty())
			{
				passed[2] = (_stricmp(result.toHex().constData(), "283990fa9d5fb731d786c5bbee94ea4db4910f18c62c03d173fc0a5e494422e8a0b3da7574dae7fa0baf005e504063b3") == 0);
				if(!passed[2]) qWarning("QKeccakHash self-test: Test #3 failed !!!");
			}
		}
	}

	if(hash.init(QKeccakHash::hb512))
	{
		if(hash.addData(input))
		{
			QByteArray result = hash.finalize();
			if(!result.isEmpty())
			{
				passed[3] = (_stricmp(result.toHex().constData(), "d135bb84d0439dbac432247ee573a23ea7d3c9deb2a968eb31d47c4fb45f1ef4422d6c531b5b9bd6f449ebcc449ea94d0a8f05f62130fda612da53c79659f609") == 0);
				if(!passed[3]) qWarning("QKeccakHash self-test: Test #4 failed !!!");
			}
		}
	}

	return (passed[0] && passed[1] && passed[2] && passed[3]);
}
