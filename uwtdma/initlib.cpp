//
// Copyright (c) 2017 Regents of the SIGNET lab, University of Padova.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. Neither the name of the University of Padova (SIGNET lab) nor the
//    names of its contributors may be used to endorse or promote products
//    derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

/**
 * @file   uwtdma/initlib.cpp
 * @author Filippo Campagnaro
 * @version 1.0.0
 *
 * @brief Provides the initializazion of the uwtdma libraries
 */

#include <tclcl.h>
#include <uwtdma.h>
#include <uwtdma-master.h>

extern EmbeddedTcl UwtdmaTclCode;

packet_t PT_FLOATER;

/**
 * Class that describe the Header of TDMA FLOATER Packet
 */
static class FLOATERHeaderClass : public PacketHeaderClass
{
public:
	/**
	 * Constructor of the class
	 */
	FLOATERHeaderClass()
		: PacketHeaderClass("PacketHeader/FLOATER", sizeof(hdr_FLOATER))
	{
		this->bind();
		bind_offset(&hdr_FLOATER::offset_);
	}
} class_hdr_FLOATER;

extern "C" int
Uwtdma_Init()
{

	PT_FLOATER = p_info::addPacket("UWTDMA/FLOATER");
	
	UwtdmaTclCode.load();
	return 0;
}

extern "C" int Cyguwtdma_Init() {
     return Uwtdma_Init();
}
