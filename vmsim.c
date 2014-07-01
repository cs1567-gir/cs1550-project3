#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>
#include <time.h>

#define BYTES_PER_LINE  11

#define VALID_BIT 		20
#define DIRTY_BIT 		21
#define REFERENCED_BIT 	22

#define NOTHING_TO_SEE_HERE 4294967295
#define NEVER_REFERENCED    4294967295
#define TWENTY_ONES         1048575

struct memory_reference {
    uint32_t address;
    char mode;
};


struct opt_page {
    uint32_t page_number;
    uint32_t next_reference; //0 means not referenced anymore
};

int is_valid(uint32_t *pt, uint32_t page) {
    int valid;
    valid = pt[page] & 1<<VALID_BIT;
	return valid;
}

int is_dirty(uint32_t *pt, uint32_t page) {
    int dirty;
    dirty = pt[page] & 1<<DIRTY_BIT;
	return dirty;
}

int is_referenced(uint32_t *pt, uint32_t page) {
    int referenced;
    referenced = pt[page] & 1<<REFERENCED_BIT;
    return referenced;
}

uint32_t find_next_reference(uint32_t page_num, struct memory_reference *instructions, uint32_t count, uint32_t start){
	uint32_t i;
	uint32_t retVal = NEVER_REFERENCED;
	for(i = start; i < count; i++){
		if((instructions[i].address>>12) == page_num){
			//printf("page %u's next reference: %u\n", page_num, i);
			retVal = i;
			break;
		}
	}
    return retVal;
}

void set_valid(uint32_t *pt, uint32_t page_num){
	pt[page_num] = pt[page_num] | 1<<VALID_BIT;
}

void set_dirty(uint32_t *pt, uint32_t page_num){
	pt[page_num] = pt[page_num] | 1<<DIRTY_BIT;
}

void set_referenced(uint32_t *pt, uint32_t page_num){
	pt[page_num] = pt[page_num] | 1<<REFERENCED_BIT;
}

void clear_status_bits(uint32_t *pt, uint32_t page_num){
	uint32_t mask = 7<<VALID_BIT;
	mask = ~mask;
	pt[page_num] = pt[page_num] & mask;
}


/**
 * opt_sim
 * simulates the OPT algorithm, using perfect foreknowledge of the memory references
 * to decide which valid page will be unused for the longest time and evict that page
 **/ 
void opt_sim(struct memory_reference *instructions, uint32_t count, uint32_t *pt, int pages, int frames)
{
	uint32_t page_number;					// the page number of the current reference
	struct opt_page valid_pages[frames];	// an inverse page table that also stores the next reference of the framed page
	uint32_t i, k;							// iterators that *might be too big for signed int
	int j, index;							// smaller iterators or indexes
	int next_frame;							// the number of frames in memory that are full
    int max;								// the IPT index of the page with the next reference furthest in the future
    int writes;								// how many disk writes
    int faults;								// how many total page faults

    next_frame = 0;
    writes = 0;
    faults = 0;

    printf("Running OPT simulation.\n");

	for(i = 0; i < frames; i++){
		valid_pages[i].page_number = NOTHING_TO_SEE_HERE;
	}

    printf("Beginning simulation.\n");
    for(i = 0; i < count; i++){									
        page_number = instructions[i].address >> 12;		// calculate page_number = top 20 bits of address
 
        if(!is_valid(pt, page_number)){
        	faults++;
        	if(next_frame == frames){						// if RAM is full, swap out another page and swap in the current page
        		max = 0;
        		for(j = 0; j < next_frame; j++){			// find the index of the page that will be referenced furthest in the future (linear search over frames)
                    if(valid_pages[j].next_reference > valid_pages[max].next_reference){
                    	max = j;
                    } else if(valid_pages[j].next_reference == valid_pages[max].next_reference){	// if there is a tie, prefer clean pages
                    	if(!is_dirty(pt, valid_pages[max].page_number)){
                    		max = j;
                    	}
                    } // else keep iterating
        		} 											// now we have the page to evict
        		if(is_dirty(pt, valid_pages[max].page_number)){	// if the page is dirty, increment the number of disk writes
        			printf("%i. page fault - evict dirty ", i);
        			// increase disk write counter
        			writes++;
        		} else {
        			printf("%i. page fault - evict clean ", i);
        		}
                
                printf(" (%i)\n", max);
                clear_status_bits(pt, valid_pages[max].page_number);			// clear R, D, V bits

        		valid_pages[max].page_number = page_number;						// update IPT
        		valid_pages[max].next_reference = find_next_reference(page_number, instructions, count, i + 1);		// linear search over remaining instructions for next reference

        		// update page table
                pt[page_number] = max;
        		set_valid(pt, page_number);
        		if(instructions[i].mode == 'W'){
        			set_dirty(pt, page_number);
        		}
        	} else {														// else there are still free frames
                printf("%i. page fault - no eviction\n", i);
        		valid_pages[next_frame].page_number = page_number;
                valid_pages[next_frame].next_reference = find_next_reference(page_number, instructions, count, i + 1);
                printf("next reference: %u\n", valid_pages[next_frame].next_reference);
 
        		// update page table
                pt[page_number] = next_frame;
        		set_valid(pt, page_number);
        		if(instructions[i].mode == 'W'){
        			set_dirty(pt, page_number);
        		}
        		// increase valid page counter
        		next_frame++;
        	}
        } else {								// else the page was already in memory
        	printf("%i. hit\n", i);

        	// find the next time this page is referenced
        	valid_pages[pt[page_number] & TWENTY_ONES].next_reference = find_next_reference(page_number, instructions, count, i + 1);

        	// if this was a write, update the dirty bit
        	if(instructions[i].mode == 'W'){
        		set_dirty(pt, page_number);
        	}
        }
    } // end main for loop

    // print some information
    printf("Number of frames: %i\n", frames);
    printf("Total memory accesses: %i\n", i);
    printf("Total page faults: %i\n", faults);
    printf("Total writes to disk: %i\n", writes);
}

void rand_sim(struct memory_reference *instructions, uint32_t count, uint32_t *pt, int pages, int frames)
{
	uint32_t page_number;
	uint32_t *ram;
    int valid_pages;
	ram = malloc(frames * sizeof(uint32_t));
    valid_pages = 0;
    uint32_t i;
    int to_evict;
    int writes;
    int faults;

    writes = 0;
    faults = 0;

	srand(time(NULL));	// seed random simulator with time
	// begin
    for(i = 0; i < count; i++){
    	page_number = instructions[i].address >> 12;               // calculate page = top 20 bits of address
    	if(!is_valid(pt, page_number)){                            // if its not already valid
            faults++;
        	if(valid_pages < frames){
        		// just put page at end
                ram[valid_pages] = page_number;
                printf("%i. page fault - no eviction\n", i);
                pt[page_number] = valid_pages;
                set_valid(pt, page_number);
                if(instructions[i].mode == 'W'){
                    set_dirty(pt, page_number);
                }
                valid_pages++;
        	} else {
        		// we need to swap
        		// pick a random frame to evict
        	    to_evict = rand() % frames;
        	    if(is_dirty(pt, ram[to_evict])){
        	    	// disk write
                    writes++;
                    printf("%i. page fault - evict dirty\n", i);
        	    } else {
        	    	// clean eviction
                    printf("%i. page fault - evict clean\n", i);
            	}
                clear_status_bits(pt, ram[to_evict]);
                ram[to_evict] = page_number;
                pt[page_number] = to_evict;
                set_valid(pt, page_number);
                if(instructions[i].mode == 'W'){
                    set_dirty(pt, page_number);
                }
        	}
    	} else {
    		// hit
            printf("%i. hit\n", i);
            if(instructions[i].mode == 'W'){
                    set_dirty(pt, page_number);
            }
    	}
    }
    printf("Number of frames: %i\n", frames);
    printf("Total memory accesses: %i\n", i);
    printf("Total page faults: %i\n", faults);
    printf("Total writes to disk: %i\n", writes);
}

void nru_sim(FILE *fp, uint32_t *pt, int pages, int frames, int period)
{

}

void aging_sim(FILE *fp, uint32_t *pt, int pages, int frames, int period)
{

}

int main(int argc, char *argv[])
{
    FILE *infile;
    char *filemode = "r";
    int frames;
    char *algorithm;
    int refresh_rate;
    char *trace_file;

    uint32_t *page_table; 
    uint32_t pages;   				// 2^32 / 2^12 = 2^20
    pages = 1<<20;

    uint32_t i;

    uint32_t file_size;
    uint32_t instruction_count;
    struct stat buf;

    struct memory_reference *instructions;
    int address;
    char mode;


    page_table = malloc(pages * 4); // num_pt_entries * sizeof(uint32_t)
    memset(page_table, 0, pages * 4);

    frames = atoi(argv[2]);
    algorithm = argv[4];

    if(argc > 6) {
    	refresh_rate = atoi(argv[6]);
    }

    trace_file = argv[argc - 1];

    // get file size to determine how many instructions we have
    stat(trace_file, &buf);
    file_size = buf.st_size;
    instruction_count = file_size / BYTES_PER_LINE;
    printf("instruction count: %u\n", instruction_count);

    // open file
    infile = fopen(trace_file, filemode);
    if(infile == NULL) {
        fprintf(stderr, "Unable to open trace file.\n");
        exit(1);
    }

    // construct head of list
    instructions = malloc(instruction_count * sizeof(struct memory_reference));

    printf("Loading instruction list into memory...\n");
    for(i = 0; i < instruction_count; i++){
    	fscanf(infile, "%x %c", &address, &mode);
    	instructions[i].address = address;
    	instructions[i].mode = mode;
    }

    if(strcmp(algorithm, "opt") == 0){
        opt_sim(instructions, instruction_count, page_table, pages, frames);
    } else if(strcmp(algorithm, "rand") == 0) {
    	rand_sim(instructions, instruction_count, page_table, pages, frames);
    } else if(strcmp(algorithm, "nru") == 0) {
    	//nru_sim(infile);
    } else if(strcmp(algorithm, "aging") == 0) {
    	//aging_sim(infile);
    } else {
    	// print usage
    }
    fclose(infile);
    free(instructions);
    free(page_table);
    printf("DONE.\n");
}
