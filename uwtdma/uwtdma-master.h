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
 * @file   uwtdma.h
 * @author Filippo Campagnaro
 * @author Roberto Francescon
 * @version 1.0.0
 *
 * @brief Provides the definition of the class <i>UWTDMA</i>.
 *
 */

#ifndef UWTDMA_MASTER_H
#define UWTDMA_MASTER_H

#include <mmac.h>
#include <queue>
#include <deque>
#include <iostream>
#include <assert.h>
#include <sstream>
#include <fstream>
#include <sys/time.h>
#include "floater-pkt-hdr.h"

#define UW_TDMA_MASTER_STATUS_MY_SLOT 1 /**< Status slot active>*/
#define UW_TDMA_MASTER_STATUS_NOT_MY_SLOT 2 /**< Status slot not active >*/



class UwTDMA_MASTER;

/**
 * UwTDMATimer class is used to handle the scheduling period of <i>UWTDMA</i>
 * slots.
 */

class UwTDMA_MASTERTimer : public TimerHandler
{

public:
	/**
	 * Costructor of the class UwTDMATimer
	 * @param Pointer of a UwTDMA object
	 */
	UwTDMA_MASTERTimer(UwTDMA_MASTER *m)
		: TimerHandler()
	{
		assert(m != NULL);
		module = m;
	}

protected:
	/**
	 * Method call when the timer expire
	 * @param Event*  pointer to an object of type Event
	 */
	virtual void expire(Event *e);
	UwTDMA_MASTER *module;
};

/**
 * Class that represents a TDMA Node
 */
class UwTDMA_MASTER : public MMac
{

	friend class UwTDMA_MASTERTimer;

public:
	/**
	 * Constructor of the TDMA class
	 */
	UwTDMA_MASTER();

	/**
	 * Destructor of the TDMA class
	 */
	virtual ~UwTDMA_MASTER();


  /**
   * Cross-Layer messages synchronous interpreter.
   * 
   * @param ClMessage* an instance of ClMessage that represent the message received
   * @return <i>0</i> if successful.
   */
	virtual int recvSyncClMsg(ClMessage* m);

protected:

	virtual void update_concentration_table(int node_id, int concentraion);
	
	virtual void update_tdoa_table(int node_id, double tdoa);
	
	virtual void txData();
	/**
	 * Change transceiver status and and start to transmit if in my slot
	 * Used when there's spare time, useful for transmitting other packtes.
	 */
	virtual void stateTxData();
	/**
	 * Alternate TDMA status between MY_STATUS and NOT_MY_STATUS
	 */
	virtual void changeStatus();
	/**
	 * Schedule the beginning of each TDMA cycle, each one after \p delay
	 * @param delay to await before starting the TDMA
	 */
	virtual void start(double delay);
	/**
	 * Terminate a TDMA cycle, essentially cancel the TDMA timer
	 */
	virtual void stop();
	/**
	 * Receive the packet from the upper layer (e.g. IP)
	 * @param Packet* pointer to the packet received
	 *
	 */
	virtual void recvFromUpperLayers(Packet *p);
	/**
	 * Method called when the Phy Layer finish to receive a Packet
	 * @param const Packet* Pointer to an Packet object that rapresent the
	 * Packet in reception
	 */
	virtual void Phy2MacEndRx(Packet *p);
	/**
	 * Method called when the Phy Layer start to receive a Packet
	 * @param const Packet* Pointer to an Packet object that rapresent the
	 * Packet in reception
	 */
	virtual void Phy2MacStartRx(const Packet *p);
	/**
	 * Method called when the Mac Layer start to transmit a Packet
	 * @param const Packet* Pointer to an Packet object that rapresent the
	 * Packet in transmission
	 */
	virtual void Mac2PhyStartTx(Packet *p);
	/**
	 * Method called when the Mac Layer finish to transmit a Packet
	 * @param const Packet* Pointer to an Packet object that rapresent the
	 * Packet in transmission
	 */
	virtual void Phy2MacEndTx(const Packet *p);
	/**
	 * Method called when the Packet received is determined to be not for me
	 * @param const Packet* Pointer to an Packet object that rapresent the
	 * Packet in reception
	 */
	virtual void rxPacketNotForMe(Packet *p);
	/**
	 * Method called to add the MAC header size
	 * @param const Packet* Pointer to an Packet object that rapresent the
	 * Packet in transmission
	 */
	virtual void initPkt(Packet *p);
	/**
	 * Calculate the epoch of the event. Used in sea-trial mode
	 * @return the epoch of the system
	 */
	inline unsigned long int
	getEpoch()
	{
		return time(NULL);
	}

	/**
	 * TCL command interpreter. It implements the following OTcl methods:
	 *
	 * @param argc Number of arguments in <i>argv</i>.
	 * @param argv Array of strings which are the command parameters
							 (Note that <i>argv[0]</i> is the name of the
	 object).
	 * @return TCL_OK or TCL_ERROR whether the command has been dispatched
															  successfully or
	 not.
	 */
	virtual int command(int argc, const char *const *argv);
	/**
	 * Enumeration class of UWTDMA status.
	 */
	enum UWTDMA_MASTER_STATUS { IDLE, TRANSMITTING, RECEIVING };
	double slot_duration; /**<Slot duration*/
	UWTDMA_MASTER_STATUS
	transceiver_status; /**<Variable holding the status enum type*/
	UwTDMA_MASTERTimer tdma_master_timer; /**<TDMA timer handler*/
	int slot_status; /**<Is it my turn to transmit data?*/
	int debug_; /**<Debug variable: 0 for no info,
				>-5 for small info, <-5 for complete info*/
	int sea_trial_; /**<Written log variable*/
	int fair_mode; /**<Fair modality on if 1: then only set
					   tot_slots and common guard_time*/
					   
	double start_time; /**<Time to wait before starting the protocol*/
	int tot_slots; /**<Number of slots in the frame (fair_mode)*/
	double guard_time; /**<Guard time between slots*/

	int slot_number; /**<set the position of the node in the frame (fair_mode)
										  (starting from 0 to tot_slots-1)*/
	std::ofstream out_file_stats; /**<File stream for the log file*/
	int HDR_size; /**<Size of the HDR if any*/
	double frame_duration; /**<Frame duration*/
	
	
	
	std::deque<Packet *> buffer; /**<Buffer of the MAC node*/
	int max_queue_size; /**< Maximum dimension of Queue */
	int packet_sent_curr_slot_; /**<counter of packet has been sent in the
								current slot */
	bool enable;
	int drop_old_; /**<flag to set the drop packet policy in case of buffer overflow: 
					if 0 (default) drops the new packet, if 1 the oldest*/
	
	

	
	int max_packet_per_slot; /**<max numer of packet it can transmit per slot */
	
	
	std::string name_label_; /**<label added in the log file, empty string by default*/
	int checkPriority; /**<flag to set to 1 if UWCBR module uses packets with priority,
						set to 0 otherwise. Priority can be used only with UWCBR module */
	bool detected_larvae;
	int concentration_table [10];
	double tdoa_table [10];
};

#endif
