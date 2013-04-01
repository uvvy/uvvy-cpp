//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
//===========================================================================================================
//
// XDR eXternal Data Representation format as defined by http://tools.ietf.org/html/rfc4506
// I probably don't want to use this, since a byte-oriented format such as BDR might be
// more useful, and even then, some standard features as provided
// for example by boost.serialization might be even more useful.
// XDR is a well-defined standard though, might be wise to follow it.
//
#pragma once

#include <boost/endian/conversion2.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/range/iterator_range.hpp>
#include "byte_array.h"

namespace xdr {

class decode_error : public std::exception
{};

// Write padding bytes after writing an object of size 'size' up to multiple of 4 bytes.
template<class Archive>
inline void pad_size(Archive& oa, size_t size)
{
    static const char zeros[4] = {0};
    if (size & 3)
        oa.save_binary(zeros, 4 - (size & 3));
}

// Skip padding bytes after reading an object of size 'size'
template<class Archive>
inline void skip_padding(Archive& ia, size_t size)
{
    if (size & 3)
    {
        char datum;
        size = 4 - (size & 3);
        for (size_t i = 0; i < size; ++i)
            ia >> datum;
    }
}

// Encode XDR vector: fixed-size array.
template<class Archive>
inline void encode_vector(Archive& oa, const byte_array& ba, uint32_t maxlen)
{
    assert(ba.length() == maxlen);
    oa << ba;
    pad_size(oa, maxlen);
}

template<class Archive>
inline void decode_vector(Archive& ia, byte_array& ba, uint32_t maxlen)
{
    ba.resize(maxlen);
    ia.load_binary(ba.data(), ba.length());
    skip_padding(ia, maxlen);
}

// Encode XDR array: variable-size byte array. (hmm?)
template<class Archive>
inline void encode_array(Archive& oa, const byte_array& ba, uint32_t maxlen)
{
    assert(ba.length() <= maxlen);
    uint32_t size = ba.length();
    size = boost::endian2::big(size);
    oa << size;
    oa << ba;
    pad_size(oa, size);
}

template<class Archive>
inline void decode_array(Archive& ia, byte_array& ba, uint32_t maxlen)
{
    uint32_t size; // this is not quite portable and is a limitation of XDR (sizes are 32 bit)
    ia >> size;
    size = boost::endian2::big(size);
    if (size > maxlen)
        throw decode_error();
    ba.resize(size);
    ia >> ba;
    skip_padding(ia, size);
}

// Encode XDR list: variable-size array of arbitrary types.
template<class Archive, typename T>
inline void encode_list(Archive& oa, const std::vector<T>& ba, uint32_t maxlen)
{
    assert(ba.size() <= maxlen);
    uint32_t size = ba.size();
    size = boost::endian2::big(size);
    oa << size;
    for (uint32_t index = 0; index < ba.size(); ++index)
        oa << ba[index];
}

template<class Archive, typename T>
inline void decode_list(Archive& ia, std::vector<T>& ba, uint32_t maxlen)
{
    uint32_t size; // this is not quite portable and is a limitation of XDR (sizes are 32 bit)
    ia >> size;
    size = boost::endian2::big(size);
    if (size > maxlen)
        throw decode_error();
    ba.resize(size);
    for (uint32_t index = 0; index < size; ++index)
        ia >> ba[index];
}


// Store an optional chunk into a substream, then feed it and its size into the archive.
// It only works with XDR-based binary archives, otherwise this operation makes little sense.
template<class Archive, typename T>
inline void encode_option(Archive& oa, const T& t, uint32_t maxlen)
{
    byte_array arr;
    boost::iostreams::filtering_ostream ov(boost::iostreams::back_inserter(arr.as_vector()));
    boost::archive::binary_oarchive oa_buf(ov, boost::archive::no_header);
    oa_buf << t;
    ov.flush();

    uint32_t size = arr.size();
    size = boost::endian2::big(size);

    oa << size << arr;
}

template<class Archive, typename T>
inline void decode_option(Archive& ia, T& t, uint32_t maxlen)
{
    uint32_t size;
    ia >> size;
    size = boost::endian2::big(size);

    if (size > maxlen)
        return;

    byte_array arr;
    arr.resize(size);

    ia >> arr;

    {
        boost::iostreams::filtering_istream iv(boost::make_iterator_range(arr.as_vector()));
        boost::archive::binary_iarchive ia_buf(iv, boost::archive::no_header);
        ia_buf >> t;
    }
}

}

// boost::array compatible output for fixed-size vector represented as a boost::array<T,N>
// @todo: specialization for non POD T's
template<class Archive, class T, size_t N>
inline Archive& operator << (Archive& oa, const boost::array<T,N>& ba)
{
    oa << ba;
    xdr::pad_size(oa, N);
    return oa;
}



/**
int identifier;
unsigned int identifier;

        (MSB)                   (LSB)
      +-------+-------+-------+-------+
      |byte 0 |byte 1 |byte 2 |byte 3 |                      INTEGER, UNSIGNED INTEGER
      +-------+-------+-------+-------+
      <------------32 bits------------>

Enumerations have the same representation as signed integers.
enum { RED = 2, YELLOW = 3, BLUE = 5 } colors;

bool identifier;
This is equivalent to:
enum { FALSE = 0, TRUE = 1 } identifier;

hyper identifier; 
unsigned hyper identifier;

        (MSB)                                                   (LSB)
      +-------+-------+-------+-------+-------+-------+-------+-------+
      |byte 0 |byte 1 |byte 2 |byte 3 |byte 4 |byte 5 |byte 6 |byte 7 |
      +-------+-------+-------+-------+-------+-------+-------+-------+
      <----------------------------64 bits---------------------------->
                                                 HYPER INTEGER
                                                 UNSIGNED HYPER INTEGER

float identifier;

         +-------+-------+-------+-------+
         |byte 0 |byte 1 |byte 2 |byte 3 |              SINGLE-PRECISION
         S|   E   |           F          |         FLOATING-POINT NUMBER
         +-------+-------+-------+-------+
         1|<- 8 ->|<-------23 bits------>|
         <------------32 bits------------>

double identifier;

         +------+------+------+------+------+------+------+------+
         |byte 0|byte 1|byte 2|byte 3|byte 4|byte 5|byte 6|byte 7|
         S|    E   |                    F                        |
         +------+------+------+------+------+------+------+------+
         1|<--11-->|<-----------------52 bits------------------->|
         <-----------------------64 bits------------------------->
                                        DOUBLE-PRECISION FLOATING-POINT


quadruple identifier;

         +------+------+------+------+------+------+-...--+------+
         |byte 0|byte 1|byte 2|byte 3|byte 4|byte 5| ...  |byte15|
         S|    E       |                  F                      |
         +------+------+------+------+------+------+-...--+------+
         1|<----15---->|<-------------112 bits------------------>|
         <-----------------------128 bits------------------------>
                                      QUADRUPLE-PRECISION FLOATING-POINT


opaque identifier[n];

          0        1     ...
      +--------+--------+...+--------+--------+...+--------+
      | byte 0 | byte 1 |...|byte n-1|    0   |...|    0   |
      +--------+--------+...+--------+--------+...+--------+
      |<-----------n bytes---------->|<------r bytes------>|
      |<-----------n+r (where (n+r) mod 4 = 0)------------>|
                                                   FIXED-LENGTH OPAQUE


opaque identifier<m>;
opaque identifier<>;

            0     1     2     3     4     5   ...
         +-----+-----+-----+-----+-----+-----+...+-----+-----+...+-----+
         |        length n       |byte0|byte1|...| n-1 |  0  |...|  0  |
         +-----+-----+-----+-----+-----+-----+...+-----+-----+...+-----+
         |<-------4 bytes------->|<------n bytes------>|<---r bytes--->|
                                 |<----n+r (where (n+r) mod 4 = 0)---->|
                                                  VARIABLE-LENGTH OPAQUE


string object<m>;
string object<>;

            0     1     2     3     4     5   ...
         +-----+-----+-----+-----+-----+-----+...+-----+-----+...+-----+
         |        length n       |byte0|byte1|...| n-1 |  0  |...|  0  |
         +-----+-----+-----+-----+-----+-----+...+-----+-----+...+-----+
         |<-------4 bytes------->|<------n bytes------>|<---r bytes--->|
                                 |<----n+r (where (n+r) mod 4 = 0)---->|
                                                                  STRING

type-name identifier[n];

   Fixed-length arrays of elements numbered 0 through n-1 are encoded by
   individually encoding the elements of the array in their natural
   order, 0 through n-1.  Each element's size is a multiple of four
   bytes.  Though all elements are of the same type, the elements may
   have different sizes.  For example, in a fixed-length array of
   strings, all elements are of type "string", yet each element will
   vary in its length.

         +---+---+---+---+---+---+---+---+...+---+---+---+---+
         |   element 0   |   element 1   |...|  element n-1  |
         +---+---+---+---+---+---+---+---+...+---+---+---+---+
         |<--------------------n elements------------------->|

                                               FIXED-LENGTH ARRAY

type-name identifier<m>;
type-name identifier<>;

           0  1  2  3
         +--+--+--+--+--+--+--+--+--+--+--+--+...+--+--+--+--+
         |     n     | element 0 | element 1 |...|element n-1|
         +--+--+--+--+--+--+--+--+--+--+--+--+...+--+--+--+--+
         |<-4 bytes->|<--------------n elements------------->|
                                                         COUNTED ARRAY

struct {
    component-declaration-A;
    component-declaration-B;
    ...
} identifier;

         +-------------+-------------+...
         | component A | component B |...                      STRUCTURE
         +-------------+-------------+...

union switch (discriminant-declaration) {
 case discriminant-value-A:
    arm-declaration-A;
 case discriminant-value-B:
    arm-declaration-B;
 ...
 default: default-declaration;
} identifier;

           0   1   2   3
         +---+---+---+---+---+---+---+---+
         |  discriminant |  implied arm  |          DISCRIMINATED UNION
         +---+---+---+---+---+---+---+---+
         |<---4 bytes--->|

void;

           ++
           ||                                                     VOID
           ++
         --><-- 0 bytes

Optional:

type-name *identifier;

This is equivalent to the following union:

 union switch (bool opted) {
 case TRUE:
    type-name element;
 case FALSE:
    void;
 } identifier;

length-delimited optional:

type-name ?identifier;

uses size field at the start of an optional, to allow skipping unknown types.

*/