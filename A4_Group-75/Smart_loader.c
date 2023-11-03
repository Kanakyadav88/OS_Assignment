#include "loader.h"

// Global variables to store elf header and program header
Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;

// Initializing the variables to keep track of it.
int page_faults = 0;
int page_allocations = 0;
long total_internal_fragmentation = 0;
unsigned char *allocated_pages = NULL; // Track page allocations
unsigned long total_allocated_memory = 0;
unsigned long total_memory_allocated = 0;

// Release memory & other cleanups
void loader_cleanup() {
    free(ehdr); ehdr = NULL;
    free(phdr); phdr = NULL;
    if (fd != -1) close(fd), fd = -1;
}

void seg_fault_handler(int signo, siginfo_t *si, void *context){
    // handle segmentation fault
    if (si->si_signo == SIGSEGV) {
        void *fault_addr = si->si_addr;
        // unsigned int page_size = getpagesize();
        unsigned int page_size = 4096;

        // printf("Page size %d\n",page_size);
        void *page_start = (void *)((uintptr_t)fault_addr & ~(page_size - 1));
        // printf("fault address %p\n", fault_addr);
        if (allocated_pages[(uintptr_t)page_start / page_size] != 1) {
            // This is a page fault
                for (int i = 0; i < ehdr->e_phnum; i++) {
                    Elf32_Phdr phdr;
                   // Calculate the offset for the i-th Program Header entry
                    off_t offset = ehdr->e_phoff + i * ehdr->e_phentsize;

                    // Seek to the calculated offset
                    if (lseek(fd, offset, SEEK_SET) == -1) {
                        perror("lseek");
                        exit(1);
                    }

                    // Read the Program Header entry into phdr
                    if (read(fd, &phdr, ehdr->e_phentsize) == -1) {
                        perror("read");
                        exit(1);
                    }
                if (phdr.p_type == PT_LOAD &&
                    (uintptr_t)page_start >= phdr.p_vaddr &&
                    (uintptr_t)page_start < phdr.p_vaddr + phdr.p_memsz) {
                        // printf("p starting address %x\n",phdr.p_vaddr);

                    // Calculate the size to mmap (round up to the page size)
                    size_t map_size = ((phdr.p_memsz + page_size - 1) / page_size) * page_size;
                    page_faults += map_size/4096;
                    
                    // printf("segment size %d\n",phdr.p_memsz);
                    // printf("map size %d\n",map_size);

                    void *segment_memory = mmap(page_start, map_size, PROT_READ | PROT_WRITE | PROT_EXEC,
                                                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

                    if (segment_memory == MAP_FAILED) {
                        perror("Error mapping memory for segment");
                        exit(1);
                    }
                    read(fd, segment_memory, phdr.p_filesz);
                    int fragmentaion = map_size - phdr.p_memsz;
                    total_internal_fragmentation+= fragmentaion;
                    // printf("total internal fra %ld\n",total_internal_fragmentation);

                    // Track allocated memory
                    total_allocated_memory += map_size;
                    
                    // Calculate the difference between the requested size and the mapped size
                    long memory_allocated_in_segment = map_size - phdr.p_memsz ;

                    // Update the total_memory_allocated
                    total_memory_allocated += memory_allocated_in_segment;
                    // printf("Result total_allocated_memory: %ld\n", total_allocated_memory);

                    // Copy segment contents
                     if (lseek(fd, phdr.p_offset, SEEK_SET) == -1) {
                        perror("lseek");
                        exit(1);
                    }
                    if (read(fd, segment_memory, phdr.p_filesz) == -1) {
                        perror("read");
                        exit(1);
                    }
                    
                    
                    page_allocations++;
                    // page_allocations++;
                    allocated_pages[(uintptr_t)page_start / page_size] = 1;
                    // printf("Result allocated_pages: %hhn\n", allocated_pages);
                    // printf("page allocations: %d\n", page_allocations);
                    // printf("..............................................\n");
                    
                }

                //munmap(segment_memory, map_size);
            }
        }
    }
}

void load_and_run_elf(char *exe) {
    fd = open(exe, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        exit(1);
    }
//  Allocate memort to store the entire ELF File
    ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
   if (ehdr == NULL) {
        perror("Failed to allocate memory for ELF header");
        loader_cleanup();
        return;
    }
    ssize_t bytes_read = read(fd, ehdr, sizeof(Elf32_Ehdr));
    if (bytes_read != sizeof(Elf32_Ehdr)) {
        perror("Failed to read ELF header");
        loader_cleanup();
        free(ehdr);  // Free the allocated memory
        return;
    }

    // Calculate the maximum address space size
    unsigned int max_address = 0;
    for (int i = 0; i < ehdr->e_phnum; i++) {
        Elf32_Phdr phdr;
         // Calculate the offset for the i-th Program Header entry
        off_t offset = ehdr->e_phoff + i * ehdr->e_phentsize;

        // Seek to the calculated offset
        if (lseek(fd, offset, SEEK_SET) == -1) {
            perror("lseek");
            exit(1);
        }

        // Read the Program Header entry into phdr
        if (read(fd, &phdr, ehdr->e_phentsize) == -1) {
            perror("read");
            exit(1);
        }

        if (phdr.p_type == PT_LOAD) {
            if (phdr.p_vaddr + phdr.p_memsz > max_address) {
                max_address = phdr.p_vaddr + phdr.p_memsz;
            }
        }
    }

    // Calculate the number of pages required for the address space
    size_t page_count = (size_t)(max_address / getpagesize()) + 1;

    // Allocate memory for tracking allocated pages
    allocated_pages = (unsigned char *)calloc(page_count, sizeof(unsigned char));

    if (allocated_pages == NULL) {
        perror("Error allocating memory for tracking pages");
        exit(1);
    }

    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = seg_fault_handler;
    sigaction(SIGSEGV, &sa, NULL);

    void *entrypoint = (void *)(ehdr->e_entry);

    // Define function pointer type for _start and use it to typecast function pointer properly
    typedef int (*Start_Func)();
    Start_Func _start = (Start_Func)entrypoint;
    if (_start == NULL) {
        perror("Failed to cast entry point to function pointer");
        free(phdr);
        loader_cleanup();
        return;
    }

    int result = _start();
    
    loader_cleanup();
    printf("..............................................\n");
    printf("Result from _start: %d\n", result);
    printf("Page Faults: %d\n", page_faults);
    printf("Page Allocations: %d\n", page_allocations);
    printf("Internal Fragmentation: %.2f KB\n", (float)total_internal_fragmentation/1024);
    printf("...............................................\n");
    loader_cleanup();
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <ELF Executable>\n", argv[0]);
        exit(1);
    }

    load_and_run_elf(argv[1]);
    loader_cleanup();
    return 0;
}