/*
 * buffer.cxx
 *
 * Handle file read buffering.
 *
 * libtpt 1.30
 *
 */

/*
 * Copyright (C) 2002-2004 Isaac W. Foraker (isaac@tazthecat.net)
 * All Rights Reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the Author nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "libtpt_conf.h"
#include "libtpt/buffer.h"
#include "libtpt/tptexcept.h"
#include <algorithm>
#include <vector>
#include <iostream>
#include <fstream>
#include <cstring>

// Anonymous namespace for local constants
namespace {
    const unsigned BUFFER_SIZE = 4096;
}

namespace TPT {

/**
 * Construct a read Buffer for the specified file.
 *
 * @param   filename        name of file to buffer
 * @return  nothing
 */
Buffer::Buffer(const char* filename) :
    instr_(0),
    name_(filename),
    buffersize_(0),
    bufferallocsize_(BUFFER_SIZE),
    buffer_(new char[BUFFER_SIZE]),
    bufferidx_(0),
    freestreamwhendone_(true),
    done_(false)
{
    // Just in case there is an exception while allocating the stream.
    try {
        openfile(filename);
    } catch(...) {
        delete [] buffer_;
        throw;
    }
    readfile();
}


/**
 * Construct a read Buffer for an open fstream.  The input stream will
 * not be closed when Buffer is destructed.
 *
 * @param   is          input fstream
 * @return  nothing
 */
Buffer::Buffer(std::istream* is) :
    instr_(is),
    name_("<stream>"),
    buffersize_(0),
    bufferallocsize_(BUFFER_SIZE),
    buffer_(new char[BUFFER_SIZE]),
    bufferidx_(0),
    freestreamwhendone_(false),
    done_(false)
{
    readfile();
}


/**
 * Construct a read Buffer for an existing buffer.
 *
 * @return  nothing
 */
Buffer::Buffer(const char* buf, unsigned long bufsize) :
    instr_(0),
    name_("<memory>"),
    buffersize_(bufsize),
    bufferallocsize_(bufsize),
    buffer_(new char[bufsize]),
    bufferidx_(0),
    freestreamwhendone_(false),
    done_(!bufsize) // if zero buffer, then done
{
    std::memcpy(buffer_, buf, bufsize);
}


/**
 * Construct a read Buffer on a subspace of an existing Buffer.
 *
 * @return  nothing
 */
Buffer::Buffer(const Buffer& buf, unsigned long start, unsigned long end) :
    instr_(0),
    name_("<buffer>"),
    buffersize_(end-start),
    bufferallocsize_(end-start),
    buffer_(new char[end-start]),
    bufferidx_(0),
    freestreamwhendone_(false),
    done_(!(end-start))
{
    std::memcpy(buffer_, &buf.buffer_[start], end-start);
}


/**
 * Free buffer and fstream if necessary
 */
Buffer::~Buffer()
{
    delete [] buffer_;
    if (freestreamwhendone_)
        delete instr_;
}


/**
 * Get the next available character from the buffer.  Use operator
 * bool() to determine if more data is available.  Reading past the end
 * of the buffer results in undefined behavior.
 *
 * @return  next available character;
 * @return  undefined if no more characters available
 */
char Buffer::getnextchar()
{
    register char c = buffer_[bufferidx_];
    ++bufferidx_;

    // If our buffer is empty, try reading from the stream
    // if there is one.
    if (bufferidx_ >= buffersize_)
        readfile();

    return c;
}


/**
 * Unget a single character back into the buffer.
 *
 * @return  false on success;
 * @return  true if unget buffer full
 */
bool Buffer::unget()
{
    if (!bufferidx_)
        return true;

    --bufferidx_;
    done_ = false;
    return false;
}


/**
 * Reset the buffer index to the beginning of the input buffer.
 *
 * @return  nothing
 */
void Buffer::reset()
{
    bufferidx_ = 0;
    done_ = false;
}


/**
 * Seek to a index point in the buffer.  The seek will force reading of
 * the file or stream if the index has not yet been buffered.  If the
 * index is past the end of the buffer, the attempt will fail and the
 * internal index will not be reset.
 *
 * @param   index           Offset into buffer from beginning.
 * @return  false on success;
 * @return  true if past end of buffer
 */
bool Buffer::seek(unsigned long index)
{
    // This loop allows seeking to part of buffer that has not yet been read.
    while (!done_ && (index >= buffersize_))
        readfile();
    if (index >= buffersize_)
        return true;
    bufferidx_ = index;
    done_ = false;

    return false;
}


/**
 * Get the offset of the next character in the buffer to be read.  Use
 * operator bool() to determine if there is data in the buffer.
 * 
 * @return  current offset into buffer of next character to be read.
 */
unsigned long Buffer::offset() const
{
    return bufferidx_;
}


/**
 * Read from a specific index in the buffer, without losing the current
 * position information, reading the input file or stream if necessary.
 *
 * @param   index           Offset into buffer from beginning.
 * @return  character at index;
 * @return  undefined if invalid index
 */
const char Buffer::operator[](unsigned long index)
{
    while (!done_ && (index >= buffersize_))
        readfile();
    if (index >= buffersize_)
        throw tptexception("Index out of bounds");
    return buffer_[index];
}


/**
 * Perform a boolean check for availability of data in the buffer.
 * Usage can look like:
 *
 * TPT::Buffer readbuf("input.txt");\n
 * if (readbuf) {\n
 *      .\n
 *      .\n
 *      .\n
 * }
 *
 * @return  false if no data is available;
 * @return  true if data is available
 */
Buffer::operator bool() const
{
    return !done_;
}


/**
 * Get the size of the current buffer.  If this is a file or
 * stream buffer, the size may be smaller than the current file
 * or stream if the buffer has not been fully filled.
 *
 * @return  current size of buffer
 */
unsigned long Buffer::size() const
{
    return buffersize_;
}


/**
 * Set the buffer name.  By default, file buffers will have a name of the
 * filename if the buffer is a file buffer, or 'stream', 'memory', or 'buffer'
 * depending on the constructor.
 *
 * @param   name    new buffer name
 * @return  nothing
 */
void Buffer::setname(const char* name)
{
    name_ = name;
}

/**
 * Get the current buffer name.
 *
 * @return  current buffer name.
 */
const char* Buffer::getname()
{
    return name_.c_str();
}

/*
 * Open a file stream for the specified file.
 *
 * @param   filename
 * @return  false on success
 * @return  true on failure
 */
void Buffer::openfile(const char* filename)
{
    std::fstream* is = new std::fstream(filename, std::ios::in | std::ios::binary);

    if (!is->is_open())
        done_ = true;
    instr_ = is;
}


/*
 * Read the next block of bytes from the file.  Set the done flag if
 * there's no more data to read.
 *
 * @return  false on success;
 * @return  true if no more data
 */
bool Buffer::readfile()
{
    if (!done_ && instr_ && !instr_->eof())
    {
        char buf[BUFFER_SIZE];
        size_t size = 0;
        if (instr_->read(buf, BUFFER_SIZE) || instr_->gcount())
            size = instr_->gcount();
        if (!size)
            done_ = true;
        else {
            if ((buffersize_ + size) >= bufferallocsize_)
                enlarge();
            std::memcpy(&buffer_[buffersize_], buf, size);
            buffersize_+= size;
        }
    }
    else
        done_ = true;

    return done_;
}


/*
 * Increase the size of the buffer by BUFFER_SIZE
 */
void Buffer::enlarge()
{
    bufferallocsize_+= BUFFER_SIZE;
    // Create a new buffer and swap it for the old buffer
    char* temp = new char[bufferallocsize_];
    std::memcpy(temp, buffer_, buffersize_);
    delete [] buffer_;
    buffer_ = temp;
}


/**
 * \example bufferfile.cxx
 *
 * This is an example of how to use TPT::Buffer with a file.
 */

/**
 * \example bufferfstream.cxx
 *
 * This is an example of how to use TPT::Buffer with an input
 * fstream.
 */

/**
 * \example bufferbuffer.cxx
 *
 * This is an example of how to use TPT::Buffer with a buffer.
 */


} // end namespace TPT
