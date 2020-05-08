#ifndef HE_BINARY_H
#define HE_BINARY_H

#include "heTypes.h"
#include <string>
#include <fstream>
#include <vector>

struct HeBinaryBuffer {
	char*			ptr	      = nullptr;
	uint32_t		maxSize   = 0; // the size of ptr
	uint32_t		offset	  = 0; // the current offset from which to read
	uint32_t		size	  = 0; // the amount of valid data in ptr
	HeAccessType	access	  = HE_ACCESS_NONE;
	
	std::ofstream	out;
	std::ifstream	in;

	std::string fullPath; // the full relative path of this file
	std::string name; // the name of this file, without parent folders and file extension
};


// tries to open a file for binary operations. If access contains the read flag, an input stream will be opened and maxSize bytes
// will be read. If access contrains the write flag, an output stream will be opened and the pointer of the buffer will be allocated
// to maxSize bytes
extern HE_API b8 heBinaryBufferOpenFile(HeBinaryBuffer* buffer, std::string const& fileName, uint32_t const maxSize, HeAccessType const access);
// closes all streams of the buffer and deletes the pointer
extern HE_API void heBinaryBufferCloseFile(HeBinaryBuffer* buffer);
// gets the next char in the pointer of the buffer. If the pointer is completely read, this tries to peek into the file
extern HE_API char heBinaryBufferPeek(HeBinaryBuffer* buffer);
// checks whether there is still unread data in the pointer, or (if not so) if there is still unread data in the file.
extern HE_API b8 heBinaryBufferAvailable(HeBinaryBuffer* buffer);
// tries reading from the buffers input file. This will try to read maxSize bytes, update the pointer of the buffer and set the
// buffers size to the amount of bytes read. This will also reset the buffers offset to zero
extern HE_API inline void heBinaryBufferReadFromFile(HeBinaryBuffer* buffer);


// -- write to binary

// adds given input of given size to the binary buffer. This will also write the buffer if it has reached its pointers limit
extern HE_API void heBinaryBufferAdd(HeBinaryBuffer* buffer, void const* input, uint32_t size);
// adds given char to the binary buffer. This will also write the buffer if it has reached its pointers limit
extern HE_API void heBinaryBufferAdd(HeBinaryBuffer* buffer, char input);
// adds	 a string to a binary buffer
extern HE_API void heBinaryBufferAddString(HeBinaryBuffer* buffer, std::string const& in);
// adds	 a float to a binary buffer (~4 bytes (sizeof(float))
extern HE_API void heBinaryBufferAddFloat(HeBinaryBuffer* buffer, float const in);
// adds	 an int to a binary buffer (4 bytes)
extern HE_API void heBinaryBufferAddInt(HeBinaryBuffer* buffer, int32_t const in);
// adds a float buffer to the given binary buffer. This will first write the size of the buffer (in bytes) and then the contents
extern HE_API void heBinaryBufferAddFloatBuffer(HeBinaryBuffer* buffer, std::vector<float> const& floats);


// -- read from binary

// copies size bytes from buffer into output
extern HE_API b8 heBinaryBufferCopy(HeBinaryBuffer* buffer, void* output, uint32_t size);
// tries to read a string from the given buffer. The first four bytes must be the size of the string, followed by that amount of
// valid ascii characters. Stores the result in output. Output is cleared before filling in the new data
extern HE_API b8 heBinaryBufferGetString(HeBinaryBuffer* buffer, std::string* output);
// tries to read a four byte float from given buffer and sets output to the read result
extern HE_API b8 heBinaryBufferGetFloat(HeBinaryBuffer* buffer, float* output);
// tries to read a four byte int from given buffer and sets output to the read result
extern HE_API b8 heBinaryBufferGetInt(HeBinaryBuffer* buffer, int32_t* output);
// tries to read a float buffer from given buffer. The first four bytes must be the size of the following buffer (in bytes).
// This will then try to read that many bytes from the buffer and add them to output. output will be cleared before filling in the
// new data.
extern HE_API b8 heBinaryBufferGetFloatBuffer(HeBinaryBuffer* buffer, std::vector<float>* output);


// -- converters

// converts a d3 instance file from ascii to binary
extern HE_API void heBinaryConvertD3InstanceFile(std::string const& inFile, std::string const& outFile);
// converts any texture to binary
extern HE_API void heBinaryConvertTexture(std::string const& inFile, std::string const& outFile);


// -- loaders

// loads the pixel data from a binary texture file. That texture should be created before using heBinaryConvertTexture
extern HE_API unsigned char* heBinaryLoadTexture(std::string const& file, int32_t* width, int32_t* height, int32_t* channels);

#endif //HE_BINARY_H
