/**********
Copyright (c) 2018, Xilinx, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********/

/*******************************************************************************
Description: 
    This is a simple vector addition example using C++ HLS.
    
*******************************************************************************/

#define VDATA_SIZE 1
#include "ap_int.h"
//TRIPCOUNT indentifier
const unsigned int c_dt_size = VDATA_SIZE;

extern "C" {
void krnl_vadd(const ap_uint<512> *in1,        // Read-Only Vector 1
               const ap_uint<512> *in2,        // Read-Only Vector 2
			   ap_uint<512> *out_r,            // Output Result for Addition
               const unsigned int size // Size in integer
) {
#pragma HLS INTERFACE m_axi port = in1 offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = in2 offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = out_r offset = slave bundle = gmem2

#pragma HLS INTERFACE s_axilite port = in1 bundle = control
#pragma HLS INTERFACE s_axilite port = in2 bundle = control
#pragma HLS INTERFACE s_axilite port = out_r bundle = control
#pragma HLS INTERFACE s_axilite port = size bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

#pragma HLS DATA_PACK variable = in1
#pragma HLS DATA_PACK variable = in2
#pragma HLS DATA_PACK variable = out_r

vadd1:
    for (int i = 0; i < size; i++) {
       #pragma HLS PIPELINE II=1
       ap_uint<512> result = 0;

		for(int j = 0; j<5; j++) {
			int base = j*96;
			if(((ap_uint<32>)in1[i].range(base+31, base)==ap_uint<32>("FFFFFFFF",16)
						||(ap_uint<32>)in1[i].range(base+31, base) == (ap_uint<32>)in2[i].range(base+31, base))
					&& ( (ap_uint<32>)in1[i].range(base+32*2-1, base+32)== ap_uint<32>("FFFFFFFF",16)
						||(ap_uint<32>)in1[i].range(base+32*2-1, base+32) == (ap_uint<32>)in2[i].range(base+32*2-1, base+32))
					&& ( (ap_uint<32>)in1[i].range(base+32*3-1, base+32*2)== ap_uint<32>("FFFFFFFF",16)
						||(ap_uint<32>)in1[i].range(base+32*3-1, base+32*2) == (ap_uint<32>)in2[i].range(base+32*3-1, base+32*2))
					){
				result.range((j+1)*96-1, j*96) = (ap_uint<96>)in2[i].range((j+1)*96-1, j*96);
			}else {
				result.range((j+1)*96-1, j*96) = ap_uint<96>("123456781234567812345678",16);
			}
		}
		out_r[i] = result;
    }
}
}
