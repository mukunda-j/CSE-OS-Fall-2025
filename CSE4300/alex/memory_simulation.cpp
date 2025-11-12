#include <iostream>
#include <vector>
#include <cstdint>

// Let's try to simulate virtual memory!

// going to use structs instead of bitwise operations
// but we can still account for the offset of bits and such


struct pageTableEntry {
    bool modifyBit = false;
    bool referenceBit = false;
    bool validBit = false;
    bool readBit = false; // will worry about permissions later
    bool writeBit = false; // ^^
    bool executeBit = false; // ^^
    bool presentBit = false; // will implement page replacement later
    int pageFrameNum = -1;
};

class MemoryManager {
    private:
        std::vector<pageTableEntry> pageTable;
        std::vector<uint8_t> physicalMemory;
        std::vector<bool> freeFrames;

        int PAGE_SIZE; // 4096 bytes, 4K per page
        int PAGE_COUNT; // 1024 entries in the page table
        int PHYSICAL_SIZE; // # of bytes of physical memory

        void _initializeMemory() {
            pageTable.resize(PAGE_COUNT);
            physicalMemory.resize(PHYSICAL_SIZE);
            freeFrames.resize(PHYSICAL_SIZE / PAGE_SIZE, true);
        }

        void writeMemory(int physicalAddress, uint8_t data) {
            if (physicalAddress >= PHYSICAL_SIZE || physicalAddress < 0)
                throw "Physical address out of bounds!";

            physicalMemory[physicalAddress] = data;
        }

        uint8_t readMemory(int physicalAddress) {
            if (physicalAddress >= PHYSICAL_SIZE || physicalAddress < 0)
                throw "Physical address out of bounds!";

            return physicalMemory[physicalAddress];
        }

        int virtualToPhysicalAddress (int virtualAddress) {
            // first let's calculate the offset and find the virtual page number
            int offset = virtualAddress & (PAGE_SIZE - 1); // offset is based on the size of each page
            int virtualPageNumber = virtualAddress / PAGE_SIZE; // essentially shifts the number right by 12 bits

            // now lets validate the virtual address (bounds and valid bit)
            if (virtualPageNumber >= PAGE_COUNT || virtualPageNumber < 0)
                throw "Attempted to access out-of-bound virtual address!";

            const pageTableEntry& entry = pageTable[virtualPageNumber];

            if (!entry.validBit)
                throw "Page fault occurred!";

            // now we can map the virtual to the physical
            int pageFrameNum = entry.pageFrameNum;
            int physicalAddress = (pageFrameNum * PAGE_SIZE) + offset;

            if (physicalAddress >= PHYSICAL_SIZE || physicalAddress < 0)
                throw "Physical address out of bounds!";

            pageTable[virtualPageNumber].referenceBit = true;

            return physicalAddress;
        }

    public:
        MemoryManager()
            : PAGE_SIZE(4096), PAGE_COUNT(1024), PHYSICAL_SIZE(4194304){
            _initializeMemory();
        }

        MemoryManager(int page_size, int num_pages, int num_bytes)
            : PAGE_SIZE(page_size), PAGE_COUNT(num_pages), PHYSICAL_SIZE(num_bytes){
            _initializeMemory();
        }

        void allocatePage(int virtualPageNumber, int frameNumber) {
            if (virtualPageNumber >= PAGE_COUNT || virtualPageNumber < 0)
                throw "Invalid virtual page number!";
            if (frameNumber >= (PHYSICAL_SIZE / PAGE_SIZE) || frameNumber < 0)
                throw "Invalid frame number!";

            pageTable[virtualPageNumber].validBit = true;
            pageTable[virtualPageNumber].pageFrameNum = frameNumber;
            pageTable[virtualPageNumber].presentBit = true;

            freeFrames[frameNumber] = false;
        }

        // returns virtual memory address
        int allocateAnyPage() {
            // find open page entry
            int vpn = -1;
            for (int i = 0; i < PAGE_COUNT; i++) {
                if (pageTable[i].validBit != true) {
                    vpn = i;
                    break;
                }
            }
            if (vpn == -1) throw "No free pages!";

            // find open frame using our map of them :P
            int freeFrame = -1;
            for (int i = 0; i < freeFrames.size(); i++) {
                if (freeFrames[i]) {
                    freeFrame = i;
                    break;
                }
            }
            if (freeFrame == -1) throw "No free physical frames!";

            allocatePage(vpn, freeFrame);

            int virtualAddress = vpn * PAGE_SIZE;
            return virtualAddress;
        }

        void writeVirtualMemory (int virtualAddress, uint8_t data) {
            int physicalAddress = virtualToPhysicalAddress(virtualAddress);
            writeMemory(physicalAddress, data);

            int vpn = virtualAddress / PAGE_SIZE;
            pageTable[vpn].modifyBit = true;
        }

        uint8_t readVirtualMemory (int virtualAddress) {
            int physicalAddress = virtualToPhysicalAddress(virtualAddress);
            return readMemory(physicalAddress);
        }

        void printPageTableEntry(int virtualPageNumber) {
            if (virtualPageNumber >= PAGE_COUNT)
                throw "Invalid page number";

            const pageTableEntry& entry = pageTable[virtualPageNumber];
            std::cout << "Page " << virtualPageNumber << ": ";
            std::cout << "Valid = " << entry.validBit;
            std::cout << ", Present = " << entry.presentBit;
            std::cout << ", Frame = " << entry.pageFrameNum;
            std::cout << ", Referenced = " << entry.referenceBit;
            std::cout << ", Modified = " << entry.modifyBit;
            std::cout << std::endl;
        }
};

int main() {
    try {
        MemoryManager mm;

        for (int i = 0; i < 512; i++) mm.allocatePage(i, i);

        int virtualAddress = mm.allocateAnyPage();

        for (int i = 513; i < 569; i++) mm.allocatePage(i, i);

        int testAddr = mm.allocateAnyPage();

        std::cout << std::showbase << std::hex << "Allocated page for future tests: " << virtualAddress << std::endl;
        std::cout << "Allocated page to see if any page really works: " << testAddr << std::endl << "- - -" << std::endl << std::endl;

        for (int i = 0; i < 4096; i++) {
            uint8_t data = i % 256;
            mm.writeVirtualMemory(virtualAddress + i, data);
        }

        int bloop[5] = {0x0485, 0x0089, 0x0a5f, 0x076e, 0x0f80};

        for (auto random : bloop) {
            int addr = random + virtualAddress;
            std::cout << std::showbase << std::hex;
            std::cout << "Page Table Entry for Virtual Address " << addr << ": " << std::endl;
            mm.printPageTableEntry(addr / 4096);

            std::cout << "Virtual Address: " << addr << std::endl;
            std::cout << "Value at Physical Address: " << std::dec << static_cast<int>(mm.readVirtualMemory(addr)) << std::endl << std::endl;
        }

        return 0;
    } catch (const char* error) {
        std::cerr << "[FATAL] " << error << std::endl;
        return -1;
    }


}
