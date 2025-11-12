// SPDX-License-Identifier: BSD-3-Clause

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

// Includ header-ul unde este declarat macro-ul ELFCLASS64
#include <elf.h>
// Includ header-ul unde este declarata functia memcpy()
#include <string.h>

void *map_elf(const char *filename)
{
	// This part helps you store the content of the ELF file inside the buffer.
	struct stat st;
	void *file;
	int fd;

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		perror("open");
		exit(1);
	}

	fstat(fd, &st);

	file = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (file == MAP_FAILED) {
		perror("mmap");
		close(fd);
		exit(1);
	}

	return file;
}

void load_and_run(const char *filename, int argc, char **argv, char **envp)
{
	// Contents of the ELF file are in the buffer: elf_contents[x] is the x-th byte of the ELF file.
	void *elf_contents = map_elf(filename);

	/**
	 * TODO: ELF Header Validation
	 * Validate ELF magic bytes - "Not a valid ELF file" + exit code 3 if invalid.
	 * Validate ELF class is 64-bit (ELFCLASS64) - "Not a 64-bit ELF" + exit code 4 if invalid.
	 */

    // Verific primii 4 bytes (magic) din continutul fisierului ELF
	unsigned char *elf = (unsigned char *)elf_contents;

	// Puteam sa fac si memcmp(elf, ELFMAG, 4) != 0
	if (elf[0] != 0x7f || elf[1] != 0x45 || elf[2] != 0x4c || elf[3] != 0x46) {
		fprintf(stderr, "Not a valid ELF file");
		exit(3);
	}

    // Verific daca este un ELF pe 64 biti
	if (elf[4] != ELFCLASS64) {
		fprintf(stderr, "Not a 64-bit ELF");
		exit(4);
	}

	/**
	 * TODO: Load PT_LOAD segments
	 * For minimal syscall-only binaries.
	 * For each PT_LOAD segment:
	 * - Map the segments in memory. Permissions can be RWX for now.
	 */

	Elf64_Ehdr *ELF_header = (Elf64_Ehdr *)elf_contents;
	Elf64_Phdr *Program_header_table = (Elf64_Phdr *)(elf_contents + ELF_header->e_phoff);

	// In cazul asta, page_size == p_align == 4kb
	// Pentru genericitate, pt un fisier mai vechi unde p_align = 4kb, dar pe un MacM1 page_size = 16kb,
	//trebuie facut sysconf
	long page_size = sysconf(_SC_PAGE_SIZE);

	// Masca pentru aliniere la 4kb (pt mmap)
	unsigned long mask = ~(page_size - 1);

	// Offset-ul in pagina unde trebuie sa copiez segmentul
	unsigned long page_offset;

	// Dimensiunea memoriei rezervate
	unsigned long map_size;

	// Adresa aliata unde voi rezerva memorie (alinierea este obligatorie pt mmap)
	void *aligned_mmap_addr = NULL;

	// Adresa propriu-zisa unde voi scrie byte-urile
	void *mmap_addr = NULL;

	int i;
	// Al i-lea segment
	Elf64_Phdr *ph_i = NULL;

	// Adresa rezultata dupa mmap()
	void *segment = NULL;

	for (i = 0; i < ELF_header->e_phnum; i++) {
		if (Program_header_table[i].p_type == PT_LOAD) {
			ph_i = &Program_header_table[i];

			// Verific proprietatea de aliniere
			if (ph_i->p_align > 1) {
				if ((ph_i->p_vaddr % ph_i->p_align) != (ph_i->p_offset % ph_i->p_align)) {
					fprintf(stderr, "ELF file is invalid or corrupt!\n");
					exit(5);
				}
			}

			mmap_addr = (void *)(ph_i->p_vaddr);
			aligned_mmap_addr = (void *)((unsigned long)mmap_addr & mask);
			page_offset = mmap_addr - aligned_mmap_addr;
			map_size = ph_i->p_memsz + page_offset;

			// Maparea initiala cu toate permisiunile (RWX)
			segment = mmap(aligned_mmap_addr, map_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);

			// Afisez mesajul/motivul de eroare (stocat in errno) pentru mmap()
			if (segment == MAP_FAILED) {
				perror("mmap");
				exit(1);
			}

			// Copierea memoriei pentru segmentele PT_LOAD
			memcpy(segment + page_offset, elf_contents + ph_i->p_offset, ph_i->p_filesz);
		}
	}

	/**
	 * TODO: Load Memory Regions with Correct Permissions
	 * For each PT_LOAD segment:
	 *	- Set memory permissions according to program header p_flags (PF_R, PF_W, PF_X).
	 *	- Use mprotect() or map with the correct permissions directly using mmap().
	 */

	/**
	 * TODO: Support Static Non-PIE Binaries with libc
	 * Must set up a valid process stack, including:
	 *	- argc, argv, envp
	 *	- auxv vector (with entries like AT_PHDR, AT_PHENT, AT_PHNUM, etc.)
	 * Note: Beware of the AT_RANDOM, AT_PHDR entries, the application will crash if you do not set them up properly.
	 */
	void *sp = NULL;

	/**
	 * TODO: Support Static PIE Executables
	 * Map PT_LOAD segments at a random load base.
	 * Adjust virtual addresses of segments and entry point by load_base.
	 * Stack setup (argc, argv, envp, auxv) same as above.
	 */

	// TODO: Set the entry point and the stack pointer
	void (*entry)() = (void (*)())(ELF_header->e_entry);

	// Transfer control
	__asm__ __volatile__(
			"mov %0, %%rsp\n"
			"xor %%rbp, %%rbp\n"
			"jmp *%1\n"
			:
			: "r"(sp), "r"(entry)
			: "memory"
			);
}

int main(int argc, char **argv, char **envp)
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <static-elf-binary>\n", argv[0]);
		exit(1);
	}

	load_and_run(argv[1], argc - 1, &argv[1], envp);
	return 0;
}
