/* Copyright (c) 2005 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Neither the name of the Advanced Micro Devices, Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 * */

/*
 * File Contents:   This file contains platform dependent functions
 *                  which provide interface to that platform.
 * 
 * SubModule:       Geode FlatPanel library
 * */

#define LINUX_ROM_SEGMENT 0x000F
#define SEGMENT_LENGTH  0xFFFF
#define PAGE_LENGTH     0x1000
#define SYS_BOARD_NAME_LEN 24

#define PLT_UNKNOWN     0xFFFF

typedef struct {
    char sys_board_name[SYS_BOARD_NAME_LEN];
    SYS_BOARD sys_board;
} SYS_BOARD_INFO;

static SYS_BOARD_INFO Sys_info;

/*
 * The names in the sys_board_name string must exactly match the names in the
 * BIOS header. These names are used by FindStringInSeg() to find the names 
 * in the BIOS header space. The BIOS does not use OTHER; it is a dummy value 
 * for program useonly. 
 */

SYS_BOARD_INFO Sys_board_info_array[] = {
    {"Marmot", MARMOT_PLATFORM},
    {"Unicorn", UNICORN_PLATFORM},
    {"Centaurus", CENTAURUS_PLATFORM},
    {"Aries", ARIES_PLATFORM},
    {"Carmel", CARMEL_PLATFORM},
    {"Hyrda", HYDRA_PLATFORM},
    {"Dorado", DORADO_PLATFORM},
    {"Redcloud", REDCLOUD_PLATFORM},
    {"Other", OTHER_PLATFORM}
};

#define NUM_SYS_BOARD_TYPES		\
		sizeof(Sys_board_info_array)/sizeof(SYS_BOARD_INFO)

static int Num_sys_board_type = NUM_SYS_BOARD_TYPES;
SYS_BOARD_INFO *Sys_board_array_base = Sys_board_info_array;
int FindStringInSeg(unsigned int, const char *);
static unsigned char get_sys_board_type(SYS_BOARD_INFO *, SYS_BOARD_INFO *);

/* Detect the Platform */
int
Detect_Platform(void)
{
    /* See if we can find the board name using Xpressrom */
    get_sys_board_type(&Sys_info, Sys_board_array_base);
    return (Sys_info.sys_board);
}

static int
Strncmp(const char *str1, char *str2, int len)
{
    int i;

    if ((str1 == 0x0) || (str2 == 0x0) || (len == 0))
        return (1);
    for (i = 0; i < len; i++) {
        if (*(str1 + i) > *(str2 + i)) {
            return 1;
        }
        else if (*(str1 + i) < *(str2 + i)) {
            return -1;
        }
    }
    return 0;
}

static char *
Strcpy(char *dst, const char *src)
{
    int i;

    if ((dst == 0x0) || (src == 0x0))
        return (0);
    for (i = 0; src[i] != 0x0; i++) {
        dst[i] = src[i];
    }
    dst[i] = 0x0;               /* NULL termination */
    return dst;
}

static int
Strlen(const char *str)
{
    int i;

    if (str == 0x0)
        return 0;
    for (i = 0; str[i] != 0x0; i++);
    return i;
}

/* Platform Detection Code */

/************************************************************************
 * int FindStringInSeg( unsigned int segment_address, char *string_ptr )
 *
 * Returns the offset where the NULL terminated string pointed to by 
 * string_ptr is located in the segment passed in segment_address.
 * Segment_address must be of the form 0xXXXX (i.e 0xf000 for segment f).
 * Returns NULL if the string is not found.
 ************************************************************************
 */
int
FindStringInSeg(unsigned int segment_address, const char *string_ptr)
{
    int string_length = Strlen(string_ptr);
    char *psegment_buf;
    unsigned long mem_ptr = (unsigned long) segment_address << 16;
    unsigned int i;

    /* silence compiler */
    (void) mem_ptr;

    psegment_buf = (char *) XpressROMPtr;

    /* Now search for the first character of the string_ptr */
    for (i = 0; i < SEGMENT_LENGTH + 1; i++) {
        if (*(psegment_buf + i) == *string_ptr) {

            /* If we match the first character, do a
             * string compare.
             */

            if (!Strncmp(string_ptr, (psegment_buf + i), string_length)) {
                /* They match! */
                return (1);
            }
        }
    }
    /* if we got this far we didn't find anything.  Return NULL. */
    return (0);

}                               /* end FindStringInSeg() */

/**********************************************************************

 * TRUE_FALSE get_sys_board_type( SYS_BOARD_INFO *sys_info,
 * SYS_BOARD_INFO *sys_board_array_base)
 *
 * Checks the system BIOS area for Xpressrom information. If found, searches 
 * the  BIOS area for one of names in the array pointed to by 
 * sys_board_array_ptr.
 * If a match is found, sets the SYS_INFO system_board_name string 
 * and the system_board variable to the board name and returns TRUE. 
 * If Xpressrom or a board is not found, sets the variables to 
 * their default values and returns FALSE.
 * Uses the global string pointer *xpress_rom_string_ptr.
 ***********************************************************************
 */
static unsigned char
get_sys_board_type(SYS_BOARD_INFO *sys_info,
                   SYS_BOARD_INFO *sys_board_array_base)
{
    int index;
    const char *xpress_rom_string_ptr = "XpressStart";
    unsigned int segment = LINUX_ROM_SEGMENT;

    /* See if XpressStart is present in the BIOS area. 
     * If it is, search for a board string.  If not, Xpressrom is 
     * not present, set system_board information to UNKNOWN and 
     * return FALSE. 
     */

    if (!FindStringInSeg(segment, xpress_rom_string_ptr)) {
        sys_info->sys_board = PLT_UNKNOWN;
        Strcpy(sys_info->sys_board_name, "Unknown");
        return (FALSE);
    }
    else {

        /* we have Xpressrom, so look for a board */
        for (index = 0; index < Num_sys_board_type; index++) {
            if (!FindStringInSeg(segment, (sys_board_array_base +
                                           index)->sys_board_name)) {
                continue;
            }
            else {

                /* a match!! */
                sys_info->sys_board = (sys_board_array_base + index)->sys_board;
                Strcpy(sys_info->sys_board_name,
                       (sys_board_array_base + index)->sys_board_name);
                return (TRUE);
            }
        }                       /* end for() */
    }                           /* end else */

    /* if we are here we have failed */
    sys_info->sys_board = PLT_UNKNOWN;
    Strcpy(sys_info->sys_board_name, "Unknown");
    return (FALSE);
}                               /* end get_sys_board_type() */
