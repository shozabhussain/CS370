#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

// GLOBAL VARIABLES
unsigned char memory[128][1024] = {0} ;
unsigned char page_table_1 = 0 ;
unsigned char current_empty_frame_L1 = 1 ;
unsigned char current_empty_frame_L2 = 33 ;

unsigned clock_l1 = 0;
unsigned clock_l2 = 0;

// masks
unsigned char frame_number_mask = 0x0 ;    //
unsigned char valid_bit_mask = 0x1;         //
unsigned char dirty_bit_mask = 0x2 ;       //
unsigned char bit_counter_mask = 0xC ;    //

unsigned char set_modified = 0x4 ;
unsigned char set_recently_used = 0x8 ;
unsigned char counter_L1 = 0 ;
unsigned char counter_L2 = 0 ;

unsigned char change_1_1_to_1_0 = 0xb ;
unsigned char change_1_0_to_0_1_part2 = 0x7 ;
unsigned char change_0_1_to_0_0 = 0x3 ; // will take &

FILE *backing_store ;

unsigned  int extract_page_offset( unsigned int a)
{
    unsigned int page_offset = a & 0x3FF ;
    return page_offset ;
}

unsigned int extract_page_number( unsigned int a)
{
    unsigned  int page_number = a & 0xFFFC00 ;
    page_number = page_number >> 10 ;
    return page_number ;
}

unsigned int extract_l1_offset( unsigned int a)
{
    unsigned  int l1_off = a & 0x3F00 ;
    l1_off = l1_off >> 8 ;
    return l1_off ;
}

unsigned int extract_l2_offset( unsigned int a)
{
    unsigned  int l2_off = a & 0xFF ;
    return l2_off ;
}

unsigned char second_chance_algorithm_L1()
{
    unsigned char victim_frame = -99 ;
    //counter_L1 = clock_l1;
    while(1)
    {
        unsigned int valid_bit_victim = !!(memory[page_table_1][4*counter_L1+2] & valid_bit_mask) ;
        unsigned int dirty_bit_victim = !!(memory[page_table_1][4*counter_L1+2] & dirty_bit_mask) ;
        unsigned char bit_counter_victim = (memory[page_table_1][4*counter_L1+2] & bit_counter_mask) ;
        unsigned char frame_number_victim = memory[page_table_1][4*counter_L1+0] ;

        if(valid_bit_victim == 1)
        {
            if(bit_counter_victim == 0)  // if bit counter is 00
            {
                clock_l1 = (clock_l1+1) % 64;

                victim_frame = frame_number_victim ;
                memory[page_table_1][4*counter_L1+2] = 0 ;
                memory[page_table_1][4*counter_L1+0] = 0 ;
                memory[page_table_1][4*counter_L1+1] = 0 ;
                memory[page_table_1][4*counter_L1+3] = 0 ;

                if(frame_number_victim < 1 || frame_number_victim > 32)
                {
                    counter_L1 = (counter_L1+1) % 64 ;
                    continue ;
                }

                //check if dirty then replace in backing store ;
                if(dirty_bit_victim == 1)
                {
                    fseek(backing_store,0x00C193E8+8+counter_L1*1024,SEEK_SET);
                    unsigned char data[1024]  ;
                    for(int p=0; p<1024; p++)
                    {
                        data[p] = memory[victim_frame][p];
                    }
                    fwrite(data,1,1024,backing_store);
                }
                counter_L1 = (counter_L1+1) % 64 ;
                return victim_frame ;
            }
            else if(bit_counter_victim == set_modified )  // if bit counter is 01
            {
                memory[page_table_1][4*counter_L1+2] = (memory[page_table_1][4*counter_L1+2] & change_0_1_to_0_0) ;
            }
            else if(bit_counter_victim ==  set_recently_used ) // if bit counter is 10
            {
                memory[page_table_1][4*counter_L1+2] = (memory[page_table_1][4*counter_L1+2] | bit_counter_mask) ; // changes 10 to 11
                memory[page_table_1][4*counter_L1+2]= (memory[page_table_1][4*counter_L1+2] & change_1_0_to_0_1_part2) ; // changes 11 to 01
            }
            else if(bit_counter_victim == bit_counter_mask)
            {
                memory[page_table_1][4*counter_L1+2] = (memory[page_table_1][4*counter_L1+2] & change_1_1_to_1_0) ;
            }
        }

        counter_L1 = (counter_L1+1) % 64 ;
    }
}

unsigned char second_chance_algorithm_L2(unsigned char L2, unsigned int table1_offset)
{
    unsigned char victim_frame = 0 ;
    //counter_L2 = clock_l2;
    while(1)
    {
        unsigned int valid_bit_victim = !!(memory[L2][4*counter_L2+2] & valid_bit_mask) ;
        unsigned int dirty_bit_victim = !!(memory[L2][4*counter_L2+2] & dirty_bit_mask) ;
        unsigned char bit_counter_victim = (memory[L2][4*counter_L2+2] & bit_counter_mask) ;
        unsigned char frame_number_victim = memory[L2][4*counter_L2+0] ;

        if(valid_bit_victim == 1)
        {
            if(bit_counter_victim == 0)  // if bit counter is 00
            {
                clock_l2 = (clock_l2+1) % 256;
                victim_frame = frame_number_victim ;
                memory[L2][4*counter_L2+0] = 0 ;
                memory[L2][4*counter_L2+2] = 0 ;

                unsigned int pageNumber = table1_offset << 8 | counter_L2  ;

                //check if dirty then replace in backing store ;
                if(dirty_bit_victim == 1)
                {
                    fseek(backing_store,pageNumber*1024,SEEK_SET);
                    unsigned char data[1024]  ;
                    for(int p=0; p<1024; p++)
                    {
                        data[p] = memory[victim_frame][p];
                    }
                    fwrite(data,1,1024,backing_store);
                }
                counter_L2 = (counter_L2+1) % 256 ;
                printf("second chance L2 returning %d\n", victim_frame) ;
                return victim_frame ;
            }
            else if(bit_counter_victim == set_modified )  // if bit counter is 01
            {
                memory[L2][4*counter_L2+2] = (memory[L2][4*counter_L2+2] & change_0_1_to_0_0) ;
            }
            else if(bit_counter_victim ==  set_recently_used ) // if bit counter is 10
            {
                memory[L2][4*counter_L2+2] = (memory[L2][4*counter_L2+2] | bit_counter_mask) ; // changes 10 to 11
                memory[L2][4*counter_L2+2]= (memory[L2][4*counter_L2+2] & change_1_0_to_0_1_part2) ; // changes 11 to 01
            }
            else if(bit_counter_victim == bit_counter_mask)
            {
                memory[L2][4*counter_L2+2] = (memory[L2][4*counter_L2+2] & change_1_1_to_1_0) ;
            }
        }

        counter_L2 = (counter_L2+1) % 256 ;
    }
}

int main()
{
    for(int i=0; i<128; i++)
    {
        for(int k=0 ; k<1024; k++)
        {
            memory[i][k] = 0;
        }
    }

    FILE *file_pointer;
    file_pointer = fopen("out.txt", "w") ;
    int j = 0;

    backing_store = fopen("BACKING_STORE_2.bin", "rb+");
    for(unsigned int i = 0x00C17C00; i<=0x00C193E8; i=i+8)
    {

        fseek(backing_store,i,SEEK_SET);
        unsigned char data[8] ;
        fread(data,1,8,backing_store);

        unsigned char opcode = data[0] ;  // extracting opCode

        if(opcode == 0x10 || opcode == 0x20 || opcode == 0x30 || opcode == 0x40 || opcode == 0x50 || opcode == 0x60 || opcode == 0x70)  //memory-value
        {
            unsigned int outer_hit = 0 ;
            unsigned int inner_hit = 0;

            unsigned int address = data[1] << 16  | data[2]<< 8 | data[3] ;    // extracting memory address
            unsigned int value = data[4] << 24 | data[5] << 16 | data[6] << 8 | data[7] ;  // extracting value

            unsigned int page_offset = extract_page_offset(address) ;   // extracting page offset
            unsigned int page_number = extract_page_number(address) ;   // extrating page number global

            unsigned int l1_offset = extract_l1_offset(page_number) ;  // extracting offset in outer page table
            unsigned int l2_offset = extract_l2_offset(page_number) ;  // extracting offset in inner page table

            // 0 pe frame hai and 2 pe baki ke bits hain
            unsigned int valid_bit_L1 = !!(memory[page_table_1][4*l1_offset+2] & valid_bit_mask) ;
            unsigned int dirty_bit_L1 = !!(memory[page_table_1][4*l1_offset+2] & dirty_bit_mask) ;
            unsigned char bit_counter_L1 = (memory[page_table_1][4*l1_offset+2] & bit_counter_mask) ;
            unsigned char frame_number_L1 = memory[page_table_1][4*l1_offset+0] ; // tells us which in which frame L2 exists

            unsigned char location_l2 ;  // frame number to find l2 table
            if(valid_bit_L1 == 0)
            {
                outer_hit = 1 ;
                if(current_empty_frame_L1 < 33)
                {
                    location_l2 = current_empty_frame_L1 ;
                    current_empty_frame_L1 ++ ;
                }
                else  // enhanced second chance algorithm
                {
                    location_l2 = second_chance_algorithm_L1() ;

                    fseek(backing_store,0x00C193E8+8+l1_offset*1024,SEEK_SET);
                    unsigned char data[1024]  ;
                    fread(data,1,1024,backing_store);

                    for(int p=0; p<1024; p++)
                    {
                        memory[location_l2][p] = data[p] ;
                    }
                }

                if(location_l2 > 32 || location_l2 <1)
                {
                    printf("frame L1 exceeded\n") ;
                    break ;
                }

                memory[page_table_1][4*l1_offset+0] = location_l2 ;  // updates entry in L1
                memory[page_table_1][4*l1_offset+2] = 0x9 ;          // 1001 both recently used and valid bit are on
            }
            else
            {
                location_l2 = frame_number_L1 ;
                memory[page_table_1][4*l1_offset+2] = memory[page_table_1][4*l1_offset+2] | 0x9 ; // 1001 both recently used and valid bit are on
            }

            // // ENTERING INTO PAGE TABLE 2

            unsigned int valid_bit_L2 = !!(memory[location_l2][4*l2_offset+2] & valid_bit_mask) ;
            unsigned int dirty_bit_L2 = !!(memory[location_l2][4*l2_offset+2] & dirty_bit_mask) ;
            unsigned char bit_counter_L2 = (memory[location_l2][4*l2_offset+2] & bit_counter_mask) ;
            unsigned char frame_number_L2 = memory[location_l2][4*l2_offset+0] ; // tells us which in which frame L2 exists

            unsigned char value_at_memory ;
            unsigned char frame_to_replace_L2 = 33 ;

            if(valid_bit_L2 == 0)
            {
                inner_hit = 1 ;
                memory[page_table_1][4*l1_offset+2] = memory[page_table_1][4*l1_offset+2] | 0x6 ; // setting L1 dirty  and modified because L2 is being changed due to page fault

                if(current_empty_frame_L2 < 127)
                {
                    frame_to_replace_L2 = current_empty_frame_L2 ;
                    current_empty_frame_L2 ++ ;
                }
                else // enhanced second chance algorithm
                {
                    frame_to_replace_L2 = second_chance_algorithm_L2(location_l2, l1_offset) ;
                }

                if(frame_to_replace_L2 < 33 || frame_to_replace_L2 > 127)
                {
                    printf("frame l2 exceeded\n") ;
                    break ;
                }

                fseek(backing_store,page_number*1024,SEEK_SET);
                unsigned char data[1024]  ;
                fread(data,1,1024,backing_store);

                for(int z=0; z<1024; z++)
                {
                    memory[frame_to_replace_L2][z] = data[z] ;
                }

                memory[location_l2][4*l2_offset+0] = frame_to_replace_L2 ;
                memory[location_l2][4*l2_offset+2] =  0x9 ; //  1001 both recently used and valid bit are on

                value_at_memory = memory[frame_to_replace_L2][page_offset] ;
            }
            else
            {
                value_at_memory = memory[frame_number_L2][page_offset] ;
                memory[location_l2][4*l2_offset+2] = 0x9 ; //  1001 both recently used and valid bit are on
            }

            // we have got our value now performing operation
            unsigned char value_before = value_at_memory ;
            if(opcode == 0x10)
            {
                value_at_memory = value_at_memory + value ;
            }
            else if(opcode == 0x20)
            {
                value_at_memory = value ;
            }
            else if(opcode == 0x30)
            {
                value_at_memory = value_at_memory - value ;
            }
            else if(opcode == 0x40)
            {
                value_at_memory = value_at_memory * value ;
            }
            else if (opcode == 0x50)
            {
                value_at_memory = value_at_memory & value ;
            }
            else if(opcode == 0x60)
            {
                value_at_memory = value_at_memory | value ;
            }
            else if(opcode == 0x70)
            {
                value_at_memory = value_at_memory ^ value ;
            }

            memory[frame_number_L2][page_offset] = value_at_memory ; // updating value at the memory
            memory[location_l2][4*l2_offset+2] = memory[location_l2][4*l2_offset+2] | 0x6 ; // setting the dirty bit and modified bit in L2
            memory[page_table_1][4*l1_offset+2] = memory[page_table_1][4*l1_offset+2] | 0x6 ; // since L2 is chainging therefore L1 will also get dirty

            fprintf(file_pointer, "MV  %d  %d  NULL  NULL  %X   %X  %X,%X,%X\n", outer_hit, inner_hit, value_before, value_at_memory, opcode, address, value) ;

        } // else if the operation is memory - memory XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
        else if(opcode == 0x11 || opcode == 0x21 || opcode == 0x31 || opcode == 0x41 || opcode == 0x51 || opcode == 0x61 || opcode == 0x71)
        {
            unsigned int address1 = data[1] << 16  | data[2]<< 8 | data[3] ;

            unsigned int address2 = data[4] << 16 | data[5] << 8 | data[6] ;

            unsigned char value_at_address_1 ;
            unsigned char value_at_address_2 ;

            unsigned char address1_outer_hit = 0;
            unsigned char address2_outer_hit = 0;

            unsigned char address1_inner_hit = 0;
            unsigned char address2_inner_hit = 0;

            unsigned char address_1_frame_number = 0 ;
            unsigned char address_1_offset = 0 ;

            // RESOLVING ADDRESS 1 XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
            if(1) // to limit the scope of variables
            {
                unsigned int page_offset = extract_page_offset(address1) ;   // extracting page offset
                address_1_offset = page_offset ;
                unsigned int page_number = extract_page_number(address1) ;   // extrating page number global

                unsigned int l1_offset = extract_l1_offset(page_number) ;  // extracting offset in outer page table
                unsigned int l2_offset = extract_l2_offset(page_number) ;  // extracting offset in inner page table

                // 0 pe frame hai and 2 pe baki ke bits hain
                unsigned int valid_bit_L1 = !!(memory[page_table_1][4*l1_offset+2] & valid_bit_mask) ;
                unsigned int dirty_bit_L1 = !!(memory[page_table_1][4*l1_offset+2] & dirty_bit_mask) ;
                unsigned char bit_counter_L1 = (memory[page_table_1][4*l1_offset+2] & bit_counter_mask) ;
                unsigned char frame_number_L1 = memory[page_table_1][4*l1_offset+0] ; // tells us which in which frame L2 exists

                unsigned char location_l2 ;  // frame number to find l2 table
                if(valid_bit_L1 == 0)
                {
                    address1_outer_hit = 1 ;
                    if(current_empty_frame_L1 < 33)
                    {
                        location_l2 = current_empty_frame_L1 ;
                        current_empty_frame_L1 ++ ;
                    }
                    else  // enhanced second chance algorithm
                    {
                        location_l2 = second_chance_algorithm_L1() ;

                        fseek(backing_store,0x00C193E8+8+l1_offset*1024,SEEK_SET);
                        unsigned char data[1024]  ;
                        fread(data,1,1024,backing_store);

                        for(int p=0; p<1024; p++)
                        {
                            memory[location_l2][p] = data[p] ;
                        }
                    }

                    if(location_l2 > 32 || location_l2 <1)
                    {
                        printf("frame L1 exceeded\n") ;
                        break ;
                    }

                    memory[page_table_1][4*l1_offset+0] = location_l2 ;  // updates entry in L1
                    memory[page_table_1][4*l1_offset+2] = 0x9 ;          // 1001 both recently used and valid bit are on
                }
                else
                {
                    location_l2 = frame_number_L1 ;
                    memory[page_table_1][4*l1_offset+2] = memory[page_table_1][4*l1_offset+2] | 0x9 ; // 1001 both recently used and valid bit are on
                }

                // // ENTERING INTO PAGE TABLE 2

                unsigned int valid_bit_L2 = !!(memory[location_l2][4*l2_offset+2] & valid_bit_mask) ;
                unsigned int dirty_bit_L2 = !!(memory[location_l2][4*l2_offset+2] & dirty_bit_mask) ;
                unsigned char bit_counter_L2 = (memory[location_l2][4*l2_offset+2] & bit_counter_mask) ;
                unsigned char frame_number_L2 = memory[location_l2][4*l2_offset+0] ; // tells us which in which frame L2 exists

                unsigned char frame_to_replace_L2 = 33 ;

                if(valid_bit_L2 == 0)
                {
                    address1_inner_hit = 1 ;
                    memory[page_table_1][4*l1_offset+2] = memory[page_table_1][4*l1_offset+2] | 0x6 ; // setting L1 dirty  and modified because L2 is being changed due to page fault

                    if(current_empty_frame_L2 < 127)
                    {
                        frame_to_replace_L2 = current_empty_frame_L2 ;
                        current_empty_frame_L2 ++ ;
                    }
                    else // enhanced second chance algorithm
                    {
                        frame_to_replace_L2 = second_chance_algorithm_L2(location_l2, l1_offset) ;
                    }

                    if(frame_to_replace_L2 < 33 || frame_to_replace_L2 > 127)
                    {
                        printf("frame l2 exceeded\n") ;
                        break ;
                    }

                    fseek(backing_store,page_number*1024,SEEK_SET);
                    unsigned char data[1024]  ;
                    fread(data,1,1024,backing_store);

                    for(int z=0; z<1024; z++)
                    {
                        memory[frame_to_replace_L2][z] = data[z] ;
                    }

                    memory[location_l2][4*l2_offset+0] = frame_to_replace_L2 ;
                    memory[location_l2][4*l2_offset+2] =  0x9 ; //  1001 both recently used and valid bit are on

                    value_at_address_1 = memory[frame_to_replace_L2][page_offset] ;
                    address_1_frame_number = frame_to_replace_L2 ;
                }
                else
                {
                    address_1_frame_number =  frame_number_L2 ;
                    value_at_address_1 = memory[frame_number_L2][page_offset] ;
                    memory[location_l2][4*l2_offset+2] = 0x9 ; //  1001 both recently used and valid bit are on
                }
            }

            // RESOLVING ADDRESS 2 ----------------------------------------------------------------------------------------------------------
            unsigned int page_offset = extract_page_offset(address2) ;   // extracting page offset
            unsigned int page_number = extract_page_number(address2) ;   // extrating page number global

            unsigned int l1_offset = extract_l1_offset(page_number) ;  // extracting offset in outer page table
            unsigned int l2_offset = extract_l2_offset(page_number) ;  // extracting offset in inner page table

            // 0 pe frame hai and 2 pe baki ke bits hain
            unsigned int valid_bit_L1 = !!(memory[page_table_1][4*l1_offset+2] & valid_bit_mask) ;
            unsigned int dirty_bit_L1 = !!(memory[page_table_1][4*l1_offset+2] & dirty_bit_mask) ;
            unsigned char bit_counter_L1 = (memory[page_table_1][4*l1_offset+2] & bit_counter_mask) ;
            unsigned char frame_number_L1 = memory[page_table_1][4*l1_offset+0] ; // tells us which in which frame L2 exists

            unsigned char location_l2 ;  // frame number to find l2 table
            if(valid_bit_L1 == 0)
            {
                address2_outer_hit = 1 ;
                if(current_empty_frame_L1 < 33)
                {
                    location_l2 = current_empty_frame_L1 ;
                    current_empty_frame_L1 ++ ;
                }
                else  // enhanced second chance algorithm
                {
                    location_l2 = second_chance_algorithm_L1() ;

                    fseek(backing_store,0x00C193E8+8+l1_offset*1024,SEEK_SET);
                    unsigned char data[1024]  ;
                    fread(data,1,1024,backing_store);

                    for(int p=0; p<1024; p++)
                    {
                        memory[location_l2][p] = data[p] ;
                    }
                }

                if(location_l2 > 32 || location_l2 <1)
                {
                    printf("frame L1 exceeded\n") ;
                    break ;
                }

                memory[page_table_1][4*l1_offset+0] = location_l2 ;  // updates entry in L1
                memory[page_table_1][4*l1_offset+2] = 0x9 ;          // 1001 both recently used and valid bit are on
            }
            else
            {
                location_l2 = frame_number_L1 ;
                memory[page_table_1][4*l1_offset+2] = memory[page_table_1][4*l1_offset+2] | 0x9 ; // 1001 both recently used and valid bit are on
            }

            // // ENTERING INTO PAGE TABLE 2

            unsigned int valid_bit_L2 = !!(memory[location_l2][4*l2_offset+2] & valid_bit_mask) ;
            unsigned int dirty_bit_L2 = !!(memory[location_l2][4*l2_offset+2] & dirty_bit_mask) ;
            unsigned char bit_counter_L2 = (memory[location_l2][4*l2_offset+2] & bit_counter_mask) ;
            unsigned char frame_number_L2 = memory[location_l2][4*l2_offset+0] ; // tells us which in which frame L2 exists

            unsigned char frame_to_replace_L2 = 33 ;

            if(valid_bit_L2 == 0)
            {
                address2_inner_hit = 1 ;
                memory[page_table_1][4*l1_offset+2] = memory[page_table_1][4*l1_offset+2] | 0x6 ; // setting L1 dirty  and modified because L2 is being changed due to page fault

                if(current_empty_frame_L2 < 127)
                {
                    frame_to_replace_L2 = current_empty_frame_L2 ;
                    current_empty_frame_L2 ++ ;
                }
                else // enhanced second chance algorithm
                {
                    frame_to_replace_L2 = second_chance_algorithm_L2(location_l2, l1_offset) ;
                }

                if(frame_to_replace_L2 < 33 || frame_to_replace_L2 > 127)
                {
                    printf("frame l2 exceeded\n") ;
                    break ;
                }

                fseek(backing_store,page_number*1024,SEEK_SET);
                unsigned char data[1024]  ;
                fread(data,1,1024,backing_store);

                for(int z=0; z<1024; z++)
                {
                    memory[frame_to_replace_L2][z] = data[z] ;
                }

                memory[location_l2][4*l2_offset+0] = frame_to_replace_L2 ;
                memory[location_l2][4*l2_offset+2] =  0x9 ; //  1001 both recently used and valid bit are on

                value_at_address_2 = memory[frame_to_replace_L2][page_offset] ;
            }
            else
            {
                value_at_address_2 = memory[frame_number_L2][page_offset] ;
                memory[location_l2][4*l2_offset+2] = 0x9 ; //  1001 both recently used and valid bit are on
            }

            unsigned char value_before_at_address_1 = value_at_address_1 ;
            if(opcode == 0x11)
            {
                value_at_address_1 = value_at_address_1 + value_at_address_2 ;
            }
            else if(opcode == 0x21)
            {
                value_at_address_1 = value_at_address_2 ;
            }
            else if(opcode == 0x31)
            {
                value_at_address_1 = value_at_address_1 - value_at_address_2 ;
            }
            else if(opcode == 0x41)
            {
                value_at_address_1 = value_at_address_1 * value_at_address_2 ;
            }
            else if (opcode == 0x51)
            {
                value_at_address_1 = value_at_address_1 & value_at_address_2 ;
            }
            else if(opcode == 0x61)
            {
                value_at_address_1 = value_at_address_1 | value_at_address_2 ;
            }
            else if(opcode == 0x71)
            {
                value_at_address_1 = value_at_address_1 ^ value_at_address_2 ;
            }

            fprintf(file_pointer, "MM  %X  %X  %X  %X  %X   %X  %X,%X,%X\n", address1_outer_hit, address1_inner_hit, address2_outer_hit, address2_inner_hit, value_before_at_address_1, value_at_address_1, opcode, address1, address2) ;
            memory[address_1_frame_number][address_1_offset] = value_at_address_1 ;
        }

        j++ ;
        //printf("%d\n", j) ;
    }
    //printf("%d\n", j) ;

    // WRITING BACK THE DIRTY PAGES BACK TO BACKING STORE
    for(int L1=0; L1<64; L1++)
    {
        if( (!!(memory[page_table_1][4*L1+2] & valid_bit_mask) == 1) && (!!(memory[page_table_1][4*L1+2] & dirty_bit_mask) == 1) )
        {
            unsigned char L2_frame = memory[page_table_1][4*L1+0] ;

            for(int L2=0; L2<256; L2++)
            {
                if( (!!(memory[L2_frame][4*L2+2] & valid_bit_mask) == 1) && (!!(memory[L2_frame][4*L2+2] & dirty_bit_mask) == 1) )
                {
                    unsigned char actual_frame = memory[L2_frame][4*L2+0] ;
                    unsigned int pg_number = L1 << 8 | L2 ;

                    fseek(backing_store,pg_number*1024,SEEK_SET);
                    unsigned char data[1024]  ;

                    for(int p=0; p<1024; p++)
                    {
                        data[p] = memory[actual_frame][p];
                    }
                    fwrite(data,1,1024,backing_store);
                }
            }
        }
    }

    fclose(backing_store) ;
    printf("Program successfully executed\n") ;
}
