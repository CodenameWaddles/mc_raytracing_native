#pragma once

#include <windows.h>
#include <iostream>
#include <vector>
#include "DataStructures.h"

class DataManager {
	public:
		void setup();
		void read(int x, int z);
		void write();
		void close();
        void readChunk(uint8_t* chunkData, int size);
        void readData(uint8_t* data, int size);
	private:
        std::vector<ChunkPos> chunkPositions;
};

// Stream stuff for import \\

class ByteArrayStreamBuf : public std::streambuf {
public:
    ByteArrayStreamBuf(const char* data, std::size_t size) {
        // Initialize the buffer pointers
        setg(const_cast<char*>(data), const_cast<char*>(data), const_cast<char*>(data) + size);
    }
};

class ByteArrayIStream : public std::istream {
public:
    ByteArrayIStream(const char* data, std::size_t size)
        : std::istream(&buffer), buffer(data, size) {}

private:
    ByteArrayStreamBuf buffer;
};

