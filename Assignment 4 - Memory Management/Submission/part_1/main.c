#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

// GLOBAL VARIABLES
unsigned char memory[64][256] ;
unsigned int page_table[256] ;
unsigned int current_empty_frame = 0 ;
float page_faults = 0 ;

unsigned int clock = 0;

// masks
unsigned int frame_number_mask = 255 ;    //    0000 00 0 0 11111111    will do &
unsigned int valid_bit_mask = 256 ;         //  0000 00 0 1 00000000    will do &
unsigned int dirty_bit_mask = 512 ;       //    0000 00 1 0 00000000    will do &
unsigned int bit_counter_mask = 3072 ;    //    0000 11 0 0 00000000    will do &

unsigned int valid_bit_off = 3839 ;       //   0000 11 1 0 11111111    will do &
unsigned int set_recently_used_1_0 = 2048 ;  // 0000 10 0 0 00000000    will do |
unsigned int set_modified_bit  = 1024 ;      // 0000 01 0 0 00000000    will do |
unsigned int change_1_1_to_1_0 = 3071 ;   //   0000 10 1 1 11111111    will do &
unsigned int change_1_0_to_0_1_part1 = 3072 ;// 0000 11 0 0 00000000    will do |    will make counter bits 11
unsigned int change_1_0_to_0_1_part2 = 2047 ;//0000 01 1 1 11111111    will do &    will make counter bits 11 to 01
unsigned int change_0_1_to_0_0 = 1023 ;    //  0000 00 1 1 11111111    will do &

FILE *backing_store ;

void initialise()
{
    // initialising memory
    for(int i = 0; i<64; i++)
    {
        for(int j=0; j<256; j++)
        {
            memory[i][j] = 0 ;
        }
    }

    for(int j=0; j<256; j++)
    {
        page_table[j] = 0 ;
    }

    return ;
}

 unsigned int extract_page_offset( unsigned int a)
{
    unsigned short int page_offset = a % (16*16) ;
    return page_offset ;
}

 unsigned int extract_page_number( unsigned int a)
{
    unsigned short int page_number = a % (16*16*16*16) ;
    page_number = page_number / (16*16) ;
    return page_number ;
}

unsigned int second_chance_algorithm()
{
    unsigned int victim_frame = 0 ;
    unsigned int counter = clock ;
    while(1)
    {
        unsigned int valid_bit_victim = !!(page_table[counter] & valid_bit_mask) ;
        unsigned int dirty_bit_victim = !!(page_table[counter] & dirty_bit_mask) ;
        unsigned int bit_counter_victim = (page_table[counter] & bit_counter_mask) ;
        unsigned int frame_number_victim = (page_table[counter] & frame_number_mask) ;

        if(valid_bit_victim == 1)
        {
            if(bit_counter_victim == 0)  // if bit counter is 00
            {
                clock = (clock+1) % 256 ;
                victim_frame = frame_number_victim ;
                page_table[counter] = page_table[counter] & 0xFEFF ;
                //check if dirty then replace in backing store ;
                if(dirty_bit_victim == 1)
                {
                    fseek(backing_store,counter*256,SEEK_SET);
                    unsigned char data[256];
                    memcpy(data, memory[victim_frame], 256 );
                    fwrite(data,1,256,backing_store);
                }
                return victim_frame ;
            }
            else if(bit_counter_victim == 1024 )  // if bit counter is 01
            {
                page_table[counter] = (page_table[counter] & 0xFBFF) ;
            }
            else if(bit_counter_victim == 2048 ) // if bit counter is 10
            {
                page_table[counter] = 0xF7FF & page_table[counter]; // turning off reference bit
                page_table[counter] = 0x400 | page_table[counter];

            }
            else if(bit_counter_victim == 3072)
            {
                page_table[counter] = (page_table[counter] & 0xFBFF) ;
            }
        }

        counter = (counter+1) % 256 ;
    }
}

int main()
{
    initialise() ;

    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    fp = fopen("addresses.txt", "r");

    backing_store = fopen("BACKING_STORE_1.bin", "rb");

    FILE *file_pointer;
    file_pointer = fopen("out.txt", "w") ;

    unsigned int i = 0;
    while ((read = getline(&line, &len, fp)) != -1) {
        if (1)
        {
            char *log_address = strtok(line," ") ;
            char *op = strtok(NULL," ") ;

            unsigned  int operation = atoi(op) ;                                  // extracting operation
            unsigned char *operation_print ;
            unsigned int logical_address = (unsigned int)strtol(log_address, NULL, 16);   // extracting logical address as hexadecimal

            unsigned int page_offset = extract_page_offset(logical_address) ;
            unsigned int page_number = extract_page_number(logical_address) ;

            unsigned int valid_bit = !!(page_table[page_number] & valid_bit_mask) ;
            unsigned int dirty_bit = !!(page_table[page_number] & dirty_bit_mask) ;
            unsigned int bit_counter = (page_table[page_number] & bit_counter_mask) ;
            unsigned int frame_number = page_table[page_number] & frame_number_mask ;

            if(valid_bit == 0)            // fage fault occuring
            {
                page_faults ++ ;
                // this frame will become the victim frame
                unsigned int frame_to_replace = 0 ;

                // memory filling for the first time
                if(current_empty_frame < 63)
                {
                    frame_to_replace = current_empty_frame ;
                    current_empty_frame ++ ;
                }
                else  // enhanced second change algo will be used here
                {
                    frame_to_replace = second_chance_algorithm() ;
                }

                // second enhaced alogirithm will give us frame number
                fseek(backing_store,page_number*256,SEEK_SET);
                unsigned char data[256];
                fread(data,1,256,backing_store);
                memcpy( memory[frame_to_replace], data, 256 );

                // preparing entry for page table to update
                page_table[page_number] = frame_to_replace ;
                page_table[page_number] = (page_table[page_number] | 0x900) ;  // setting the bit counters as 10

                unsigned char value = memory[frame_to_replace][page_offset] ;

                if(operation == 0 )
                {
                    fprintf(file_pointer, "0x%X%X   0x%X%X   Read   0x%X   Yes\n", page_number, page_offset, frame_to_replace, page_offset, value) ;
                }
                else if(operation == 1)
                {
                    value = value/2 ;
                    page_table[page_number] = (page_table[page_number] | 0x600) ;
                    memory[frame_to_replace][page_offset] = value ;
                    fprintf(file_pointer, "0x%X%X   0x%X%X   Write   0x%X   Yes\n", page_number, page_offset, frame_to_replace, page_offset, value) ;
                }
            }
            else
            {
                unsigned char value = memory[frame_number][page_offset] ;
                page_table[page_number] = (page_table[page_number] | 0x900 ) ;

                if(operation == 0 )
                {
                    fprintf(file_pointer, "0x%X%X   0x%X%X   Read   0x%X   No\n", page_number, page_offset, frame_number, page_offset, value) ;
                }
                else if(operation == 1)
                {
                    value = value/2 ;
                    page_table[page_number] = (page_table[page_number] | 0x600 ) ;
                    memory[frame_number][page_offset] = value ;
                    fprintf(file_pointer, "0x%X%X   0x%X%X   Write   0x%X   No\n", page_number, page_offset, frame_number, page_offset, value) ;
                }
            }
        }
    }
    fclose(file_pointer);
    printf("page-faults rate = %f\n", page_faults*100/100000) ;
}