#include "loader.h"

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;

void loader_cleanup() {
    free(ehdr); ehdr = NULL;
    free(phdr); phdr = NULL;
    if (fd != -1) close(fd), fd = -1;
}

void load_and_run_elf(char *exe) {
    fd = open(exe, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return;
    }

    // Read ELF header
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
    // 1. Get the size of the ELF file
    off_t file_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    
    read(fd, ehdr, sizeof(Elf32_Ehdr));

    // Find the PT_LOAD segment with entrypoint
    phdr = (Elf32_Phdr *)malloc(ehdr->e_phentsize);
    if (phdr == NULL) {
      perror("Failed to allocate memory for program header");
      loader_cleanup();
      return;
}

    lseek(fd, ehdr->e_phoff, SEEK_SET);
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (read(fd, phdr, ehdr->e_phentsize) != ehdr->e_phentsize) {
            perror("Failed to read program header");
            free(phdr);
            loader_cleanup();
            return;
    }

    if (phdr->p_type == PT_LOAD &&
        ehdr->e_entry >= phdr->p_vaddr &&
        ehdr->e_entry < phdr->p_vaddr + phdr->p_memsz) {
        break;
    }
  }

    if (phdr->p_type != PT_LOAD ||
        ehdr->e_entry < phdr->p_vaddr ||
        ehdr->e_entry >= phdr->p_vaddr + phdr->p_memsz) {
        perror("Failed to find suitable PT_LOAD segment for entry point");
        free(phdr);
        loader_cleanup();
        return;
  }


    // Allocate memory using mmap
    void *virtual_memory = mmap(NULL, phdr->p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC,
                                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    lseek(fd, phdr->p_offset, SEEK_SET);
    read(fd, virtual_memory, phdr->p_filesz);

    // Calculate entrypoint address within the loaded segment
   void *entrypoint = virtual_memory + (ehdr->e_entry - phdr->p_vaddr);

    // Define function pointer type for _start and use it to typecast function pointer properly
    typedef int (*Start_Func)();
    Start_Func _start = (Start_Func)entrypoint;
    if (_start == NULL) {
        perror("Failed to cast entry point to function pointer");
        free(phdr);
        loader_cleanup();
        return;
    }

    // Call the _start function and print the result
    int result = _start();
    printf("Result from _start: %d\n", result);

    free(phdr);
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