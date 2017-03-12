/*
 *
 * Written by Denis Odintsov @ Cisco Systems
 * On Nov-2013
 * There is no Copyright, and I don't accept any
 * You can use this function as you wish in any project for any purposes
 * Information must be free
 *
 */

@ void nv12mt_to_yuv420m_neon(void *y_out, void *cb_out, void *cr_out,
@                             const void *y_in, const void *cbcr_in,
@                             unsigned width, unsigned height)

.text
.align  2
.global nv12mt_to_yuv420m_neon
.type   nv12mt_to_yuv420m_neon, %function

#define Y_ZHGHT   r0
#define UV_ZHGHT  r1
#define HCOUNT    r2
#define WCOUNT    r3
#define BCOUNT    r4
#define COMPENS   r5
#define HEIGHT    r6
#define BACK      r7
#define Y_TO      r8
#define U_TO      r9
#define V_TO      r10
#define Y_FROM    r11
#define UV_FROM   r12

nv12mt_to_yuv420m_neon:
  .fnstart
  push    {r0-r12,lr}                         // save r0-r12,lr in stack. 14 words
  // Initialize incoming parameters
  cpy     Y_TO,     r0                        // mov first called param to Y_TO
  cpy     U_TO,     r1                        // mov second called param to U_TO
  cpy     V_TO,     r2                        // mov third called param to V_TO
  cpy     Y_FROM,   r3                        // mov fourth called param to Y_FROM
  ldr     UV_FROM,  [sp, #14*4]               // copy 15-th stack element to UV of NV12MT
  ldr     HEIGHT,   [sp, #16*4]               // copy 17-th stack element to HEIGHT

  // Initialize variables needed
  cpy     Y_ZHGHT,  HEIGHT                    // Compute the number of full pairs of lines in pixels in NV12MT which can hold Z shapes in Y plane
  add     Y_ZHGHT,  #31
  lsr     Y_ZHGHT,  Y_ZHGHT, #6
  lsl     Y_ZHGHT,  Y_ZHGHT, #6
  mov     UV_ZHGHT, HEIGHT, lsr #1            // Compute the number of full pairs of lines in pixels in NV12MT which can hold Z shapes in UV plane
  add     UV_ZHGHT, #31
  lsr     UV_ZHGHT, UV_ZHGHT, #6
  lsl     UV_ZHGHT, UV_ZHGHT, #6

  ldr     COMPENS,  [sp, #15*4]               // Load WIDTH (stack element 16) to COMPENS
  lsr     COMPENS,  COMPENS, #8               // COMPENS is a shift needed to compensate BACK in the last line of blocks which is not Z shaped
  lsl     COMPENS,  COMPENS, #13              // It equals to number of blocks*2048px BACK will skip in normal odd line (not Z shaped line always be instead of odd)
                                              // ((WIDTH/64)>>2)<<2)*2048

  mov     HCOUNT,   #0                        // HCOUNT counter = 0
  mov     BCOUNT,   #4                        // BCOUNT: block counter = 4
  ldr     WCOUNT,   [sp, #15*4]               // Load WIDTH (stack element 16) to WCOUNT

process_frame:                                // When next line of NV12MT blocks begins
  eor     BCOUNT,   BCOUNT, #6                // 2<->4, Number of sequental blocks in the beginning of the line of blocks
  push    {BCOUNT}                            // store current BCOUNT in stack

next_line:                                    // Process next line of YUV420
  ldr     BCOUNT,   [sp]                      // BCOUNT = BCOUNT in the stack, without popping it out from the stack
  mov     BACK,     #-64                      // BACK = -64, BACK is the counter how far we need to shift back in NV12MT when we
                                              // reach end of a line in YUV420 and need to fill next line. -64 is to shift exactly to the
                                              // beginning of the next line in NV12MT block to proceed
line_in_block:
  pld     [Y_FROM]                            // Read 64 bytes (one block width) from NV12MT Y-plane line
  vld1.u8 {d0,d1},  [Y_FROM]!
  vld1.u8 {d2,d3},  [Y_FROM]!
  pld     [Y_FROM]
  vld1.u8 {d4,d5},  [Y_FROM]!
  vld1.u8 {d6,d7},  [Y_FROM]!
  vst1.u8 {d0},     [Y_TO]!                   // Store all 64 bits into Y-plane of YUV420
  vst1.u8 {d1},     [Y_TO]!
  vst1.u8 {d2},     [Y_TO]!
  vst1.u8 {d3},     [Y_TO]!
  vst1.u8 {d4},     [Y_TO]!
  vst1.u8 {d5},     [Y_TO]!
  vst1.u8 {d6},     [Y_TO]!
  vst1.u8 {d7},     [Y_TO]!

  add     Y_FROM,   Y_FROM, #1984             // Jump to the beginning of the same horizontal line in next NV12MT Y-plane block to continue the line in YUV420

  cmp     HCOUNT,   HEIGHT, lsr #1            // If HCOUNT > HEIGHT/2 (UV-plane has ended, no need to proceed it further)
  bge     skip_uv                             // then jump to skip_uv

  pld     [UV_FROM]                           // Read 64 bytes from UV-plane of NV12MT
  vld2.u8 {d0,d1},  [UV_FROM]!
  vld2.u8 {d2,d3},  [UV_FROM]!
  pld     [UV_FROM]
  vld2.u8 {d4,d5},  [UV_FROM]!
  vld2.u8 {d6,d7},  [UV_FROM]!
  vst1.u8 {d0},     [U_TO]!                   // Store every odd byte to U-plane of YUV420
  vst1.u8 {d1},     [V_TO]!                   // Every even byte to V-Plane of YUV420
  vst1.u8 {d2},     [U_TO]!
  vst1.u8 {d3},     [V_TO]!
  vst1.u8 {d4},     [U_TO]!
  vst1.u8 {d5},     [V_TO]!
  vst1.u8 {d6},     [U_TO]!
  vst1.u8 {d7},     [V_TO]!

  add     UV_FROM,  UV_FROM, #1984            // Jump to the beginning of the same line in next NV12MT UV-plane block to continue the line in YUV420

skip_uv:
  add     BACK,     #2048                     // Add block size to the shiftback counter

  subs    WCOUNT,   #64                       // Width Counter = 64. This is processed pixel counter in a line, we prcessed 64 pixel in a bulk
  ble     line_end                            // Jump to the line_end if we are at the end of the line (WCOUNT == 0). This is the exit from the loop

  subs    BCOUNT,   #1                        // BCOUNT = BCOUNT - 1, processed block counter in a counter of blocks processed in a pack of 4 sequental NV12MT blocks
  bgt     line_in_block                       // Jump to line_in_block if we are still inside of a block of 4 NV12MT blocks 

  cmp     HCOUNT,   Y_ZHGHT                   // If this is not the last row in Y-plane which is not Z-shaped
  addlt   Y_FROM,   #8192                     // skip 4 blocks to the right
  cmp     HCOUNT,   UV_ZHGHT                  // If this is not the last row in UV-plane which is not Z-shaped
  addlt   UV_FROM,  #8192                     // skip 4 blocks to the right
  add     BACK,     #8192                     // Add size of 4 blocks to BACK shift
  mov     BCOUNT,   #4                        // BCOUNT = 4, reset block numbers to process to 4
  b       line_in_block                       // Jump to line_in_block

line_end:
  add     HCOUNT,   #1                        // HCOUNT = HCOUNT + 1. 1 line of YUV420 processed

  sub     Y_FROM,   Y_FROM, BACK              // roll back through NV12MT Y-plane to the beginning of the line in YUV420
  cmp     Y_ZHGHT,  HCOUNT                    // If this is the last row of NV12MT blocks in Y-plane which is not Z-shaped
  addlt   Y_FROM,   COMPENS                   // compensate BACK shift
  sub     UV_FROM,  UV_FROM, BACK             // roll back through NV12MT UV-plane to the beginning of the line in YUV420
  cmp     UV_ZHGHT, HCOUNT                    // If this is the last row of NV12MT blocks in UV-plane which is not Z-shaped
  addlt   UV_FROM,  COMPENS                   // compensate BACK shift

  ldr     WCOUNT,   [sp, #16*4]               // Load WIDTH (stack element 17 at the moment) to WCOUNT

  ands    BACK,     HCOUNT, #31               // BACK used as tmp, compare HCOUNT%32, is line of blocks ended vertically?
  bne     next_line                           // Proceed to the next_line if not

  ands    BACK,     HCOUNT, #63               // BACK used as tmp, compare HCOUNT%64. Is even line of NV12MT blocks ended vertically?
  addeq   Y_FROM,   WCOUNT, lsl #6            // if yes, we need to skip forward (WIDTH / 32 blocks - 3 block)
  addeq   UV_FROM,  WCOUNT, lsl #6
  subeq   Y_FROM,   #6144
  subeq   UV_FROM,  #6144
  addne   Y_FROM,   #2048                     // if that was odd line which ended we only need to add one block forward to Y-plane
  addne   UV_FROM,  #2048                     // and UV-plane

  pop     {BCOUNT}                            // pop BCOUNT from stack

  cmp     HCOUNT,   HEIGHT                    // compare HCOUNT = HEIGHT
  popge   {r0-r12,pc}                         // if they are equeal - we reached vertical end of the frame. Restore registers and exit the function
  b       process_frame                       // else go to process_frame
  .fnend
