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
 * @file   uwtdma.cpp
 * @author Filippo Campagnaro
 * @author Roberto Francescon
 * @version 1.0.0
 *
 * @brief Provides the implementation of the class <i>UWTDMA</i>.
 *
 */

#include "uwtdma-master.h"
#include <iostream>
#include <stdint.h>
#include <mac.h>
#include <uwmmac-clmsg.h>
#include <uwcbr-module.h>

/**
 * Class that represent the binding of the protocol with tcl
 */
static class TDMA_MASTERModuleClass : public TclClass
{

public:
	/**
	 * Constructor of the TDMAGenericModule class
	 */
	TDMA_MASTERModuleClass()
		: TclClass("Module/UW/TDMA_MASTER")
	{
	}
	/**
	 * Creates the TCL object needed for the tcl language interpretation
	 * @return Pointer to an TclObject
	 */
	TclObject *
	create(int, const char *const *)
	{
		return (new UwTDMA_MASTER());
	}

} class_uwtdma_master;

void
UwTDMA_MASTERTimer::expire(Event *e)
{
	((UwTDMA_MASTER *) module)->changeStatus();
}

UwTDMA_MASTER::UwTDMA_MASTER()
	: MMac()
	, tdma_master_timer(this)
	, slot_status(UW_TDMA_MASTER_STATUS_NOT_MY_SLOT)
	, slot_duration(0)
	, transceiver_status(IDLE)
	, start_time(0)
	, tot_slots(0)
	, out_file_stats(0)
	, HDR_size(0)
	, guard_time(0)
	, max_queue_size(10)
	, enable(true)
	, max_packet_per_slot(1)
	, packet_sent_curr_slot_(0)
	, drop_old_(0)
	, name_label_("")
	, checkPriority(0)
	, detected_larvae (false)
	, concentration_table {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	, tdoa_table {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
{
	bind("queue_size_", (int *) &max_queue_size);
	bind("frame_duration", (double *) &frame_duration);
	bind("debug_", (int *) &debug_);
	bind("sea_trial_", (int *) &sea_trial_);
	bind("fair_mode", (int *) &fair_mode);
	bind("HDR_size_", (int *) &HDR_size);
	bind("max_packet_per_slot", (int *) &max_packet_per_slot);
	bind("drop_old_", (int *) &drop_old_);
	bind("checkPriority_", (int *) &checkPriority);
	bind("mac2phy_delay_", (double *) &mac2phy_delay_);
	if (fair_mode == 1) {
		bind("guard_time", (double *) &guard_time);
		bind("tot_slots", (int *) &tot_slots);
	}

	if (max_queue_size < 0) {
		cerr << NOW << " UwTDMAMASTER() not valid max_queue_size < 0!! set to 1 by default " 
			 << std::endl;
		max_queue_size = 1;
	}
	if (frame_duration < 0) {
		cerr << NOW << " UwTDMAMASTER() not valid frame_duration < 0!! set to 1 by default " 
			 << std::endl;
		frame_duration = 1;
	}
	if (max_packet_per_slot < 0) {
		cerr << NOW << " UwTDMAMASTER() not valid max_packet_per_slot < 0!! set to 1 by default " 
			 << std::endl;
		max_packet_per_slot = 1;
	}
	if (drop_old_ == 1 && checkPriority == 1) {
		cerr << NOW << " UwTDMAMASTER() drop_old_ and checkPriority cannot be set both to 1!! "
			 << "checkPriority set to 0 by default " << std::endl;
		checkPriority = 0; 
	}
	if (mac2phy_delay_ <= 0) {
		cerr << NOW << " UwTDMAMASTER() not valid mac2phy_delay_ < 0!! set to 1e-9 by default "
			 << std::endl; 
		mac2phy_delay_ = 1e-9;
	}
}

UwTDMA_MASTER::~UwTDMA_MASTER()
{
}

void
UwTDMA_MASTER::update_concentration_table(int node_id, int concentration)
{
	concentration_table[node_id] = concentration;
	std::cout << NOW << " _MASTER node ID (" << addr 
			<< ")My conc table now is " << std::endl;
	
	for (int i = 0; i < 10; i++) {
        	std::cout << concentration_table[i] << ' '<< std::endl;
    	}
	
}

void
UwTDMA_MASTER::update_tdoa_table(int node_id, double tdoa)
{
	tdoa_table[node_id] = tdoa;
	std::cout << NOW << " _MASTER node ID (" << addr 
			<< ")My tdoa table now is " << std::endl;
	
	for (int i = 0; i < 10; i++) {
        	std::cout << tdoa_table[i] << ' '<< std::endl;
    	}
	
}

void
UwTDMA_MASTER::recvFromUpperLayers(Packet *p)
{
	incrUpperDataRx();
	if (buffer.size() < max_queue_size) {
		initPkt(p);
		if (checkPriority == 0) {
			buffer.push_back(p);
		} else {
			hdr_uwcbr *uwcbrh = HDR_UWCBR(p);
			if (uwcbrh->priority() == 0) {
					buffer.push_back(p);
				if (debug_)
					std::cout << NOW << " _MASTER(" << addr 
					<< ")::insert packet with standard priority in the queue" << std::endl;
			} else {
				buffer.push_front(p);
				if (debug_)
					std::cout << NOW << " _MASTER(" << addr 
					<< ")::insert packet with high priority in the queue" << std::endl;
			}
		}
 		
 	} else {
	if (debug_)
			cout << NOW << " TDMA(" << addr
				 << ")::recvFromUpperLayers() dropping pkt due to buffer full "
				 << std::endl;
		if (drop_old_) {
			Packet *p_old = buffer.front();
			buffer.pop_front();
			Packet::free(p_old);
			initPkt(p);
			buffer.push_back(p);
		}else if (checkPriority == 1) {
			hdr_uwcbr *uwcbrh = HDR_UWCBR(p);
			if (uwcbrh->priority() == 1) {
				Packet *p_old = buffer.back();
				buffer.pop_back();
				Packet::free(p_old);
				initPkt(p);
				buffer.push_front(p);
			}
		}
		else
			Packet::free(p);
	}
	txData();
}

void
UwTDMA_MASTER::stateTxData()
{
	//if (transceiver_status == TRANSMITTING)
	//	transceiver_status = IDLE;
	txData();
}

void
UwTDMA_MASTER::txData()
{
	if (packet_sent_curr_slot_ < max_packet_per_slot) {
		if (slot_status == UW_TDMA_MASTER_STATUS_MY_SLOT && transceiver_status == IDLE) {
			if (buffer.size() > 0) {
				Packet *p = buffer.front();
				buffer.pop_front();
				Mac2PhyStartTx(p);
				incrDataPktsTx();
			}
		} else if (debug_) {
			if (slot_status != UW_TDMA_MASTER_STATUS_MY_SLOT)
				std::cout << NOW << " ID " << addr << ": Wait my slot to send"
						  << std::endl;
			else
				std::cout << NOW << " ID " << addr
						  << ": Wait earlier packet expires to send the current one"
						  << std::endl;
		}
	}
	else {
		if (sea_trial_)
			out_file_stats << left << "[" << getEpoch() << "]::" << NOW << "::TDMA_node(" 
						<< addr << ")::already_tx max packet = " << max_packet_per_slot 
						<< std::endl;
		if (debug_)
			cout << NOW << " TDMA(" << addr
				 << ")::already_tx max packet = " << max_packet_per_slot
				 << std::endl;
	}
}

void
UwTDMA_MASTER::Mac2PhyStartTx(Packet *p)
{
	if (!sea_trial_)
		assert(transceiver_status == IDLE);

	transceiver_status = TRANSMITTING;
	if(sea_trial_) {
		sendDown(p, 0.01);
	} else {
		MMac::Mac2PhyStartTx(p);
	}

	if (debug_ < -5)
		std::cout << NOW << " ID " << addr << ": Sending packet" << std::endl;
	if (sea_trial_)
		out_file_stats << left << "[" << getEpoch() << "]::" << NOW
					   << "::TDMA_node(" << addr << ")::PCK_SENT packet_sent_curr_slot_ = " 
					   << packet_sent_curr_slot_ << " max_packet_per_slot = " 
					   << max_packet_per_slot << std::endl;
}

void
UwTDMA_MASTER::Phy2MacEndTx(const Packet *p)
{
	transceiver_status = IDLE;
	packet_sent_curr_slot_++;
	if (sea_trial_)
		out_file_stats << left << "[" << getEpoch() << "]::" << NOW
					   << "::TDMA_node(" << addr << ")::Phy2MacEndTx(p)"
					   << std::endl;

	txData();
}

void
UwTDMA_MASTER::Phy2MacStartRx(const Packet *p)
{
	if (sea_trial_ != 1)
		assert(transceiver_status != RECEIVING);

	if (transceiver_status == IDLE)
		transceiver_status = RECEIVING;
}

void
UwTDMA_MASTER::Phy2MacEndRx(Packet *p)
{
	if (transceiver_status != TRANSMITTING) {
		hdr_cmn *ch = HDR_CMN(p);
		hdr_mac *mach = HDR_MAC(p);
		hdr_FLOATER *floater = HDR_FLOATER(p);
		
		int node_id = floater->node_id;
		int concentration = floater->conc; 
		
		int dest_mac = mach->macDA();
		int src_mac = mach->macSA();
		
		if(concentration > 7){ //this is just an example
			detected_larvae = true;
		}

		if (ch->error()) {
			if (debug_)
				cout << NOW << " TDMA(" << addr
					 << ")::Phy2MacEndRx() dropping corrupted pkt from node = "
					 << src_mac << std::endl;

			incrErrorPktsRx();
			Packet::free(p);
		} else {
			if (dest_mac != addr && dest_mac != MAC_BROADCAST) {
				rxPacketNotForMe(p);

			} else {
				sendUp(p);
				incrDataPktsRx();
			}
		}

		transceiver_status = IDLE;
		
		update_concentration_table(node_id, concentration);
		if (detected_larvae){ //if good larvae concentration is detected -> master nodes are surfacing -> start locating
			update_tdoa_table (node_id, NOW);
		}

		if (slot_status == UW_TDMA_MASTER_STATUS_MY_SLOT)
			txData();

	} else {
		if (debug_)
			std::cout << NOW << " ID " << addr
					  << ": Received packet while transmitting " << std::endl;
		if (sea_trial_) {
			out_file_stats << left << "[" << getEpoch() << "]::" << NOW
						   << "::TDMA_node(" << addr << ")::RCVD_PCK_WHILE_TX"
						   << std::endl;
			sendUp(p);
			incrDataPktsRx();
		} else {
			Packet::free(p);
		}
	}
}

void
UwTDMA_MASTER::initPkt(Packet *p)
{
	hdr_cmn *ch = hdr_cmn::access(p);
	hdr_mac *mach = HDR_MAC(p);
	hdr_FLOATER *floater = HDR_FLOATER(p);
	
	floater->conc = 5;
	floater->ts = 12.02;
	floater-> node_id = addr;

	int curr_size = ch->size();

	ch->size() = curr_size + HDR_size;
}


void
UwTDMA_MASTER::rxPacketNotForMe(Packet *p)
{
	if (p != NULL)
		Packet::free(p);
}

void
UwTDMA_MASTER::changeStatus()
{
	packet_sent_curr_slot_ = 0;
	if (slot_status == UW_TDMA_MASTER_STATUS_MY_SLOT) {
		slot_status = UW_TDMA_MASTER_STATUS_NOT_MY_SLOT;
		UwTDMA_MASTER::tdma_master_timer.resched(frame_duration - slot_duration + guard_time);

		if (debug_ < -5)
			std::cout << NOW << " Off ID " << addr << " "
					  << frame_duration - slot_duration + guard_time << ""
					  << std::endl;
		if (sea_trial_)
			out_file_stats << left << "[" << getEpoch() << "]::" << NOW
						   << "::TDMA_node(" << addr << ")::Off" << std::endl;
	} else {
		slot_status = UW_TDMA_MASTER_STATUS_MY_SLOT;
		UwTDMA_MASTER::tdma_master_timer.resched(slot_duration - guard_time);

		if (debug_ < -5)
			std::cout << NOW << " On ID " << addr << " "
					  << slot_duration - guard_time << " " << std::endl;
		if (sea_trial_)
			out_file_stats << left << "[" << getEpoch() << "]::" << NOW
						   << "::TDMA_node(" << addr << ")::On" << std::endl;

		stateTxData();
	}
}

void
UwTDMA_MASTER::start(double delay)
{

	enable = true;
	if (sea_trial_) {
		std::stringstream stat_file;
		stat_file << "./TDMA_node_" << addr << "_" << name_label_ << ".out";
		std::cout << stat_file.str().c_str() << std::endl;
		out_file_stats.open(stat_file.str().c_str(), std::ios_base::app);

		out_file_stats << left << "[" << getEpoch() << "]::" << NOW
					   << "::TDMA_node(" << addr << ")::Start_simulation"
					   << std::endl;
	}

	UwTDMA_MASTER::tdma_master_timer.sched(delay);

	if (debug_ < -5)
		std::cout << NOW << " Status " << slot_status << " on ID " << addr
				  << " " << std::endl;
}

void
UwTDMA_MASTER::stop()
{
	enable = false;
	UwTDMA_MASTER::tdma_master_timer.cancel();
	if (sea_trial_)
		out_file_stats << left << "[" << getEpoch() << "]::" << NOW
					   << "::TDMA_node(" << addr << ")::TDMA_stopped_"
					   << std::endl;
}

int
UwTDMA_MASTER::command(int argc, const char *const *argv)
{
	Tcl &tcl = Tcl::instance();
	if (argc == 2) {
		if (strcasecmp(argv[1], "start") == 0) {
			if (fair_mode == 1) {
				if (tot_slots == 0) {
					std::cout << "Error: number of slots set to 0" << std::endl;
					return TCL_ERROR;
				} else {
					slot_duration = frame_duration / tot_slots;
					if (slot_duration - guard_time < 0) {
						std::cout
								<< "Error: guard time or frame set incorrectly"
								<< std::endl;
						return TCL_ERROR;
					} else {
						start_time = slot_number * slot_duration;
						start(start_time);
						return TCL_OK;
					}
				}
			}
			start(start_time);
			return TCL_OK;
		} else if (strcasecmp(argv[1], "stop") == 0) {
			stop();
			return TCL_OK;
		} else if (strcasecmp(argv[1], "get_buffer_size") == 0) {
			tcl.resultf("%d", buffer.size());
			return TCL_OK;
		} else if (strcasecmp(argv[1], "get_upper_data_pkts_rx") == 0) {
			tcl.resultf("%d", up_data_pkts_rx);
			return TCL_OK;
		} else if (strcasecmp(argv[1], "get_sent_pkts") == 0) {
			tcl.resultf("%d", data_pkts_tx);
			return TCL_OK;
		} else if (strcasecmp(argv[1], "get_recv_pkts") == 0) {
			tcl.resultf("%d", data_pkts_rx);
			return TCL_OK;
		}
	} else if (argc == 3) {
		if (strcasecmp(argv[1], "setStartTime") == 0) {
			start_time = atof(argv[2]);
			return TCL_OK;
		} else if (strcasecmp(argv[1], "setSlotDuration") == 0) {
			if (fair_mode == 1) {
				std::cout << "Fair mode is being used! Change to generic TDMA"
						  << std::endl;
				return TCL_ERROR;
			} else {
				slot_duration = atof(argv[2]);
				return TCL_OK;
			}
		} else if (strcasecmp(argv[1], "setGuardTime") == 0) {
			if (fair_mode == 1) {
				std::cout << "Fair mode is being used! Change to generic TDMA"
						  << std::endl;
				return TCL_ERROR;
			} else {
				guard_time = atof(argv[2]);
				return TCL_OK;
			}
		} else if (strcasecmp(argv[1], "setSlotNumber") == 0) {
			slot_number = atoi(argv[2]);
			return TCL_OK;
		} else if (strcasecmp(argv[1], "setMacAddr") == 0) {
			addr = atoi(argv[2]);
			if (debug_)
				cout << "TDMA MAC address of current node is " << addr
					 << std::endl;
			return TCL_OK;
		} else if (strcasecmp(argv[1], "setLogLabel") == 0) {
			name_label_ = argv[2];
			if (debug_)
				cout << "TDMA name_label_ " << name_label_
					 << std::endl;
			return TCL_OK;
		} 
	}
	return MMac::command(argc, argv);
}

int
UwTDMA_MASTER::recvSyncClMsg(ClMessage *m)
{
	if (m->type() == CLMSG_UWMMAC_ENABLE) {
		ClMsgUwMmacEnable *command = dynamic_cast<ClMsgUwMmacEnable *>(m);
		if (command->getReqType() == ClMsgUwMmac::SET_REQ) {
			(command->getEnable()) ? start(NOW + start_time) : stop();
			command->setReqType(ClMsgUwMmac::SET_REPLY);
			return 0;
		}
	}
	return MMac::recvSyncClMsg(m);
}
