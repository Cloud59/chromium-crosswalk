// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NET_BIT_STREAM_READER_H_
#define CHROME_BROWSER_NET_BIT_STREAM_READER_H_

#include <stdint.h>

#include "base/strings/string_piece.h"

namespace internal {

// A class for reading individual bits from a packed buffer. Bits are read
// MSB-first from the stream.
// It is limited to 64-bit reads, 4GB streams and is inefficient as a design
// choice. This class should not be used frequently.
//
// It is meant for data that is is packed across bytes, necessitating the need
// to read a variable number of bits across a byte boundary.
class BitStreamReader {
 public:
  explicit BitStreamReader(const base::StringPiece& source);

  // Reads unary-encoded number into |out|. Returns true if
  // there was at least one bit to read, false otherwise.
  bool ReadUnaryEncoding(uint64_t* out);

  // Reads |num_bits| (up to 64) into |out|. |out| is filled from the MSB to the
  // LSB. If |num_bits| is less than 64, the most significant |64 - num_bits|
  // bits are unused and left as zeros. Returns true if the stream had the
  // requested |num_bits|, false otherwise.
  bool ReadBits(uint8_t num_bits, uint64_t* out);

  // Returns the number of bits left in the stream.
  uint64_t BitsLeft() const;

 private:
  // Reads a single bit. Within a byte, the bits are read from the MSB to the
  // LSB.
  uint8_t ReadBit();

  const base::StringPiece source_;

  // Index of the byte currently being read from.
  size_t current_byte_;

  // Index of the last bit read within |current_byte_|. Since bits are read
  // from the MSB to the LSB, this value is initialized to 7 and decremented
  // after each read.
  int8 current_bit_;

  DISALLOW_COPY_AND_ASSIGN(BitStreamReader);
};

}  // namespace internal

#endif  // CHROME_BROWSER_NET_BIT_STREAM_READER_H_
