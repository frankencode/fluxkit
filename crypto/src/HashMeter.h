/*
 * Copyright (C) 2007-2015 Frank Mertens.
 *
 * Use of this source is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#ifndef FLUXCRYPTO_HASHMETER_H
#define FLUXCRYPTO_HASHMETER_H

#include <flux/Stream>
#include <flux/crypto/HashSum>

namespace flux {
namespace crypto {

/** \brief Hash sum computing stream
  * \see flux::stream::TransferMeter
  */
class HashMeter: public Stream
{
public:
    static Ref<HashMeter> open(HashSum *hashSum, Stream *stream = 0);

    virtual bool readyRead(double interval) const;
    virtual int read(ByteArray *data);

    virtual void write(const ByteArray *data);
    virtual void write(const StringList *parts);

    Ref<ByteArray> finish();

private:
    HashMeter(HashSum *hashSum, Stream *stream);

    Ref<HashSum> hashSum_;
    Ref<Stream> stream_;
};

}} // namespace flux::crypto

#endif // FLUXCRYPTO_HASHMETER_H
