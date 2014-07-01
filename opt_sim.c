#include <stdio.h>

uint32_t find_next_reference(uint32_t page_num, struct memory_reference *instructions, uint32_t count, uint32_t start){
	uint32_t i;
	for(i = start; i < count; i++){
		if((instructions[i].address>>12) == page_num){
			return i;
		}
	}
    return NEVER_REFERENCED;
}

void opt_sim(struct memory_reference *instructions, uint32_t count, uint32_t *pt, int pages, int frames)
{
	struct opt_node * references;
	references = malloc((1 << 20) * sizeof(struct opt_node));
	memset(references, 0, (1 << 20) * sizeof(struct opt_node));
	char line_buffer[30];
	unsigned int instruction;
	instruction = 0;
	uint32_t page_number;
	struct opt_page valid_pages[frames];
	uint32_t i, k;
	int j, index;
	int next_frame;
    int max;

	for(i = 0; i < frames; i++){
		valid_pages[i].page_number = NOTHING_TO_SEE_HERE;
	}
    
    struct memory_reference current = instructions;

    for(i = 0; i < count; i++){
        index = -1;
        page_number = instructions[i].address >> 12;
        // search for page in frame table.  if it's not there, we need to add it.
        for(j = 0; j < next_frame; j++){
        	if(page_number == valid_pages[j].page_number){
        		index = j;
        	}
        }
        if(index < 0){
        	// we did not find the page in our frames, add it
        	if(next_frame == frames){
        		max = 0;
        		// if we are full, we need to swap
        		for(j = 0; j < next_frame; j++){
                    if(valid_pages[j].next_reference > valid_pages[max].next_reference){
                    	max = j;
                    } else if(valid_pages[j].next_reference == valid_pages[max].next_reference){
                    	if(!is_dirty(pt[valid_pages[max].page_number])){
                    		max = j;
                    	}
                    }
        		} // now we have the page to evict
        		if(is_dirty(pt[valid_pages[max].page_num])){
        			printf("page fault - evict dirty\n");
        			// increase disk write counter
        		} else {
        			printf("page fault - evict clean\n");
        		}
        		valid_pages[max].page_number = page_number;
        		valid_pages[max].next_reference = find_next_reference(instructions, count, i);
        		// update page table
        		
        	} else {
        		valid_pages[next_frame].page_number = page_number; // no eviction
                valid_pages[next_frame].next_reference = find_next_reference(instructions, count, i);
        		printf("page fault - no eviction\n");
        		next_frame++;
        	}
        } else {
        	printf("hit\n");
        }   
    }

    // keep a list of instruction numbers for each page
    // 
}