#include <windows.h>
#include <iostream>
#include "SharedBuffer.h"

SharedBuffer::SharedBuffer(const char* name, const size_t size) {
    fileName = name;
    memorySize = size;
}

void SharedBuffer::setup() {

    // Open the shared memory file created by Java
    fileHandle = CreateFileA(
        fileName,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (fileHandle == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open shared memory file. Error: " << fileName << GetLastError() << std::endl;
    }

    // Create a file mapping object
    mappingHandle = CreateFileMappingA(
        fileHandle,            // File handle
        NULL,                  // Default security
        PAGE_READWRITE,        // Read/Write access
        0,                     // Maximum object size (high-order DWORD)
        memorySize,            // Maximum object size (low-order DWORD)
        NULL);                 // Name of mapping object

    if (mappingHandle == NULL) {
        std::cerr << "Failed to create file mapping. Error: " << fileName << GetLastError() << std::endl;
        CloseHandle(fileHandle);
    }

    // Map the shared memory into the process's address space
    sharedMemory = MapViewOfFile(
        mappingHandle,         // Handle to mapping object
        FILE_MAP_READ | FILE_MAP_WRITE, // Read/Write access
        0,                     // File offset (high-order DWORD)
        0,                     // File offset (low-order DWORD)
        memorySize);           // Number of bytes to map

    if (sharedMemory == NULL) {
        std::cerr << "Failed to map shared memory. Error: " << fileName << GetLastError() << std::endl;
        CloseHandle(mappingHandle);
        CloseHandle(fileHandle);
    }

    std::cout << "C++ : Setup shared memory : " << fileName << std::endl;
    writeInt(0, 80);
}

int SharedBuffer::readInt(int index) {
    // Read values
    int* intData = static_cast<int*>(sharedMemory);
    return intData[index];
    //std::cout << "C++ : Integer value: " << intData[index] << std::endl;
}

void SharedBuffer::writeInt(int index, int value) {
    // Write values
    int* intData = static_cast<int*>(sharedMemory);
    intData[index] = value;
}

float SharedBuffer::readFloat(int index) {
    // Read values
    float* floatData = static_cast<float*>(sharedMemory);
    return floatData[index];
}

void SharedBuffer::writeFloat(int index, float value) {
    // Write values
    float* floatData = static_cast<float*>(sharedMemory);
    floatData[index] = value;
}

void SharedBuffer::close() {
    // Cleanup
    UnmapViewOfFile(sharedMemory);
    CloseHandle(mappingHandle);
    CloseHandle(fileHandle);
}