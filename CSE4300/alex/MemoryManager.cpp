#include <iostream>
#include <stdexcept> // will fix error handling laterrrr
#include "MemoryManager.h"

// Let's try to simulate virtual memory!

// fairly basic memory manager

// constructors

MemoryManager:: MemoryManager()
: PAGE_SIZE(4096), PAGE_COUNT(1024), PHYSICAL_SIZE(4096*1024){
    _initializeMemory();
}

MemoryManager:: MemoryManager(int page_size, int num_pages, int num_frames)
: PAGE_SIZE(page_size), PAGE_COUNT(num_pages), PHYSICAL_SIZE(num_frames * page_size){
    _initializeMemory();
}

// private methods

void MemoryManager:: _initializeMemory() {
    pageTable.resize(PAGE_COUNT);
    physicalMemory.resize(PHYSICAL_SIZE);
    freeFrames.resize(PHYSICAL_SIZE / PAGE_SIZE, true);

    diskStorage.resize(PAGE_COUNT * PAGE_SIZE); // give disk space for every entry
}

void MemoryManager:: _writeMemory(int physicalAddress, uint8_t data) {
    if (physicalAddress >= PHYSICAL_SIZE || physicalAddress < 0)
        throw std::out_of_range("Physical address out of bounds");

    physicalMemory[physicalAddress] = data;
}

uint8_t MemoryManager:: _readMemory(int physicalAddress) {
    if (physicalAddress >= PHYSICAL_SIZE || physicalAddress < 0)
        throw std::out_of_range("Physical address out of bounds");

    return physicalMemory[physicalAddress];
}

void MemoryManager:: _allocatePage(int virtualPageNumber, int frameNumber) {
    if (virtualPageNumber >= PAGE_COUNT || virtualPageNumber < 0)
        throw std::out_of_range("Invalid virtual page number");
    if (frameNumber >= (PHYSICAL_SIZE / PAGE_SIZE) || frameNumber < 0)
        throw std::out_of_range("Invalid frame number");

    pageTableEntry& entry = pageTable[virtualPageNumber];


    entry.validBit = true;
    entry.pageFrameNum = frameNumber;
    entry.presentBit = true;

    freeFrames[frameNumber] = false;
}

int MemoryManager:: _virtualToPhysicalAddress(int virtualAddress, bool writeOperation) {
    // first let's calculate the offset and find the virtual page number
    int offset = virtualAddress & (PAGE_SIZE - 1); // offset is based on the size of each page
    int virtualPageNumber = virtualAddress / PAGE_SIZE; // essentially shifts the number right by 12 bits

    // now lets validate the virtual address (bounds and valid bit)
    if (virtualPageNumber >= PAGE_COUNT || virtualPageNumber < 0)
        throw std::out_of_range("Attempted to access out-of-bound virtual address");

    pageTableEntry& entry = pageTable[virtualPageNumber];

    if (!entry.validBit)
        throw std::runtime_error("Segmentation fault occurred: Invalid page accessed");

    if (!entry.presentBit) {
        _handlePageFault(virtualPageNumber);
    }

    // now we can map the virtual to the physical
    int pageFrameNum = entry.pageFrameNum;
    int physicalAddress = (pageFrameNum * PAGE_SIZE) + offset;

    if (physicalAddress >= PHYSICAL_SIZE || physicalAddress < 0)
        throw std::out_of_range("Physical address out of bounds");

    entry.referenceBit = true;
    if (writeOperation) entry.modifyBit = true;

    return physicalAddress;
}

void MemoryManager:: _handlePageFault(int virtualPageNumber){
    std::cout << "Page fault at VPN: " <<virtualPageNumber << std::endl;

    pageTableEntry& entry = pageTable[virtualPageNumber];

    // check for free frame, if none, page replacement
    int freeFrame = -1;
    for (int i = 0; i < freeFrames.size(); i++) {
        if (freeFrames[i]) {
            freeFrame = i;
            break;
        }
    }

    if (freeFrame == -1) {
        entry.pageFrameNum = _replacePage();
    } else
        entry.pageFrameNum = freeFrame;

    _readPageFromDisk(virtualPageNumber, entry.pageFrameNum);

    freeFrames[entry.pageFrameNum] = false;

    entry.presentBit = true;
}

int MemoryManager:: _replacePage() {
    std::cout << "No free frame found, replacing page" << std::endl;

    int frameNumber = -1;
    int replacedVPN = -1;
    bool entryWasModified = false;

    bool ticking = true;
    while (ticking) {
        if (clockPointer == PAGE_COUNT) clockPointer = 0;

        pageTableEntry& entry = pageTable[clockPointer];

        if (entry.presentBit) {
            if (entry.referenceBit) {
                entry.referenceBit = false;
            } else {
                entry.presentBit = false;
                frameNumber = entry.pageFrameNum;
                replacedVPN = clockPointer;
                entryWasModified = entry.modifyBit;
                ticking = false;
            }
        }

        clockPointer++;
    }

    if (entryWasModified) _writePageToDisk(replacedVPN);

    freeFrames[frameNumber] = true;
    pageTable[replacedVPN].pageFrameNum = -1;
    _wipeMemoryFrame(frameNumber);

    return frameNumber;
}

void MemoryManager:: _wipeMemoryFrame(int frameNumber){
    if (frameNumber >= (PHYSICAL_SIZE / PAGE_SIZE) || frameNumber < 0)
        throw std::out_of_range("Invalid frame number");

    int baseAddress = frameNumber * PAGE_SIZE;

    for (int i = 0; i < PAGE_SIZE; i++) {
        physicalMemory[baseAddress + i] = 0;
    }
}

void MemoryManager:: _writePageToDisk(int virtualPageNumber){
    int frameNum = pageTable[virtualPageNumber].pageFrameNum;

    for (int i = 0; i < PAGE_SIZE; i++) {
        diskStorage[virtualPageNumber * PAGE_SIZE + i] = physicalMemory[frameNum * PAGE_SIZE + i];
    }
}

void MemoryManager:: _readPageFromDisk(int virtualPageNumber, int frameNumber){
    for (int i = 0; i < PAGE_SIZE; i++) {
        physicalMemory[frameNumber * PAGE_SIZE + i] = diskStorage[virtualPageNumber * PAGE_SIZE + i];
    }
}

void MemoryManager:: _deletePageFromDisk(int virtualPageNumber) {
    for (int i = 0; i < PAGE_SIZE; i++) {
        diskStorage[virtualPageNumber * PAGE_SIZE + i] = 0;
    }
}

// public methods

int MemoryManager:: allocateAnyPage() {
    // find open page entry
    int vpn = -1;
    for (int i = 0; i < PAGE_COUNT; i++) {
        if (pageTable[i].validBit != true) {
            vpn = i;
            break;
        }
    }
    if (vpn == -1) throw std::runtime_error("No free pages available for allocation");;

    // find open frame using our map of them :P
    int freeFrame = -1;
    for (int i = 0; i < freeFrames.size(); i++) {
        if (freeFrames[i]) {
            freeFrame = i;
            break;
        }
    }

    if (freeFrame == -1) freeFrame = _replacePage(); // page replacement here!

    _allocatePage(vpn, freeFrame);

    int virtualAddress = vpn * PAGE_SIZE;
    return virtualAddress;
}

void MemoryManager:: writeVirtualMemory(int virtualAddress, uint8_t data) {
    int physicalAddress = _virtualToPhysicalAddress(virtualAddress, true);
    _writeMemory(physicalAddress, data);
}

uint8_t MemoryManager:: readVirtualMemory(int virtualAddress) {
    int physicalAddress = _virtualToPhysicalAddress(virtualAddress, false);
    return _readMemory(physicalAddress);
}

void MemoryManager:: deletePageTableEntry(int virtualAddress) {
    int virtualPageNumber = virtualAddress / PAGE_SIZE;
    if (virtualPageNumber >= PAGE_COUNT || virtualPageNumber < 0)
        throw std::out_of_range("Attempted to access out-of-bound virtual address");

    pageTableEntry& entry = pageTable[virtualPageNumber];

    if (!entry.validBit)
        throw std::logic_error("Attempted to delete an invalid page");

    entry.validBit = false;

    if (entry.presentBit) {
        _wipeMemoryFrame(entry.pageFrameNum);
        entry.presentBit = false;
        freeFrames[entry.pageFrameNum] = true;
        entry.pageFrameNum = -1;

    } else {
        _deletePageFromDisk(virtualPageNumber);
    }

    entry.modifyBit = false;
    entry.referenceBit = false;
}

void MemoryManager:: printPageTableEntry(int virtualAddress) {
    int virtualPageNumber = virtualAddress / PAGE_SIZE;
    if (virtualPageNumber >= PAGE_COUNT || virtualPageNumber < 0)
        throw std::out_of_range("Attempted to access out-of-bound virtual address");

    const pageTableEntry& entry = pageTable[virtualPageNumber];
    std::cout << "Page " << virtualPageNumber << ": ";
    std::cout << "Valid = " << entry.validBit;
    std::cout << ", Present = " << entry.presentBit;
    std::cout << ", Frame = " << entry.pageFrameNum;
    std::cout << ", Referenced = " << entry.referenceBit;
    std::cout << ", Modified = " << entry.modifyBit;
    std::cout << std::endl;
}
