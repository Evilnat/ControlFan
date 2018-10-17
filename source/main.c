/* 
    (c) 2013 Estwald <www.elotrolado.net>

    "ControlFan" is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    "ControlFan" is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with "ControlFan". If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <math.h>

#include <ppu-lv2.h>

#include <io/pad.h>

#include <tiny3d.h>
#include <libfont.h>

#include <sys/memory.h>
#include <sys/process.h>
#include <sys/file.h>

#include <lv2_syscall.h>

#define FS_S_IFMT 0170000
#define FS_S_IFDIR 0040000

// font 0: 224 chr from 32 to 255, 16 x 32 pix 2 bit depth
#include "font.h"


#include "pad.h"

#include "ps3_controlfan_bin.h"

char * LoadFile(char *path, int *file_size);
int SaveFile(char *path, char *mem, int file_size);
char temp_buffer[2048];

#define LV2_SM_CMD_ADDR 0x8000000000000450ULL

#define WITH_LEDS 2
#define WITHOUT_LEDS 1

#undef AUTO_BUTTON_REP2
#define AUTO_BUTTON_REP2(v, b) if(v && (old_pad & b)) { \
                                 v++; \
                                 if(v > 10) {v = 0; new_pad |= b;} \
                                 } else v = 0;


u64 lv2peek(u64 addr) 
{ 
    lv2syscall1(6, (u64) addr);
    return_to_user_prog(u64);

}

u64 lv2poke(u64 addr, u64 value) 
{ 
    lv2syscall2(7, (u64) addr, (u64) value); 
    return_to_user_prog(u64);
}

u32 lv2peek32(u64 addr) {
    u32 ret = (u32) (lv2peek(addr) >> 32ULL);
    return ret;
}

u64 lv2poke32(u64 addr, u32 value) 
{ 
    return lv2poke(addr, (((u64) value) <<32) | (lv2peek(addr) & 0xffffffffULL));
}

int sys_game_get_temperature(int sel, u32 *temperature) 
{
    u32 temp;
  
    lv2syscall2(383, (u64) sel, (u64) &temp); 
    *temperature = (temp >> 24);
    return_to_user_prog(int);
}

int sys_sm_set_fan_policy(u8 arg0, u8 arg1, u8 arg2)
{
    lv2syscall3(389, (u64) arg0, (u64) arg1, (u64) arg2); 
    return_to_user_prog(int);
}

int sys_sm_get_fan_policy(u8 id, u8 *st, u8 *mode, u8 *speed, u8 *unknown)
{
    lv2syscall5(409, (u64) id, (u64) st, (u64) mode, (u64) speed, (u64) unknown); 
    return_to_user_prog(int);
}


int sys_set_leds(u64 color, u64 state) 
{
    lv2syscall2(386,  (u64) color, (u64) state);
    return_to_user_prog(int);
}

#define PATCH_JUMP(add_orig, add_dest) lv2poke32(0x8000000000000000ULL + (u64) (add_orig), (0x48000000 | ((add_dest-add_orig) & 0x3fffffc)))
static u64 PAYLOAD_BASE = 0x8000000000000f70ULL;

// CFW 4.40
u32 cfw = 0x440C;
u64 syscall_base = 0x800000000035E260ULL;


int load_ps3_controlfan_payload()
{
    int ret = 0;

    u64 *addr= (u64 *) memalign(8, ps3_controlfan_bin_size + 31);
  
    if(cfw == 0x341C)         
        syscall_base = 0x80000000002EB128ULL;
     else if(cfw == 0x355C)         
        syscall_base = 0x8000000000346570ULL;
     else if(cfw == 0x355D)         
        syscall_base = 0x8000000000361578ULL;
     else if(cfw == 0x421C)         
        syscall_base = 0x800000000035BCA8ULL;
     else if(cfw == 0x421D)         
        syscall_base = 0x800000000037A1B0ULL;
     else if(cfw == 0x430C)         
        syscall_base = 0x800000000035DBE0ULL;
     else if(cfw == 0x430D)     
        syscall_base = 0x800000000037C068ULL;    
     else if(cfw == 0x431C)         
        syscall_base = 0x800000000035DBE0ULL;
     else if(cfw == 0x440C)     
        syscall_base = 0x800000000035E260ULL;    
     else if(cfw == 0x441C)     
        syscall_base = 0x800000000035E260ULL;    
     else if(cfw == 0x441D)     
        syscall_base = 0x800000000037C9E8ULL;    
     else if(cfw == 0x446C)     
        syscall_base = 0x800000000035E860ULL;    
     else if(cfw == 0x446D)     
        syscall_base = 0x800000000037CFE8ULL;    
     else if(cfw == 0x450C)     
        syscall_base = 0x800000000035F0D0ULL;    
     else if(cfw == 0x450D)     
        syscall_base = 0x8000000000383658ULL;    
     else if(cfw == 0x453C)     
        syscall_base = 0x800000000035F300ULL;    
     else if(cfw == 0x453D)     
        syscall_base = 0x8000000000385108ULL;    
     else if(cfw == 0x455C)     
        syscall_base = 0x8000000000362680ULL;    
     else if(cfw == 0x455D)     
        syscall_base = 0x8000000000388488ULL;    
     else if(cfw == 0x460C)     
        syscall_base = 0x8000000000363A18ULL;    
     else if(cfw == 0x465C)     
        syscall_base = 0x8000000000363A18ULL;    
     else if(cfw == 0x465D)     
        syscall_base = 0x800000000038A120ULL;    
     else if(cfw == 0x466C)     
        syscall_base = 0x8000000000363A18ULL;    
     else if(cfw == 0x470C)     
        syscall_base = 0x8000000000363B60ULL;    
     else if(cfw == 0x470D)     
        syscall_base = 0x800000000038A368ULL;    
     else if(cfw == 0x475C)     
        syscall_base = 0x8000000000363BE0ULL;    
     else if(cfw == 0x475D)     
        syscall_base = 0x800000000038A3E8ULL;    
     else if(cfw == 0x476C)     
        syscall_base = 0x8000000000363BE0ULL;    
     else if(cfw == 0x476D)     
        syscall_base = 0x800000000038A3E8ULL;    
     else if(cfw == 0x478C)     
        syscall_base = 0x8000000000363BE0ULL;    
     else if(cfw == 0x478D)     
        syscall_base = 0x800000000038A3E8ULL;    
     else if(cfw == 0x480C)     
        syscall_base = 0x8000000000363BE0ULL;    
     else if(cfw == 0x480D)     
        syscall_base = 0x800000000038A4E8ULL;    
     else if(cfw == 0x481C)     
        syscall_base = 0x8000000000363BE0ULL;    
     else if(cfw == 0x481D)     
        syscall_base = 0x800000000038A4E8ULL;    
     else if(cfw == 0x481C)     
        syscall_base = 0x8000000000363BE0ULL;    
     else if(cfw == 0x482D)     
        syscall_base = 0x800000000038A4E8ULL;    
     else if(cfw == 0x482C)     
        syscall_base = 0x8000000000363BE0ULL; 
     else if(cfw == 0x483C)     
        syscall_base = 0x8000000000363BE0ULL;   
     else exit(0);

    if(!addr) {

        exit(0);
    }

    if(!syscall_base)        
        exit(0);    
    
    if(lv2peek(PAYLOAD_BASE)) goto skip_the_load;
    
    memcpy((char *) addr, (char *) ps3_controlfan_bin, ps3_controlfan_bin_size);

    addr[1] = syscall_base;

    addr[2] += PAYLOAD_BASE;
    addr[3] = lv2peek(syscall_base + (u64) (130 * 8));
    addr[4] += PAYLOAD_BASE;
    addr[5] = lv2peek(syscall_base + (u64) (138 * 8));

    addr[6] += PAYLOAD_BASE;
    addr[7] = lv2peek(syscall_base + (u64) (379 * 8));

    int n;

    for(n = 0; n < 200; n++) 
    {
        int m;

        for(m = 0; m < ((ps3_controlfan_bin_size + 7) & ~7); m+=8)
            lv2poke(PAYLOAD_BASE + (u64) m, addr[m>>3]);

        lv2poke(syscall_base + (u64) (130 * 8), PAYLOAD_BASE + 0x10ULL);
        lv2poke(syscall_base + (u64) (138 * 8), PAYLOAD_BASE + 0x20ULL);

        lv2poke(syscall_base + (u64) (379 * 8), PAYLOAD_BASE + 0x30ULL);

        usleep(10000);
    }

    sleep(1);

    // Firmware 3.41
    if(cfw == 0x341C) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000AEA0ULL, 0x38600000); // sys 383 *

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000008644ULL, 0x38600001); // sys 409 *

        // enables sys_sm_set_fan_policy
        lv2poke32(0x8000000000008B40ULL, 0x38600001); // sys 389 *

        // enables sys_set_leds
        lv2poke32(0x8000000000008C08ULL, 0x38600001); // sys 386 *
        
        ret = 1;
    } 

    // Firmware 3.55
    else if(cfw == 0x355C) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000B518ULL, 0x38600000); // sys 383 *

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000008CBCULL, 0x38600001); // sys 409 *

        // enables sys_sm_set_fan_policy
        lv2poke32(0x80000000000091B8ULL, 0x38600001); // sys 389 *

        // enables sys_set_leds
        lv2poke32(0x8000000000009280ULL, 0x38600001); // sys 386 *
        
        ret = 1;
    } 

    // Firmware 3.55 DEX
    else if(cfw == 0x355D) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000B598ULL, 0x38600000); // sys 383 *

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000008D3CULL, 0x38600001); // sys 409 *

        // enables sys_sm_set_fan_policy
        lv2poke32(0x8000000000009238ULL, 0x38600001); // sys 389 *

        // enables sys_set_leds
        lv2poke32(0x8000000000009300ULL, 0x38600001); // sys 386 *
        
        ret = 1;
    } 

    // Firmware 4.21
    else if(cfw == 0x421C) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C698ULL, 0x38600000); // sys 383 *

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009E28ULL, 0x38600001); // sys 409 *

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A324ULL, 0x38600001); // sys 389 *

        // enables sys_set_leds
        lv2poke32(0x800000000000A3ECULL, 0x38600001); // sys 386 *
        
        ret = 1;
    } 

    // Firmware 4.21 DEX
    else if(cfw == 0x421D) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C718ULL, 0x38600000); // sys 383 *

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009EA8ULL, 0x38600001); // sys 409 *

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A3A4ULL, 0x38600001); // sys 389 *

        // enables sys_set_leds
        lv2poke32(0x800000000000A46CULL, 0x38600001); // sys 386 *
        
        ret = 1;
    } 

    // Firmware 4.30
    else if(cfw == 0x430C) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C694ULL, 0x38600000); // sys 383 *

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009E28ULL, 0x38600001); // sys 409 *

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A324ULL, 0x38600001); // sys 389 *

        // enables sys_set_leds
        lv2poke32(0x800000000000A3ECULL, 0x38600001); // sys 386 *

        ret = 1;

    } 

    // Firmware 4.30 DEX
    else if(cfw == 0x430D) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C714ULL, 0x38600000); // sys 383 *

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009EA8ULL, 0x38600001); // sys 409 *

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A3A4ULL, 0x38600001); // sys 389 *

        // enables sys_set_leds
        lv2poke32(0x800000000000A46CULL, 0x38600001); // sys 386 *

        ret = 1;

    } 

    // Firmware 4.31
    else if(cfw == 0x431C) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C698ULL, 0x38600000); // sys 383

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009E28ULL, 0x38600001); // sys 409

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A324ULL, 0x38600001); // sys 389

        // enables sys_set_leds
        lv2poke32(0x800000000000A3ECULL, 0x38600001); // sys 386

        ret = 1;

    } 

    // Firmware 4.40
    else if(cfw == 0x440C) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C694ULL, 0x38600000); // sys 383

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009E28ULL, 0x38600001); // sys 409

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A324ULL, 0x38600001); // sys 389

        // enables sys_set_leds
        lv2poke32(0x800000000000A3ECULL, 0x38600001); // sys 386
        
        ret = 1;
    } 

    // Firmware 4.41
    else if(cfw == 0x441C) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C698ULL, 0x38600000); // sys 383

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009E28ULL, 0x38600001); // sys 409

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A324ULL, 0x38600001); // sys 389

        // enables sys_set_leds
        lv2poke32(0x800000000000A3ECULL, 0x38600001); // sys 386
        
        ret = 1;
    }

    // Firmware 4.41 DEX
    else if(cfw == 0x441D) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C718ULL, 0x38600000); // sys 383

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009EA8ULL, 0x38600001); // sys 409

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A3A4ULL, 0x38600001); // sys 389

        // enables sys_set_leds
        lv2poke32(0x800000000000A46CULL, 0x38600001); // sys 386
        
        ret = 1;
    }

    // Firmware 4.46
    else if(cfw == 0x446C) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C698ULL, 0x38600000); // sys 383

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009E28ULL, 0x38600001); // sys 409

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A324ULL, 0x38600001); // sys 389

        // enables sys_set_leds
        lv2poke32(0x800000000000A3ECULL, 0x38600001); // sys 386
        
        ret = 1;
    }

    // Firmware 4.46 DEX
    else if(cfw == 0x446D) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C718ULL, 0x38600000); // sys 383

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009EA8ULL, 0x38600001); // sys 409

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A3A4ULL, 0x38600001); // sys 389

        // enables sys_set_leds
        lv2poke32(0x800000000000A46CULL, 0x38600001); // sys 386
        
        ret = 1;
    }

    // Firmware 4.50
    else if(cfw == 0x450C) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C694ULL, 0x38600000); // sys 383

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009E28ULL, 0x38600001); // sys 409

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A324ULL, 0x38600001); // sys 389

        // enables sys_set_leds
        lv2poke32(0x800000000000A3ECULL, 0x38600001); // sys 386
        
        ret = 1;
    }

    // Firmware 4.50 DEX
    else if(cfw == 0x450D) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C714ULL, 0x38600000); // sys 383

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009EA8ULL, 0x38600001); // sys 409

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A3A4ULL, 0x38600001); // sys 389

        // enables sys_set_leds
        lv2poke32(0x800000000000A46CULL, 0x38600001); // sys 386
        
        ret = 1;
    }

    // Firmware 4.53
    else if(cfw == 0x453C) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C698ULL, 0x38600000); // sys 383

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009E28ULL, 0x38600001); // sys 409

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A324ULL, 0x38600001); // sys 389

        // enables sys_set_leds
        lv2poke32(0x800000000000A3ECULL, 0x38600001); // sys 386
        
        ret = 1;
    }

    // Firmware 4.53 DEX
    else if(cfw == 0x453D) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C718ULL, 0x38600000); // sys 383

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009EA8ULL, 0x38600001); // sys 409

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A3A4ULL, 0x38600001); // sys 389

        // enables sys_set_leds
        lv2poke32(0x800000000000A46CULL, 0x38600001); // sys 386
        
        ret = 1;
    }

    // Firmware 4.55
    else if(cfw == 0x455C) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C6A8ULL, 0x38600000); // sys 383

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009E38ULL, 0x38600001); // sys 409

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A334ULL, 0x38600001); // sys 389

        // enables sys_set_leds
        lv2poke32(0x800000000000A3FCULL, 0x38600001); // sys 386
        
        ret = 1;
    }

    // Firmware 4.55 DEX
    else if(cfw == 0x455D) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C728ULL, 0x38600000); // sys 383

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009EB8ULL, 0x38600001); // sys 409

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A3B4ULL, 0x38600001); // sys 389

        // enables sys_set_leds
        lv2poke32(0x800000000000A47CULL, 0x38600001); // sys 386
        
        ret = 1;
    }

    // Firmware 4.60
    else if(cfw == 0x460C) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C6A4ULL, 0x38600000); // sys 383

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009E38ULL, 0x38600001); // sys 409

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A334ULL, 0x38600001); // sys 389

        // enables sys_set_leds
        lv2poke32(0x800000000000A3FCULL, 0x38600001); // sys 386
        
        ret = 1;
    }

    // Firmware 4.65
    else if(cfw == 0x465C) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C6A8ULL, 0x38600000); // sys 383

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009E38ULL, 0x38600001); // sys 409

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A334ULL, 0x38600001); // sys 389

        // enables sys_set_leds
        lv2poke32(0x800000000000A3FCULL, 0x38600001); // sys 386
        
        ret = 1;
    }

    // Firmware 4.65 DEX
    else if(cfw == 0x465D) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C728ULL, 0x38600000); // sys 383

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009EB8ULL, 0x38600001); // sys 409

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A3B4ULL, 0x38600001); // sys 389

        // enables sys_set_leds
        lv2poke32(0x800000000000A47CULL, 0x38600001); // sys 386
        
        ret = 1;
    }

    // Firmware 4.66
    else if(cfw == 0x466C) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C6A8ULL, 0x38600000); // sys 383

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009E38ULL, 0x38600001); // sys 409

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A334ULL, 0x38600001); // sys 389

        // enables sys_set_leds
        lv2poke32(0x800000000000A3FCULL, 0x38600001); // sys 386
        
        ret = 1;
    }

    // Firmware 4.66 DEX
    else if(cfw == 0x466D) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C728ULL, 0x38600000); // sys 383

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009EB8ULL, 0x38600001); // sys 409

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A3B4ULL, 0x38600001); // sys 389

        // enables sys_set_leds
        lv2poke32(0x800000000000A47CULL, 0x38600001); // sys 386
        
        ret = 1;
    }

    // Firmware 4.70
    else if(cfw == 0x470C) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C6A4ULL, 0x38600000); // sys 383

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009E38ULL, 0x38600001); // sys 409

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A334ULL, 0x38600001); // sys 389

        // enables sys_set_leds
        lv2poke32(0x800000000000A3FCULL, 0x38600001); // sys 386
        
        ret = 1;
    }

    // Firmware 4.70 DEX
    else if(cfw == 0x470D) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C724ULL, 0x38600000); // sys 383

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009EB8ULL, 0x38600001); // sys 409

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A3B4ULL, 0x38600001); // sys 389

        // enables sys_set_leds
        lv2poke32(0x800000000000A47CULL, 0x38600001); // sys 386
        
        ret = 1;
    }

    // Firmware 4.75
    else if(cfw == 0x475C) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C6A8ULL, 0x38600000); // sys 383

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009E38ULL, 0x38600001); // sys 409

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A334ULL, 0x38600001); // sys 389

        // enables sys_set_leds
        lv2poke32(0x800000000000A3FCULL, 0x38600001); // sys 386
        
        ret = 1;
    }

    // Firmware 4.75 DEX
    else if(cfw == 0x475D) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C728ULL, 0x38600000); // sys 383

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009EB8ULL, 0x38600001); // sys 409

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A3B4ULL, 0x38600001); // sys 389

        // enables sys_set_leds
        lv2poke32(0x800000000000A47CULL, 0x38600001); // sys 386
        
        ret = 1;
    }

    // Firmware 4.76
    else if(cfw == 0x476C) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C6A8ULL, 0x38600000); // sys 383

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009E38ULL, 0x38600001); // sys 409

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A334ULL, 0x38600001); // sys 389

        // enables sys_set_leds
        lv2poke32(0x800000000000A3FCULL, 0x38600001); // sys 386
        
        ret = 1;
    }

    // Firmware 4.76 DEX
    else if(cfw == 0x476D) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C728ULL, 0x38600000); // sys 383

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009EB8ULL, 0x38600001); // sys 409

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A3B4ULL, 0x38600001); // sys 389

        // enables sys_set_leds
        lv2poke32(0x800000000000A47CULL, 0x38600001); // sys 386
        
        ret = 1;
    }

    // Firmware 4.78
    else if(cfw == 0x478C) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C6A8ULL, 0x38600000); // sys 383

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009E38ULL, 0x38600001); // sys 409

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A334ULL, 0x38600001); // sys 389

        // enables sys_set_leds
        lv2poke32(0x800000000000A3FCULL, 0x38600001); // sys 386
        
        ret = 1;
    }

    // Firmware 4.78 DEX
    else if(cfw == 0x478D) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C728ULL, 0x38600000); // sys 383

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009EB8ULL, 0x38600001); // sys 409

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A3B4ULL, 0x38600001); // sys 389

        // enables sys_set_leds
        lv2poke32(0x800000000000A47CULL, 0x38600001); // sys 386
        
        ret = 1;
    }

    // Firmware 4.80
    else if(cfw == 0x480C) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C6A4ULL, 0x38600000); // sys 383

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009E38ULL, 0x38600001); // sys 409

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A334ULL, 0x38600001); // sys 389

        // enables sys_set_leds
        lv2poke32(0x800000000000A3FCULL, 0x38600001); // sys 386
        
        ret = 1;
    }

    // Firmware 4.80 DEX
    else if(cfw == 0x480D) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C724ULL, 0x38600000); // sys 383

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009EB8ULL, 0x38600001); // sys 409

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A3B4ULL, 0x38600001); // sys 389

        // enables sys_set_leds
        lv2poke32(0x800000000000A47CULL, 0x38600001); // sys 386
        
        ret = 1;
    }

    // Firmware 4.81
    else if(cfw == 0x481C) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C6A8ULL, 0x38600000); // sys 383

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009E38ULL, 0x38600001); // sys 409

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A334ULL, 0x38600001); // sys 389

        // enables sys_set_leds
        lv2poke32(0x800000000000A3FCULL, 0x38600001); // sys 386
        
        ret = 1;
    }

    // Firmware 4.81 DEX
    else if(cfw == 0x481D) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C728ULL, 0x38600000); // sys 383

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009EB8ULL, 0x38600001); // sys 409

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A3B4ULL, 0x38600001); // sys 389

        // enables sys_set_leds
        lv2poke32(0x800000000000A47CULL, 0x38600001); // sys 386
        
        ret = 1;
    }

    // Firmware 4.82
    else if(cfw == 0x482C) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C6A8ULL, 0x38600000); // sys 383

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009E38ULL, 0x38600001); // sys 409

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A334ULL, 0x38600001); // sys 389

        // enables sys_set_leds
        lv2poke32(0x800000000000A3FCULL, 0x38600001); // sys 386
        
        ret = 1;
    }

    // Firmware 4.82 DEX
    else if(cfw == 0x482D) 
    { 
        // enables sys_game_get_temperature
        lv2poke32(0x800000000000C728ULL, 0x38600000); // sys 383

        // enables sys_sm_get_fan_policy
        lv2poke32(0x8000000000009EB8ULL, 0x38600001); // sys 409

        // enables sys_sm_set_fan_policy
        lv2poke32(0x800000000000A3B4ULL, 0x38600001); // sys 389

        // enables sys_set_leds
        lv2poke32(0x800000000000A47CULL, 0x38600001); // sys 386
        
        ret = 1;
    }
    
skip_the_load:
    free(addr);
    return ret;
}

int is_firm_355dex(void)
{
    u64 addr = lv2peek((0x8000000000361578ULL + 813 * 8));
    // check address first
    if(addr < 0x8000000000000000ULL || addr > 0x80000000007FFFFFULL || (addr & 3)!=0)
        return 0;
    addr = lv2peek(addr);

    if(addr == 0x800000000019be24ULL) return 1;

    return 0;
}

int is_firm_421dex(void)
{
    u64 addr = lv2peek((0x800000000037A1B0ULL + 813 * 8));
    // check address first
    if(addr < 0x8000000000000000ULL || addr > 0x80000000007FFFFFULL || (addr & 3)!=0)
        return 0;
    addr = lv2peek(addr);

    if(addr == 0x80000000001BC9B8ULL) return 1;

    return 0;
}

int is_firm_430dex(void)
{
    // 4.30 dex
    u64 dex2;
    dex2 = lv2peek(0x8000000000365CA0ULL);
    if(dex2 == 0x800000000031A998ULL) 
        return 1;
    else
        return 0;
}


void cls()
{
    tiny3d_Clear(0xff180018, TINY3D_CLEAR_ALL);
        
    // Enable alpha Test
    tiny3d_AlphaTest(1, 0x10, TINY3D_ALPHA_FUNC_GEQUAL);

   // Enable alpha blending.
            tiny3d_BlendFunc(1, TINY3D_BLEND_FUNC_SRC_RGB_SRC_ALPHA | TINY3D_BLEND_FUNC_SRC_ALPHA_SRC_ALPHA,
                TINY3D_BLEND_FUNC_DST_RGB_ONE_MINUS_SRC_ALPHA | TINY3D_BLEND_FUNC_DST_ALPHA_ZERO,
                TINY3D_BLEND_RGB_FUNC_ADD | TINY3D_BLEND_ALPHA_FUNC_ADD);
}

void Init_Graph()
{
    tiny3d_Init(1024*1024);
    tiny3d_Project2D();

    u32 * texture_mem = tiny3d_AllocTexture(64*1024*1024); // alloc 64MB of space for textures (this pointer can be global)    

    u32 * texture_pointer; // use to asign texture space without changes texture_mem

    if(!texture_mem) 
        exit(0); // fail!

    texture_pointer = texture_mem;

    ResetFont();
    texture_pointer = (u32 *) AddFontFromBitmapArray((u8 *) font  , (u8 *) texture_pointer, 32, 255, 16, 32, 2, BIT0_FIRST_PIXEL);


    int videoscale_x = 0;
    int videoscale_y = 0;

    double sx = (double) Video_Resolution.width;
    double sy = (double) Video_Resolution.height;
    double psx = (double) (1000 + videoscale_x)/1000.0;
    double psy = (double) (1000 + videoscale_y)/1000.0;
    
    tiny3d_UserViewport(1, 
        (float) ((sx - sx * psx) / 2.0), // 2D position
        (float) ((sy - sy * psy) / 2.0), 
        (float) ((sx * psx) / 848.0),    // 2D scale
        (float) ((sy * psy) / 512.0),
        (float) ((sx / 1920.0) * psx),  // 3D scale
        (float) ((sy / 1080.0) * psy));

}

char self_path[1024];


char progress[4]={'|', '/', '-', '\\'};


static inline u64 get_ticks(void)
{
    u64 ticks;
    asm volatile("mftb %0" : "=r" (ticks));
    return ticks;
}

int use_sm = 0;


s32 main(s32 argc, const char* argv[])
{
    int n;
    u64 command;

    float x, y;

    strcpy(self_path, "/dev_hdd0/game/CTRLFAN00");

    if(argc>0 && argv) 
    {    
        if(!strncmp(argv[0], "/dev_hdd0/game/", 15)) 
        {
            strcpy(self_path, argv[0]);

            n= 15; while(self_path[n] != '/' && self_path[n] != 0) n++;

            if(self_path[n] == '/') 
                self_path[n] = 0;            
        }
    }

    Init_Graph();

	ioPadInit(7);

    x = y = 0;

    cls();
    SetFontColor(0xffffffff, 0x10000000);
    SetCurrentFont(0);

    SetFontSize(14, 32);
    SetFontAutoCenter(1);

    DrawString(0, 16+24, "Control Fan Utility");
    SetFontAutoCenter(0);

    SetFontColor(0x0fcf2fff, 0x10000000);

    SetCurrentFont(0);
    SetFontSize(10, 16);

    // CFW identify

    u64 uval;
    uval= lv2peek(0x8000000000003000ULL);
    cfw = 0;

    if(is_firm_355dex()) cfw = 0x355D; 
    else if(is_firm_421dex()) cfw = 0x421D; 
    else if(is_firm_430dex()) cfw = 0x430D;
    else if(uval == 0x800000000033E720ULL) cfw = 0x341C;
    else if(uval == 0x8000000000330540ULL) cfw = 0x355C;
    else if(uval == 0x8000000000346390ULL) cfw = 0x421C; 
    else if(uval == 0x8000000000363E80ULL) cfw = 0x421D; 
    else if(uval == 0x8000000000348200ULL) cfw = 0x430C; 
    else if(uval == 0x8000000000365CA0ULL) cfw = 0x430D; 
    else if(uval == 0x8000000000348210ULL) cfw = 0x431C; 
    else if(uval == 0x80000000003487D0ULL) cfw = 0x440C; 
    else if(uval == 0x80000000003487E0ULL) cfw = 0x441C; 
    else if(uval == 0x80000000003665C0ULL) cfw = 0x441D; 
    else if(uval == 0x8000000000348DF0ULL) cfw = 0x446C; 
    else if(uval == 0x8000000000366BD0ULL) cfw = 0x446D; 
    else if(uval == 0x800000000034B160ULL) cfw = 0x450C; 
    else if(uval == 0x800000000036EC40ULL) cfw = 0x450D; 
    else if(uval == 0x800000000034B2E0ULL) cfw = 0x453C; 
    else if(uval == 0x8000000000370620ULL) cfw = 0x453D; 
    else if(uval == 0x800000000034E620ULL) cfw = 0x455C; 
    else if(uval == 0x80000000003738E0ULL) cfw = 0x455D; 
    else if(uval == 0x800000000034F950ULL) cfw = 0x460C; 
    else if(uval == 0x800000000034F960ULL) cfw = 0x465C; 
    else if(uval == 0x8000000000375510ULL) cfw = 0x465D; 
    else if(uval == 0x800000000034F960ULL) cfw = 0x466C; 
    else if(uval == 0x800000000034FB10ULL) cfw = 0x470C; 
    else if(uval == 0x8000000000375850ULL) cfw = 0x470D; 
    else if(uval == 0x800000000034FBB0ULL) cfw = 0x475C; 
    else if(uval == 0x80000000003758E0ULL) cfw = 0x475D; 
    else if(uval == 0x800000000034FBB0ULL) cfw = 0x476C; 
    else if(uval == 0x80000000003758E0ULL) cfw = 0x476D; 
    else if(uval == 0x800000000034FBB0ULL) cfw = 0x478C; 
    else if(uval == 0x80000000003758E0ULL) cfw = 0x478D; 
    else if(uval == 0x800000000034FBA0ULL) cfw = 0x480C; 
    else if(uval == 0x80000000003759B0ULL) cfw = 0x480D; 
    else if(uval == 0x800000000034FBB0ULL) cfw = 0x481C; 
    else if(uval == 0x80000000003759C0ULL) cfw = 0x481D; 
    else if(uval == 0x800000000034FBB0ULL) cfw = 0x482C; 
    else if(uval == 0x80000000003759C0ULL) cfw = 0x482D; 
    else if(uval == 0x800000000034FBB0ULL) cfw = 0x483C;

    if(!cfw) 
    {
         DrawString(24, 56+24, "You need a CFW CEX/DEX 3.41, 3.55, 4.21, 4.30, 4.31, 4.40, 4.41, 4.46, 4.50,\n  4.53, 4.55, 4.60, 4.65, 4.66, 4.70, 4.75, 4.76, 4.78, 4.80, 4.81, 4.82\n  or 4.83 to work");
         tiny3d_Flip();
         sleep(10);
         exit(0);
    }
    
    u64 payload_ctrl;

    if(load_ps3_controlfan_payload()) 
    {
        payload_ctrl = (PAYLOAD_BASE + (lv2peek32(PAYLOAD_BASE + 4ULL ))) - 8ULL;

        lv2poke32(payload_ctrl +  0ULL, 0x33); // current fan speed
        lv2poke32(payload_ctrl +  4ULL, 0); // 0 - disabled, 1 - enabled without leds, 2 - enabled with leds
        usleep(100000);  // waits
        
        lv2poke32(payload_ctrl +  8ULL, 0x5f); // fan speed in shutdown syscall (when it calls PS2 Emulator)
        lv2poke32(payload_ctrl + 12ULL, 0x4d); // fan speed < temp_control0
        lv2poke32(payload_ctrl + 16ULL, 0x54); // fan speed temp_control0 => temp_control1
        lv2poke32(payload_ctrl + 20ULL, 0x60); // fan speed temp_control0 <= temp_control1
        lv2poke32(payload_ctrl + 24ULL, 0x68); // fan speed >= temp_control1
        lv2poke32(payload_ctrl + 28ULL, 0x70); // fan speed >= temp_control2
        lv2poke32(payload_ctrl + 32ULL, 0x78); // fan speed >= temp_control3
        lv2poke32(payload_ctrl + 36ULL, 0xa0); // fan speed >= temp_control4

        lv2poke32(payload_ctrl + 40ULL, 62); // temp_control0 (ºC)
        lv2poke32(payload_ctrl + 44ULL, 68); // temp_control1 (ºC)
        lv2poke32(payload_ctrl + 48ULL, 70); // temp_control2 (ºC)
        lv2poke32(payload_ctrl + 52ULL, 72); // temp_control3 (ºC)
        lv2poke32(payload_ctrl + 56ULL, 75); // temp_control4 (ºC)

        sys_set_leds(2, 0); // restore LEDS
        sys_set_leds(0, 0);
        sys_set_leds(1, 1);

        lv2poke32(payload_ctrl + 4ULL, WITH_LEDS); // enable with leds
        
        command =lv2peek(LV2_SM_CMD_ADDR)>>56ULL;
        if(command == 0x55ULL) 
        {
             // SM present!
            use_sm = 1;
            lv2poke(LV2_SM_CMD_ADDR, 0xAA01000000000000ULL); // enable SM Fan Control Mode 1

            while(1) 
            {
                usleep(1000);
                command =lv2peek(LV2_SM_CMD_ADDR)>>48ULL;
                if(command== 0x55AAULL) break;
            }
        }

        usleep(10000);  // waits
    } 
    else 
        payload_ctrl = (PAYLOAD_BASE + (lv2peek32(PAYLOAD_BASE + 4ULL ))) - 8ULL;


    int cmode = 3;
    static int cspeed = 0x70; // initial speed for mode 2

    int auto_up = 0, auto_down = 0;
    int alive = 0;

    int alarm = 0;

    int cpu_temp = 0, rsx_temp = 0;

    u8 st, mode, speed, unknown;

    sprintf(temp_buffer, "%s/fan_speed.dat", self_path);
    int * val = (int *) LoadFile(temp_buffer, &n);
    if(val)  cspeed = (*val) & 0xff;
    
    n= sys_sm_get_fan_policy(0, &st, &mode, &speed, &unknown);

    if(n==0) 
    {
        cmode = mode;
        if(lv2peek32(payload_ctrl +  4ULL)!=0) cmode = 3;
    }


    // initials get_temperatures can get some extra time
    n= sys_game_get_temperature(0, (void *) &cpu_temp);
    n= sys_game_get_temperature(1, (void *) &rsx_temp);

    while(1) 
    {
        x = y = 0;

        cpu_temp = 0, rsx_temp = 0;

        n= sys_game_get_temperature(0, (void *) &cpu_temp);
        if(n < 0 ) cpu_temp = -1;

        if(cpu_temp > 79) alarm = alarm != 2; else alarm = 0;

        n= sys_game_get_temperature(1, (void *) &rsx_temp);
        if(n < 0 ) rsx_temp = -1;

        if(rsx_temp > 79) alarm = alarm != 2;

        int ret_get_fan = sys_sm_get_fan_policy(0, &st, &mode, &speed, &unknown);

        tiny3d_Flip();
        cls();
        SetFontColor(0xffffffff, 0x10000000);
        SetCurrentFont(0);

        SetFontSize(14, 32);
        SetFontAutoCenter(1);

        DrawString(0, 16+24, "Control Fan Utility");
        SetFontAutoCenter(0);

        SetFontColor(0x0fcf2fff, 0x10000000);

        SetCurrentFont(0);
        SetFontSize(10, 16);

        y+= 16 +24;
        x = 24;

        SetFontColor(0x00c000ff, 0x10000000);
        if(cpu_temp >= 75) SetFontColor(((alive>>4) & 1) ? 0xf02000ff : 0x801000ff, 0x10000000);
        else if(cpu_temp >= 70) SetFontColor(((alive>>4) & 1) ? 0xf0f000ff: 0x808000ff, 0x10000000);

        x = DrawFormatString(x, 56 + y, "CPU temp: %iºC", cpu_temp);

        SetFontColor(0xc0c0c0ff, 0x10000000);
        x = DrawFormatString(x, 56 + y, " / ");

        u32 time_on = (u32) (get_ticks()/0x4c1a6bdULL);

        SetFontColor(0x00c000ff, 0x10000000);
        if(rsx_temp >= 75) SetFontColor(((alive>>4) & 1) ? 0xf02000ff : 0x801000ff, 0x10000000);
        else if(rsx_temp >= 70) SetFontColor(((alive>>4) & 1) ? 0xf0f000ff: 0x808000ff, 0x10000000);

        x = DrawFormatString(x, 56 + y, "RSX temp %iºC", rsx_temp);
        SetFontColor(0xc0c0c0ff, 0x10000000);
        x = DrawFormatString(x, 56 + y, " [%c] PS3 Start Time (HH:MM:SS) %02u:%02u:%02u", 
            progress[(alive>>1) & 3], time_on/3600, (time_on/60) % 60, time_on % 60);
        alive++;

        SetFontColor(0xc0c000ff, 0x10000000);

        x = 24; y += 16;
        
        if(ret_get_fan < 0)  
            DrawFormatString(x, 56 + y, "Error 0x%X in sys_sm_get_fan_policy()", ret_get_fan);
        else
            DrawFormatString(x, 56 + y, "sys_sm_get_fan_policy: (mode: %X speed: 0x%X unknown: %X)", mode, speed, unknown);
        
        x = 24; y += 16*3;


        SetFontColor(0x00c0c0ff, 0x10000000);

        if(cmode==2 && cspeed < 0x4d && !((alive>>4) & 1)) SetFontColor(0x008080ff, 0x10000000);

        if(cmode == 3) x = DrawFormatString(x, 56 + y, "Current mode: #Payload,", cmode); 
        else x = DrawFormatString(x, 56 + y, "Current mode: #%i,", cmode);
        
        if(cmode == 3) DrawFormatString(x, 56 + y, " Speed: 0x%X", lv2peek32(payload_ctrl + 0ULL));
        else if(cmode == 2) DrawFormatString(x, 56 + y, " Speed: 0x%X", cspeed); 
        else DrawFormatString(x, 56 + y, " Speed: --");
        
        SetFontColor(0xc0c0c0ff, 0x10000000);
        x = 24; y += 16*4;
        DrawFormatString(x, 56 + y, "Press SQUARE to change the mode");
        x = 24; y += 16*2;
        DrawFormatString(x, 56 + y, "Press UP/DOWN to change the speed in Mode 2");
        x = 24; y += 16*2;
        DrawFormatString(x, 56 + y, "Press TRIANGLE to exit");
        x = 24; y += 16*2;
        DrawFormatString(x, 56 + y, "Press START to recording speed in Mode 2");
        x = 24; y += 16*3;

        SetFontColor(0x0fcf2fff, 0x10000000);

        DrawFormatString(x, 56 + y, "Modes:");
        x = 24; y += 16*2;
        DrawFormatString(x, 56 + y, "0 => Max Fan Speed");
        x = 24; y += 16;
        DrawFormatString(x, 56 + y, "1 => SYSCON Fan Control");
        x = 24; y += 16;
        DrawFormatString(x, 56 + y, "2 => Manual Fan Control");
        x = 24; y += 16;
        DrawFormatString(x, 56 + y, "Payload => Automatic Alternative Fan Control");
        x = 24; y += 16;

        ps3pad_read();

        if(new_pad & BUTTON_START) 
        {
            sprintf(temp_buffer, "%s/fan_speed.dat", self_path);
            SaveFile(temp_buffer, (void *) &cspeed, 4);
        }

        if(new_pad & BUTTON_TRIANGLE) 
        {
            // if mode 2 and speed < 0x5f, fix 0x5f by security (remember you this is a manual mode)
            if(cmode == 2 && cspeed < 0x5f) {cspeed = 0x5f; sys_sm_set_fan_policy(0, 2, cspeed);}
            break;
        }

        if((new_pad & BUTTON_SQUARE) || alarm==1 || (alarm==2 && cpu_temp < 60 && rsx_temp < 60)) 
        {

            cmode++; if(cmode > 3) cmode = 0;

            if(alarm == 1 || cpu_temp > 79 || rsx_temp > 79) {alarm = 2; cmode = 0;} // alarm mode
            // NOTE (0+1) if for current mode 0 increased in 1 when it enter here
            else if(alarm!=1 && cpu_temp < 60 && rsx_temp < 60 && cmode == (0+1) 
                && (new_pad & BUTTON_SQUARE)==0) {alarm = 0; cmode = 3;}
            else 
                alarm = 0;

            lv2poke32(payload_ctrl + 4ULL, 0); // disabled

            command =lv2peek(LV2_SM_CMD_ADDR)>>56ULL;
            if(command == 0x55ULL && use_sm) 
            {
                // SM present!
                while(1) 
                {
                    lv2poke(LV2_SM_CMD_ADDR, 0xFF00000000000000ULL); // get status
                    usleep(2000);
                    command =lv2peek(LV2_SM_CMD_ADDR);
                    if((command>>48ULL) == 0x55FFULL && (command & 0xF) == 1) break; // SM Fan Control Stopped
                }
            }

            usleep(100000); // waits

            sys_set_leds(2, 0); // restore LEDS
            sys_set_leds(0, 0);
            sys_set_leds(1, 1);

            lv2poke32(payload_ctrl + 0ULL, 0x33); // current fan speed
            lv2poke32(payload_ctrl + 4ULL, cmode==3 ? WITH_LEDS : 0); // enabled/ disabled

            command =lv2peek(LV2_SM_CMD_ADDR)>>56ULL;
            if(command == 0x55ULL && use_sm && cmode == 3) 
            { 
                // SM present!
                while(1) 
                {
                    lv2poke(LV2_SM_CMD_ADDR, 0xAA01000000000000ULL); // enable SM Fan Control Mode 1
                    usleep(1000);
                    command =lv2peek(LV2_SM_CMD_ADDR)>>48ULL;
                    if(command== 0x55AAULL) break;
                }
            }

            usleep(1000); // waits
           
            if(cmode == 0) 
                sys_sm_set_fan_policy(0, 0, 0x0);
            else if(cmode == 1) 
                sys_sm_set_fan_policy(0, 1, 0x0);
            else if(cmode == 2) 
                sys_sm_set_fan_policy(0, 2, cspeed);
  
        }

        AUTO_BUTTON_REP2(auto_up, BUTTON_UP)
        AUTO_BUTTON_REP2(auto_down, BUTTON_DOWN)

        if(cmode == 2) 
        {
            if(new_pad & BUTTON_UP) 
            {
                auto_up = 1; cspeed++; cspeed&= 0xff;
                sys_sm_set_fan_policy(0, 2, cspeed);
            }

            if(new_pad & BUTTON_DOWN) 
            {
                auto_down = 1; cspeed--; cspeed&= 0xff; if(cspeed < 0x30) cspeed = 0x30;
                sys_sm_set_fan_policy(0, 2, cspeed);
            }
        }
    }

      
 
	return 0;
}

char * LoadFile(char *path, int *file_size)
{
    FILE *fp;
    char *mem = NULL;
    
    *file_size = 0;

    sysLv2FsChmod(path, FS_S_IFMT | 0777);

    fp = fopen(path, "rb");

	if (fp != NULL) 
    {        
        fseek(fp, 0, SEEK_END);		
        *file_size = ftell(fp);        
        mem = malloc(*file_size);

		if(!mem) 
        {
            fclose(fp);
            return NULL;
        }       

        fseek(fp, 0, SEEK_SET);
		fread((void *) mem, 1, *file_size, fp);
		fclose(fp);
    }

    return mem;
}

int SaveFile(char *path, char *mem, int file_size)
{
    FILE *fp;
   
    fp = fopen(path, "wb");

	if (fp != NULL) 
    {       
		fwrite((void *) mem, 1, file_size, fp);
		fclose(fp);
    } 
    else
        return -1;

    sysLv2FsChmod(path, FS_S_IFMT | 0777);

    return 0;
}

