#include "dataManager.h"
#include "enkimi.h"

// Chunk tags

//DataVersion
//Heightmaps
//InhabitedTime
//LastUpdate
//PostProcessing
//Status
//block_entities
//block_ticks
//fabric : attachments
//fluid_ticks
//sections
//structures
//xPos
//yPos
//zPos

void DataManager::setup() {
    
}

void DataManager::read(int x, int z) {
    
}

void DataManager::write() {
    
}

void DataManager::close() {
    
}

// Read chunk data from byte array and add to data
void DataManager::readChunk(uint8_t* chunkData, int size) {
    //auto* dataStream = new ByteArrayIStream(reinterpret_cast<const char*>(chunkData), size);

    enkiNBTDataStream stream;
    //enkiInitNBTDataStreamForChunk(regionFile, i, &stream);
    enkiNBTInitFromMemoryCompressed(&stream, chunkData, size, 0);
    if (stream.dataLength)
    {
        enkiChunkBlockData aChunk = enkiNBTReadChunk(&stream);
        enkiMICoordinate chunkOriginPos = enkiGetChunkOrigin(&aChunk);
        
        //printf("Chunk at xyz{ %d, %d, %d }  Number of sections: %d \n",
            //chunkOriginPos.x/16, chunkOriginPos.y, chunkOriginPos.z/16, aChunk.countOfSections);
    }
    enkiNBTFreeAllocations(&stream);

    //nbt::io::stream_reader reader(*stream);

    //std::pair<std::string, std::unique_ptr<nbt::tag_compound>> data = reader.read_compound();
    //nbt::tag_compound compound = *(data.second.get());
    //for (std::pair<const std::string, nbt::value> pair : compound) {
    //    //std::cout << pair.first << std::endl;
    //}
}

void DataManager::readData(uint8_t* data, int size) {
    enkiNBTDataStream stream;
    enkiNBTInitFromMemoryCompressed(&stream, data, size, 0);
}