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

/********************************************************************************************
 * Description:
 * 
 * Xilinx's High Bandwidth Memory (HBM)-enabled FPGA are the clear solution for providing 
 * massive memory bandwidth within the lowest power, footprint, and system cost envolopes. 
 * 
 * This example is designed to show a simple use case to understand how to use HBM memory. 
 *
 * There are total 32 memory resources referenced as HBM[0:31] by XOCC and each has a 
 * capacity of storing 256MB of data.
 *
 * This example showcases two use cases to differentiate how efficiently one can use HBM banks.
 *
 * CASE 1: 
 *          +-----------+                   +-----------+
 *          |           | ---- Input1 ----> |           |
 *          |           |                   |           |
 *          |   HBM0    | ---- Input2 ----> |   KERNEL  |
 *          |           |                   |           |
 *          |           | <---- Output ---- |           |                 
 *          +-----------+                   +-----------+ 
 *
 *  In this case only one HBM Bank, i.e. HBM0, has been used for both the input
 *  vectors and the processed output vector.
 *
 *  CASE 2:
 *          +-----------+                   +-----------+
 *          |           |                   |           |
 *          |   HBM1    | ---- Input1 ----> |           |
 *          |           |                   |           |
 *          +-----------+                   |           |
 *                                          |           |
 *          +-----------+                   |           |                 
 *          |           |                   |   KERNEL  |
 *          |   HBM2    | ---- Input2 ----> |           |
 *          |           |                   |           |
 *          +-----------+                   |           |
 *                                          |           |
 *          +-----------+                   |           |
 *          |           |                   |           |
 *          |   HBM3    | <---- Output ---- |           |
 *          |           |                   |           |
 *          +-----------+                   +-----------+ 
 *
 *  In this case three different HBM Banks, i.e. HBM1, HBM2 and HBM3, have been used for input
 *  vectors and the processed output vector.
 *  The banks HBM1 & HBM2 are used for input vectors whereas HBM3 is used for
 *  processed output vector.
 *
 *  The use case highlights significant change in the data transfer throughput (in terms of
 *  Gigabytes per second) when a single and multiple HBM banks are used for the
 *  same application.
 *
 *  *****************************************************************************************/

#include "xcl2.hpp"
#include <algorithm>
#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include "ap_int.h"

//Number of HBM Banks required
#define MAX_HBM_BANKCOUNT 32
#define BANK_NAME(n) n | XCL_MEM_TOPOLOGY
const int bank[MAX_HBM_BANKCOUNT] = {
    BANK_NAME(0),  BANK_NAME(1),  BANK_NAME(2),  BANK_NAME(3),  BANK_NAME(4),
    BANK_NAME(5),  BANK_NAME(6),  BANK_NAME(7),  BANK_NAME(8),  BANK_NAME(9),
    BANK_NAME(10), BANK_NAME(11), BANK_NAME(12), BANK_NAME(13), BANK_NAME(14),
    BANK_NAME(15), BANK_NAME(16), BANK_NAME(17), BANK_NAME(18), BANK_NAME(19),
    BANK_NAME(20), BANK_NAME(21), BANK_NAME(22), BANK_NAME(23), BANK_NAME(24),
    BANK_NAME(25), BANK_NAME(26), BANK_NAME(27), BANK_NAME(28), BANK_NAME(29),
    BANK_NAME(30), BANK_NAME(31)};

// Function for verifying results
bool verify(std::vector<ap_uint<512>, aligned_allocator<ap_uint<512>>> &source_sw_results,
            std::vector<ap_uint<512>, aligned_allocator<ap_uint<512>>> &source_hw_results,
            unsigned int size) {
    bool check = true;
    for (size_t i = 0; i < size; i++) {
    	if(i == 0) {
    		std::cout << std::hex << source_hw_results[i] << "\n"
    				<< std::hex << source_sw_results[i]
					<< std::endl;
    	}
        if (source_hw_results[i] != source_sw_results[i]) {
            std::cout << "Error: Result mismatch" << std::endl;
            std::cout << "i = " << i << " CPU result = " << source_sw_results[i]
                      << " Device result = " << source_hw_results[i]
                      << std::endl;
            check = false;
            break;
        }
    }
    return check;
}

double run_krnl(cl::Context &context,
                cl::CommandQueue &q,
                cl::Kernel &kernel,
                std::vector<ap_uint<512>, aligned_allocator<ap_uint<512>>> &source_in1,
                std::vector<ap_uint<512>, aligned_allocator<ap_uint<512>>> &source_in2,
                std::vector<ap_uint<512>, aligned_allocator<ap_uint<512>>> &source_hw_results,
                int *bank_assign,
                unsigned int size) {
    cl_int err;

    // For Allocating Buffer to specific Global Memory Bank, user has to use cl_mem_ext_ptr_t
    // and provide the Banks
    cl_mem_ext_ptr_t inBufExt1, inBufExt2, outBufExt;
    std::cout << bank_assign[0] << std::endl;
    std::cout << bank_assign[1] << std::endl;
    std::cout << bank_assign[2] << std::endl;
    inBufExt1.obj = source_in1.data();
    inBufExt1.param = 0;
    inBufExt1.flags = bank_assign[0*4];

    inBufExt2.obj = source_in2.data();
    inBufExt2.param = 0;
    inBufExt2.flags = bank_assign[1*4];

    outBufExt.obj = source_hw_results.data();
    outBufExt.param = 0;
    outBufExt.flags = bank_assign[2*4];

    // These commands will allocate memory on the FPGA. The cl::Buffer objects can
    // be used to reference the memory locations on the device.
    //Creating Buffers
    std::cout << "Size: " << sizeof(ap_uint<512>) * size << std::endl;
    OCL_CHECK(err,
              cl::Buffer buffer_input1(context,
                                       CL_MEM_READ_ONLY |
                                           CL_MEM_EXT_PTR_XILINX |
                                           CL_MEM_USE_HOST_PTR,
                                       sizeof(ap_uint<512>) * size,
                                       &inBufExt1,
                                       &err));
    OCL_CHECK(err,
              cl::Buffer buffer_input2(context,
                                       CL_MEM_READ_ONLY |
                                           CL_MEM_EXT_PTR_XILINX |
                                           CL_MEM_USE_HOST_PTR,
                                       sizeof(ap_uint<512>) * size,
                                       &inBufExt2,
                                       &err));
    OCL_CHECK(err,
              cl::Buffer buffer_output(context,
                                       CL_MEM_WRITE_ONLY |
                                           CL_MEM_EXT_PTR_XILINX |
                                           CL_MEM_USE_HOST_PTR,
                                       sizeof(ap_uint<512>) * size,
                                       &outBufExt,
                                       &err));

    //Setting the kernel Arguments
    OCL_CHECK(err, err = (kernel).setArg(0, buffer_input1));
    OCL_CHECK(err, err = (kernel).setArg(1, buffer_input2));
    OCL_CHECK(err, err = (kernel).setArg(2, buffer_output));
    OCL_CHECK(err, err = (kernel).setArg(3, size));

    // Copy input data to Device Global Memory
    OCL_CHECK(err,
              err = q.enqueueMigrateMemObjects({buffer_input1, buffer_input2},
                                               0 /* 0 means from host*/));
    q.finish();

    std::chrono::duration<double> kernel_time(0);

    auto kernel_start = std::chrono::high_resolution_clock::now();
    OCL_CHECK(err, err = q.enqueueTask(kernel));
    q.finish();
    auto kernel_end = std::chrono::high_resolution_clock::now();

    kernel_time = std::chrono::duration<double>(kernel_end - kernel_start);

    // Copy Result from Device Global Memory to Host Local Memory
    OCL_CHECK(err,
              err = q.enqueueMigrateMemObjects({buffer_output},
                                               CL_MIGRATE_MEM_OBJECT_HOST));
    q.finish();

    return kernel_time.count();
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <XCLBIN> \n", argv[0]);
        return -1;
    }
    cl_int err;
    std::string binaryFile = argv[1];
    std::cout << binaryFile << std::endl;

    // The get_xil_devices will return vector of Xilinx Devices
    auto devices = xcl::get_xil_devices();
    auto device = devices[0];

    // Creating Context and Command Queue for selected Device
    OCL_CHECK(err, cl::Context context(device, NULL, NULL, NULL, &err));
    OCL_CHECK(
        err,
        cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE, &err));

    std::string device_name = device.getInfo<CL_DEVICE_NAME>();
    std::cout << "Found Device=" << device_name.c_str() << std::endl;

    // read_binary_file() command will find the OpenCL binary file created using the
    // xocc compiler load into OpenCL Binary and return pointer to file buffer.
   auto fileBuf = xcl::read_binary_file(binaryFile);

   cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};
    devices.resize(1);
    OCL_CHECK(err, cl::Program program(context, devices, bins, NULL, &err));
    OCL_CHECK(err, cl::Kernel kernel_vadd(program, "krnl_vadd", &err));

    unsigned int dataSize = 1024;
    if (xcl::is_emulation()) {
        dataSize = 1024;
        std::cout << "Original Dataset is reduced for faster execution on "
                     "emulation flow. Data size="
                  << dataSize << std::endl;
    }

    std::vector<ap_uint<512>, aligned_allocator<ap_uint<512>>> source_in1(dataSize);
    std::vector<ap_uint<512>, aligned_allocator<ap_uint<512>>> source_in2(dataSize);
    std::vector<ap_uint<512>, aligned_allocator<ap_uint<512>>> source_hw_results(dataSize);
    std::vector<ap_uint<512>, aligned_allocator<ap_uint<512>>> source_sw_results(dataSize);

    // Create the test data
    ap_uint<512> query = ap_int<512>("FFFFFFFF22222222FFFFFFFF"
    		"FFFFFFFF22222222FFFFFFFF"
			"FFFFFFFF22222222FFFFFFFF"
			"FFFFFFFF22222222FFFFFFFF"
			"FFFFFFFF22222222FFFFFFFF",16);
    ap_uint<512> data = ap_int<512>("123412342222222212341234"
        		"123412341111111112341234"
    			"123412342222222212341234"
    			"123412341111111112341234"
    			"123412342222222212341234",16);
    std::generate(source_in1.begin(), source_in1.end(), [&query](){return query;});
    std::generate(source_in2.begin(), source_in2.end(), [&data](){return data;});

    for (size_t i = 0; i < dataSize; i++) {
    	ap_uint<512> result = 0;
    	for(int j = 0; j<5; j++) {
    		int base = j*96;
    		if(i == 0) {
    			std::cout << j << std::endl;
    			std::cout << std::hex << (ap_uint<32>)source_in1[i].range(base+31, base) <<std::endl;
    			if(source_in1[i].range(base+31, j) == ap_uint<32>("FFFFFFFF",16)) {
    				if(i==0) std::cout << "------------------" <<std::endl;
    			}
    			std::cout << std::hex << source_in1[i] <<std::endl;
    		}
			if(((ap_uint<32>)source_in1[i].range(base+31, base)==ap_uint<32>("FFFFFFFF",16)
						||(ap_uint<32>)source_in1[i].range(base+31, base) == (ap_uint<32>)source_in2[i].range(base+31, base))
					&& ( (ap_uint<32>)source_in1[i].range(base+32*2-1, base+32)== ap_uint<32>("FFFFFFFF",16)
						||(ap_uint<32>)source_in1[i].range(base+32*2-1, base+32) == (ap_uint<32>)source_in2[i].range(base+32*2-1, base+32))
					&& ( (ap_uint<32>)source_in1[i].range(base+32*3-1, base+32*2)== ap_uint<32>("FFFFFFFF",16)
						||(ap_uint<32>)source_in1[i].range(base+32*3-1, base+32*2) == (ap_uint<32>)source_in2[i].range(base+32*3-1, base+32*2))
					){
				result.range((j+1)*96-1, j*96) = (ap_uint<96>)source_in2[i].range((j+1)*96-1, j*96);

				if(i==0) {
					std::cout << "hit!!!!" << (ap_uint<96>)source_in2[i].range((j+1)*96-1, j*96) <<std::endl;
				}
			}else {
				result.range((j+1)*96-1, j*96) = ap_uint<96>("123456781234567812345678",16);
				if(i==0) std::cout << "no hit...." <<std::endl;
			}
			//if(i==0) std::cout << j << " = " << std::hex <<result <<std::endl;
    	}
        source_sw_results[i] = result;
        source_hw_results[i] = 0;
    }

    double kernel_time_in_sec = 0, result = 0;
    bool match = true;
    const int numBuf = 3*4; // Since three buffers are being used
    int bank_assign[numBuf];

//    std::cout << "Running CASE 1  : Single HBM for all three Buffers "
//              << std::endl;
//    if (!xcl::is_emulation()) {
//        dataSize = 16 * 1024 * 1024;
//        dataSize = 1;
//        std::cout << "Picking Buffer size " << dataSize * sizeof(uint32_t)
//                  << " so that all three buffer should fit into Single HBM "
//                     "(max 256MB)"
//                  << std::endl;
//    }
//
//    std::cout << "Each buffer is allocated with same HBM bank." << std::endl;
//    std::cout << "input 1 -> bank 0 " << std::endl;
//    std::cout << "input 2 -> bank 0 " << std::endl;
//    std::cout << "output  -> bank 0 " << std::endl;
//    for (int j = 0; j < numBuf; j++) {
//        bank_assign[j] = bank[0];
//    }
//
//    kernel_time_in_sec = run_krnl(context,
//                                  q,
//                                  kernel_vadd,
//                                  source_in1,
//                                  source_in2,
//                                  source_hw_results,
//                                  bank_assign,
//                                  dataSize);
//    match = verify(source_sw_results, source_hw_results, dataSize);
//
//    // Multiplying the actual data size by 3 because three buffers are being used.
//    result = 3 * dataSize * sizeof(ap_uint<512>);
//    result /= (1000 * 1000 * 1000); // to GB
//    result /= kernel_time_in_sec;   // to GBps
//    std::cout << "kernel_time_in_sec " << kernel_time_in_sec << std::endl;
//    std::cout << "[CASE 1] THROUGHPUT = " << result << " GB/s" << std::endl;

    std::cout << "Running CASE 2: Three Separate Banks for Three Buffers"
              << std::endl;
    if (!xcl::is_emulation()) {
        std::cout << "For This case each buffer will be having different HBM, "
                     "so buffer size is picked to utilize full HBM"
                  << std::endl;
        dataSize = 2;
        std::cout << "vector size is " << dataSize * sizeof(ap_uint<512>)
                  << " as maximum possible inside single HBM" << std::endl;
    }

    std::cout << "Each buffer is allocated with different HBM bank."
              << std::endl;
    std::cout << "input 1 -> bank 1 " << std::endl;
    std::cout << "input 2 -> bank 2 " << std::endl;
    std::cout << "output  -> bank 3 " << std::endl;
    for (int j = 0; j < numBuf; j++) {
        bank_assign[j] = bank[j + 1];
    }
    std::cout << "finished assigning bank." << std::endl;
    std::cout << "datasize is " << dataSize << std::endl;
    kernel_time_in_sec = run_krnl(context,
                                  q,
                                  kernel_vadd,
                                  source_in1,
                                  source_in2,
                                  source_hw_results,
                                  bank_assign,
                                  dataSize);
    std::cout << "finished running krnl." << std::endl;
    match = verify(source_sw_results, source_hw_results, dataSize);
    std::cout << "finished testing." << std::endl;
    result = 3 * dataSize * sizeof(ap_uint<512>);
    result /= (1000 * 1000 * 1000); // to GB
    result /= kernel_time_in_sec;   // to GBps
    std::cout << "kernel_time_in_sec " << kernel_time_in_sec << std::endl;
    std::cout << "[CASE 2] THROUGHPUT = " << result << " GB/s " << std::endl;


    std::cout << (match ? "TEST PASSED" : "TEST FAILED") << std::endl;
    return (match ? EXIT_SUCCESS : EXIT_FAILURE);
}
