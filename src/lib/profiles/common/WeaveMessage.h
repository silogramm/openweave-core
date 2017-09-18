/*
 *
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *      This file defines macros and objects commonly used for the
 *      processing of Weave messages.
 *
 */

#ifndef _WEAVE_MESSAGE_H
#define _WEAVE_MESSAGE_H

#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Core/WeaveError.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Support/NLDLLUtil.h>

#include <SystemLayer/SystemPacketBuffer.h>

/*
 * these macros are the guts of the packing and parsing stuff and they're used
 * everywhere.
 *
 * here are the writers
 * parameters:
 * - PTR, a pointer into a buffer of type uint8_t
 * - VAL, a value to write
 */
#define WRITEBYTE(PTR, VAL)        \
    do {                           \
        *(PTR)++ = (uint8_t)(VAL); \
    } while (0)

#define WRITE16(PTR, VAL)               \
    do {                                \
        WRITEBYTE((PTR), ((VAL) >> 0)); \
        WRITEBYTE((PTR), ((VAL) >> 8)); \
    } while (0)

#define WRITE32(PTR, VAL)                \
    do {                                 \
        WRITEBYTE((PTR), ((VAL) >>  0)); \
        WRITEBYTE((PTR), ((VAL) >>  8)); \
        WRITEBYTE((PTR), ((VAL) >> 16)); \
        WRITEBYTE((PTR), ((VAL) >> 24)); \
    } while (0)

/*
 * and the readers
 * parameter: PTR, a pointer into a buffer of type uint8_t
 * value: the value read form the buffer
 */
#define READBYTE(PTR) (*(uint8_t *)(PTR)++)
/*
 * parameters:
 * - PTR, a pointer into a buffer of type uint8_t
 * - DEST, where to put what's read from *p
 */
#define READ16(PTR, DEST)                           \
    do {                                            \
        uint16_t __byte0 = (uint16_t)READBYTE(PTR); \
        uint16_t __byte1 = (uint16_t)READBYTE(PTR); \
        DEST = __byte0 + (__byte1 << 8);            \
    } while (0)

#define READ32(PTR, DEST)                                                    \
    do {                                                                     \
        uint16_t __byte0 = (uint16_t)READBYTE(PTR);                          \
        uint16_t __byte1 = (uint16_t)READBYTE(PTR);                          \
        uint16_t __byte2 = (uint16_t)READBYTE(PTR);                          \
        uint16_t __byte3 = (uint16_t)READBYTE(PTR);                          \
        DEST = __byte0 + (__byte1 << 8) + (__byte2 << 16) + (__byte3 << 24); \
    } while (0)

/*
 * the message iterator class allows packet handling to operate in
 * a regular manner, do a bit of work, check the result, get out if
 * it's not good and so on. this results in a bunch of boilerplate
 * code, which is captured here.
 */
#define TRY(OPERATION)                      \
    do {                                    \
        WEAVE_ERROR e = (OPERATION);        \
        if ( e != WEAVE_NO_ERROR) return e; \
    } while (0)

#define RESCUE(ERR,OPERATION,OUT)            \
    do {                                     \
        ERR = (OPERATION);                   \
        if (ERR != WEAVE_NO_ERROR) goto OUT; \
    } while (0)

namespace nl {

namespace Weave {

namespace Profiles {

  /**
   *  @class RetainedPacketBuffer
   *
   *  @brief
   *    This is a base class that serves as a convenience object for
   *    automatically reference counting a System::PacketBuffer.
   *
   */
  class NL_DLL_EXPORT RetainedPacketBuffer
  {
  public:
      // Con/destructors
      RetainedPacketBuffer(void);
      RetainedPacketBuffer(const RetainedPacketBuffer &aRetainedPacketBuffer);
      ~RetainedPacketBuffer(void);

      RetainedPacketBuffer &operator =(const RetainedPacketBuffer &aRetainedPacketBuffer);

      virtual bool IsRetaining(void) const;

      void Retain(System::PacketBuffer *aBuffer);
      virtual void Release(void);

      inline System::PacketBuffer *GetBuffer(void) { return (mBuffer); }

  protected:
      System::PacketBuffer *mBuffer; //<! A pointer to the retained packet
                                     //<! buffer.
  };

  /*
   * in order to use message buffers sensibly, we define this iterator,
   * which can be used to keep track of boundaries and so on.
   */
  class NL_DLL_EXPORT MessageIterator : public RetainedPacketBuffer
  {
  public:
    // constructor
    MessageIterator(System::PacketBuffer*);
    // reading and writing
    WEAVE_ERROR readByte(uint8_t*);
    WEAVE_ERROR read16(uint16_t*);
    WEAVE_ERROR read32(uint32_t*);
    WEAVE_ERROR read64(uint64_t*);
    WEAVE_ERROR readString(uint16_t, char*);
    WEAVE_ERROR readBytes(uint16_t, uint8_t*);
    WEAVE_ERROR writeByte(uint8_t);
    WEAVE_ERROR write16(uint16_t);
    WEAVE_ERROR write32(uint32_t);
    WEAVE_ERROR write64(uint64_t);
    WEAVE_ERROR writeString(uint16_t, char*);
    WEAVE_ERROR writeBytes(uint16_t, uint8_t*);
    // standard iterator operations
    MessageIterator& operator ++(void);
    MessageIterator& operator +(uint16_t);
    MessageIterator& operator -(uint16_t);
    bool operator ==(const MessageIterator&);
    bool operator !=(const MessageIterator&);
    uint8_t& operator *(void);
    void append(void);
    // size checking
    bool hasData(uint16_t);
    bool hasRoom(uint16_t);
    // finishing
    void finishWriting(void);
    // data members
    uint8_t *thePoint;
  };
  /*
   * here's how we handle strings in weave. this class has an 8-bit
   * and a 16-bit variant.
   */
  class NL_DLL_EXPORT ReferencedString : public RetainedPacketBuffer
  {
  public:
    // constructor
    ReferencedString(void);
    // initializers
    WEAVE_ERROR init(uint16_t, char*, System::PacketBuffer*);
    WEAVE_ERROR init(uint16_t, char*);
    WEAVE_ERROR init(uint8_t, char*, System::PacketBuffer*);
    WEAVE_ERROR init(uint8_t, char*);
    // pack and parse
    WEAVE_ERROR pack(MessageIterator&);
    static WEAVE_ERROR parse(MessageIterator&, ReferencedString&);
    // comparison
    bool operator ==(const ReferencedString&) const;
    // print string generation (for testing)
    char *printString(void);
    //data members
    uint16_t theLength;
    char *theString;
    bool isShort;
  };
  /*
   * similarly, we need to be able to represent a big old blob
   * of TLV data.
   */
  class NL_DLL_EXPORT ReferencedTLVData : public RetainedPacketBuffer
  {
  public:

    /*
     * under certain circumstances, e.g. when we don't want to blow out the
     * stack by writing some big thing into it in preparation for sending
     * a message with a referenced extent in it, we want to only write the
     * bits AFTER we've obtained the outgoing buffer. we do that using one
     * of these.
     *
     * NOTE!!! the handler set up below does NOT return an error and, in
     * the case where one is supplied, the pack method will not return an
     * error either. this means that the NHL-supplied handler MUST handler
     * all errors that arise from the formatting of TLV.
     *
     * parameters:
     * - TLVWriter&, a TLV writer to use to write things out
     * - void*, a state object known to the application
     */

    typedef void (*TLVWriteCallback)(TLV::TLVWriter &aWriter, void *aAppState);

    // constructor

    ReferencedTLVData(void);

    // initializers

    WEAVE_ERROR init(System::PacketBuffer*);
    WEAVE_ERROR init(MessageIterator&);
    WEAVE_ERROR init(uint16_t, uint16_t, uint8_t*);
    WEAVE_ERROR init(TLVWriteCallback, void*);

    /*
     * ReferencedTLVData needs to override the free() and isFree()
     * methods because "freedom", in this case, is more than nothing
     * left to lose.
     */

    void free(void);

    bool isFree(void);

    /**
     * Check if a ReferencedTLVData object has anything in it.
     *
     * There are two possible ways such an object could "have"
     * something. Either it could have 0 length OR it could have no
     * write callback.
     *
     * @return true if the data set has 0 length or there's no write
     * callback in hand, false otherwise.
     */

    inline bool isEmpty(void)
    {
        return (theLength == 0 && theWriteCallback == NULL);
    };

    // packing and parsing

    /**
     * Pack a ReferencedTLVData object directly into an inet buffer.
     *
     * @param [in] PacketBuffer *buff, the buffer to fill.
     *
     * @return a WEAVE_ERROR reflecting the success of the underlying
     * pack call.
     */

    inline WEAVE_ERROR pack(System::PacketBuffer *buff)
    {
        MessageIterator i(buff);

        return pack(i);
    };

    WEAVE_ERROR pack(MessageIterator&);

    /**
     * Return the data length assuming that the object has been packed
     * into a buffer.
     *
     * @return the integer length of the packed data.
     */

    inline uint16_t packedLength(void)
    {
        return theLength;
    };

    /**
     * Parse a ReferencedTLVData object out of an inet buffer
     * (assuming it just contains TLV).
     *
     * @param [in] PacketBuffer *buff, the buffer to read from.
     *
     * @param [out] ReferencedTLVData &aTarget, a ReferencedTLVData
     * object to "fill" with the result.
     *
     * @return a WEAVE_ERROR reflecting the success of the underlying
     * parse call.
     */

    static inline WEAVE_ERROR parse(System::PacketBuffer *buff, ReferencedTLVData &aTarget)
    {
        MessageIterator i(buff);

        return parse(i, aTarget);
    };

    static WEAVE_ERROR parse(MessageIterator&, ReferencedTLVData&);

    // comparison

    bool operator ==(const ReferencedTLVData&) const;

    // data members

    uint16_t         theLength;
    uint16_t         theMaxLength;
    uint8_t         *theData;

  private:
    TLVWriteCallback theWriteCallback;
    void            *theAppState;
  };

}; // Profiles

}; // Weave

}; // nl

#endif // _WEAVE_MESSAGE_H
