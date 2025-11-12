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
// Includ header-ul unde este declarata structura rlimit, macro-ul RLIM_INFINITY, respectiv functia getrlimit()
#include <sys/resource.h>

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

			// Task 3:  Load memory regions with correct permissions

			// In variabila flags retin permisiunile pentru mmap()
			int flags = 0;

			// Permisiunea de citire (READ)
			if (ph_i->p_flags & PF_R)
				flags |= PROT_READ;
			// Permisiunea de scriere (WRITE)
			if (ph_i->p_flags & PF_W)
				flags |= PROT_WRITE;
			// Permisiunea de executare (EXECUTABLE)
			if (ph_i->p_flags & PF_X)
				flags |= PROT_EXEC;

			// Modific permisiunile pe zone mapate/rezervate
			if (mprotect(segment, map_size, flags) == -1) {
				// Afisiez mesajul/motivul de eroare pentru mprotect()
				perror("mprotect");
				exit(1);
			}
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

	// rlimit => soft limit, hard limit
	struct rlimit stack_limit;
	unsigned long stack_size;

	// Cer limitele sistemului pentru stiva
	if (getrlimit(RLIMIT_STACK, &stack_limit) == -1) {
		perror("getrlimit");
		exit(1);
	}

	// Daca sistemul a impus limita curenta a stivei = inifnit, setez la o valoare rezonabila
	// Altfel, setez limita stivei cea impusa de sistem
	if (stack_limit.rlim_cur == RLIM_INFINITY)
		stack_size = 8 * 1024 * 1024; // 8 MB
	else
		stack_size = stack_limit.rlim_cur;

// I. Rezerv o zona de memorie pentru stiva
	void *sp_bottom = mmap(NULL, stack_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if (sp_bottom == MAP_FAILED) {
		perror("mmap stack");
		exit(1);
	}

	// Masca pentru alinierea stivei (16 bytes)
	unsigned long mask_stack = ~0xF;

	// Incep sa pun datele la cea mai mare adresa (stiva "creste" de sus in jos)
	void *sp = sp_bottom + stack_size;

	// Aliniez stiva inainte sa pun date in ea
	sp = (void *)((unsigned long)sp & mask_stack);

// II. Pun stringurile brute pentru argv si envp pe stiva
	// Contorizez numarul variabilelor envp
	int envc = 0;

	while (envp[envc] != NULL)
		envc++;

	// new_envp stocheaza adresele string-urilor variabilelor de mediu
	char *new_envp[envc + 1];

	for (i = 0; i < envc; i++) {
		// "+1" pentru '\0'
		sp -= strlen(envp[i]) + 1;
		memcpy(sp, envp[i], strlen(envp[i]) + 1);
		new_envp[i] = (char *)sp;
	}
	new_envp[envc] = NULL;

	// new_argv stocheaza adresele string-urilor argumentelor din CLI
	char *new_argv[argc + 1];

	for (i = 0; i < argc; i++) {
		sp -= strlen(argv[i]) + 1;
		memcpy(sp, argv[i], strlen(argv[i]) + 1);
		new_argv[i] = (char *)sp;
	}
	new_argv[argc] = NULL;

	// "3020: random gap padded to 16B boundary" (Documentatie)
	sp = (void *)((unsigned long)sp & mask_stack);

// III. auxv data
	// Pun 16 random bytes pe stiva (Zona la care pointeaza AT_RANDOM) pentru securitate impotriva SSP (Stack Smashing Protection)
	unsigned char random_bytes[16];
	int random_fd = open("/dev/urandom", O_RDONLY);

	if (random_fd < 0) {
		perror("/dev/urandom");
		memset(random_bytes, 0, 16);
	} else {
		read(random_fd, random_bytes, 16);
		close(random_fd);
	}

	// Pun pe stiva cei 16 bytes random
	sp -= 16;
	memcpy(sp, random_bytes, 16);
	void *AT_RANDOM_ptr = sp;

// IV. Aliniez stiva (obligatoriu la 16 bytes pe x86-64 => ABI)
	// "3000: zero padding to align stack" (Documentatie)
	sp = (void *)((unsigned long)sp & mask_stack);

// V. Pun pe stiva vectorul auxiliar auxv (cheie -> valoare, de sus in jos)
	// Creez un pointer de tipul unsigned long pt usurinta
	unsigned long *stack_ptr = (unsigned long *)sp;

	// Valoare
	*(--stack_ptr) = 0;
	// Cheie
	*(--stack_ptr) = AT_NULL;

	// Valoare
	*(--stack_ptr) = (unsigned long)AT_RANDOM_ptr;
	// Cheie
	*(--stack_ptr) = AT_RANDOM;

// VI. Pun pe stiva adresele string-urilor brute ale lui envp, resp argv si la final argc
	// Le pun in ordine descrescatoare (NULL-ul se afla in sir)
	for (i = envc; i >= 0; i--)
		*(--stack_ptr) = (unsigned long)new_envp[i];

	for (i = argc; i >= 0; i--)
		*(--stack_ptr) = (unsigned long)new_argv[i];

	*(--stack_ptr) = (unsigned long)argc;

// VII. Setez pozitia finala a pointer-ului stivei (sp)
	sp = (void *)stack_ptr;

	// void *sp = NULL;

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
