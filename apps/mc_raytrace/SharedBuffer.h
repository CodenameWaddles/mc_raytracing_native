#pragma once

#include <windows.h>

class SharedBuffer {
	public:
		SharedBuffer(const char* name, const size_t size);
		void setup();
		int readInt(int index);
		void writeInt(int index, int value);
		float readFloat(int index);
		void writeFloat(int index, float value);
		void close();
	private:
		const char* fileName;
		size_t memorySize;
		void* sharedMemory;
		HANDLE mappingHandle;
		HANDLE fileHandle;
};