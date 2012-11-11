/***************************************************************************
**                                                                        **
**  QKeccakHash, an API wrapper bringing the optimized implementation of  **
**  Keccak (http://keccak.noekeon.org/) to Qt.                            **
**  Copyright (C) 2012 Emanuel Eichhammer                                 **
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

QKeccakHash::QKeccakHash() :
  mState(0),
  mHashBitLength(0)
{
}

QKeccakHash::QKeccakHash(const QString &asciiMessage, HashBits hashBits) :
  mState(0),
  mHashBitLength(0)
{
  if (!setHashBitLength(hashBits))
    return;
  
  bool success = false;
  QByteArray msg = asciiMessage.toAscii();
  
  mState = new KeccakImpl::hashState;
  if (KeccakImpl::Init(mState, mHashBitLength) == KeccakImpl::SUCCESS)
  {
    if (KeccakImpl::Update(mState, (KeccakImpl::BitSequence*)msg.constData(), msg.size()*8) == KeccakImpl::SUCCESS)
    {
      if (KeccakImpl::Final(mState, (KeccakImpl::BitSequence*)mHashResult.data()) == KeccakImpl::SUCCESS)
        success = true;
    }
  }
  delete mState;
  mState = 0;
  
  if (!success)
  {
    mHashResult.clear();
    qDebug() << "QKeccakHash::QKeccakHash(): hash construction failed";
  }
}

QKeccakHash::~QKeccakHash()
{
  if (mState)
    delete mState;
}

QByteArray QKeccakHash::toRaw() const
{
  return mHashResult;
}

QByteArray QKeccakHash::toHex() const
{
  return mHashResult.toHex();
}

QString QKeccakHash::toHexString() const
{
  return QString(mHashResult.toHex());
}

bool QKeccakHash::file(const QString &fileName, HashBits hashBits, int blockSize)
{
  if (isBatchRunning() || blockSize < 1)
    return false;
  if (!setHashBitLength(hashBits))
    return false;
  QFile file(fileName);
  if (file.open(QFile::ReadOnly))
  {
    qint64 fileSize = file.size();
    qint64 readBytes = 0;
    if (!startBatch())
      return false;
    bool success = true;
    char *buffer = new char[blockSize];
    // repeatedly read blockSize bytes of the file to buffer and pass it to putBatch:
    while (file.error() == QFile::NoError)
    {
      readBytes = file.read(buffer, qMin(fileSize-file.pos(), (qint64)blockSize)); // read till end of file to buffer, but not more than blockSize bytes
      if (readBytes > 0)
      {
        if (!putBatch(buffer, readBytes))
        {
          success = false;
          break;
        }
        if (readBytes < blockSize) // that was the last block
          break;
      } else // error occured
      {
        success = false;
        break;
      }
    }
    delete buffer;
    if (!stopBatch())
      success = false;
    return success;
  } else
    return false;
}

bool QKeccakHash::startBatch(HashBits hashBits)
{
  if (isBatchRunning())
    return false;
  if (!setHashBitLength(hashBits))
    return false;
  mState = new KeccakImpl::hashState;
  bool success = KeccakImpl::Init(mState, mHashBitLength) == KeccakImpl::SUCCESS;
  if (!success)
  {
    delete mState;
    mState = 0;
    mHashResult.clear();
  }
  return success;
}

bool QKeccakHash::putBatch(const QByteArray &ba)
{
  return putBatch(ba.constData(), ba.size());
}

bool QKeccakHash::putBatch(const char *data, int size)
{
  if (!isBatchRunning() || mHashBitLength == 0)
    return false;
  bool success = KeccakImpl::Update(mState, (KeccakImpl::BitSequence*)data, size*8) == KeccakImpl::SUCCESS;
  if (!success)
  {
    delete mState;
    mState = 0;
    mHashResult.clear();
  }
  return success;
}

bool QKeccakHash::stopBatch()
{
  if (!isBatchRunning() || mHashBitLength == 0)
    return false;
  bool success = KeccakImpl::Final(mState, (KeccakImpl::BitSequence*)mHashResult.data()) == KeccakImpl::SUCCESS;
  delete mState;
  mState = 0;
  if (!success)
    mHashResult.clear();
  return success;
}

bool QKeccakHash::isBatchRunning() const
{
  return mState;
}

bool QKeccakHash::setHashBitLength(HashBits hashBits)
{
  switch (hashBits)
  {
    case hb224: mHashBitLength = 224; break;
    case hb256: mHashBitLength = 256; break;
    case hb384: mHashBitLength = 384; break;
    case hb512: mHashBitLength = 512; break;
    default: 
    {
      mHashBitLength = 0;
      qDebug() << "QKeccakHash::setHashBitLength(): invalid hash bit value" << (int)hashBits << ", must be hb224, hb256, hb384 or hb512";
      return false;
    }
  }
  mHashResult.fill(0, mHashBitLength/8);
  return true;
}

