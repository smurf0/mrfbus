/******************************************************************************
*
* Copyright (c) 2012-16 Gnusys Ltd
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
******************************************************************************/

#include "mrf_sys.h"
#include "mrf_if.h"
#include "mrf_buff.h"
#include  <msp430.h>
#include <legacymsp430.h>
#include "_mrf_rf1.h"

//#define mrf_buff_loaded(buff)  mrf_buff_loaded_if(RF0,buff)

#define mrf_alloc() mrf_alloc_if(RF0)
int rf_if_send_func(I_F i_f, uint8 *buff);
int mrf_rf_init(I_F i_f);

const MRF_IF_TYPE mrf_rf_cc_if = {
 tx_del : 4,
 funcs : { send : rf_if_send_func,
           init : mrf_rf_init }
};



int rf_if_send_func(I_F i_f, uint8 *buff){
  MRF_IF *mif = mrf_if_ptr(i_f);
  _xb_hw_wr_tx_fifo(buff[0] , buff);
}

extern RF_SETTINGS rfSettings;

static void _mrf_init_radio()
{
  // xb_active_on_lpm3();
  WriteRfSettings(&rfSettings);
  WriteSinglePATable(PATABLE_VAL);
}
void _mrf_receive_enable(void);

int mrf_rf_init(I_F i_f){
  ResetRadioCore();
  _mrf_init_radio();
  _mrf_receive_enable();
  return 0;
}

void xb_hw_idle(){
  Strobe(RF_SIDLE);
  Strobe(RF_SFRX);// flush rx fifo
}

void _mrf_receive_enable(void){
  RF1AIES |= BIT9;                          // Falling edge of RFIFG9
  RF1AIFG &= ~BIT9;                         // Clear a pending interrupt
  RF1AIE  |= BIT9;                          // Enable the interrupt 
  
  // Radio is in IDLE following a TX, so strobe SRX to enter Receive Mode
  // _xb_state = XB_ST_RX;
  // Strobe( RF_SFRX  );    
  Strobe( RF_SIDLE );
  Strobe(RF_SFRX);
  Strobe( RF_SRX );      
}



int _xb_hw_rd_rx_fifo(I_F i_f){
  uint8 i,len,bnum;
  uint8 *buff;
  uint8 lenerr = FALSE;
  uint8 ebuff;
  MRF_IF *mif;
  mif = mrf_if_ptr(i_f);
  len = ReadSingleReg( RXBYTES ); 

  if ( len < sizeof(MRF_PKT_HDR) + 2) { // at least 2 are LQI and RSSI
    lenerr = TRUE;    
  }
  else if  (len > _MRF_BUFFLEN -1){ // packet too long
    lenerr = TRUE;
  }
  else{
    bnum = mrf_alloc();

    if ( bnum == _MRF_BUFFS){  // emergency buffer used to collect header + toks only
      mif->status->stats.alloc_err++;
      lenerr = TRUE;
  
    }else {
      buff = _mrf_buff_ptr(bnum);
      buff[0] = len;
    }
  }
  
  while (!(RF1AIFCTL1 & RFINSTRIFG));    // wait for RFINSTRIFG
  RF1AINSTR1B = (RF_RXFIFORD | RF_REGRD);          

  for (i = 0; i < len; i++)    
    if ( lenerr == FALSE)
      buff[i+1] = RF1ADOUT1B;                 // Read DOUT from Radio Core + clears RFDOUTIFG
    else 
      ebuff = RF1ADOUT1B;                     // Also initiates auto-read for next DOUT byte
  if (lenerr){ // re-enable reception
    _mrf_receive_enable();
    return;
  }
  mrf_buff_loaded(bnum);
  return 0;
 
}
int  _xb_hw_wr_tx_fifo(int len , uint8 *buff){
  int i;
  if ((len < 1 ) || ( len > 255 ))
    return -1;
  RF1AIES |= BIT9;                          
  RF1AIFG &= ~BIT9;                         // clear interrupt flag
  RF1AIE |= BIT9;                           // enable TX end-of-packet interrupt  
  //Strobe( RF_STX );                       // Strobe STX   
  /* total bodging - what is this for? */
  Strobe( RF_SIDLE );
  Strobe( RF_SFTX  );  
  Strobe( RF_SNOP );                         // Strobe STX   
  while (!(RF1AIFCTL1 & RFINSTRIFG));        // wait for tx ready  
  RF1AINSTRW = ((RF_TXFIFOWR | RF_REGWR)<<8 ) + (uint8)len; // write reg addr + len
  for (i = 0; i < len; i++){
    while (!(RFDINIFG & RF1AIFCTL1));       
    RF1ADINB = buff[i];                   
  }
  i = RF1ADOUTB;                            // reset RFDOUTIFG flag status byte  
  Strobe( RF_STX );                         // Strobe STX   
  return 0; 
}

void inline xb_hw_disable_tx_eop(void){
RF1AIE &= ~BIT9;                    // Disable TX end-of-packet interrupt
}

void mrf_rf_idle(I_F i_f){
  MRF_IF *ifp = mrf_if_ptr(i_f);
  ifp->status->state = MRF_ST_IDLE;
  Strobe( RF_SIDLE );

}

int _mrf_rf_tx_intr(I_F i_f){
  MRF_IF *ifp = mrf_if_ptr(i_f);
  IF_STATE *if_state = &(ifp->status->state);
  xb_hw_disable_tx_eop();
	// _dbg100(xbst);
        //  (*if_state) = XB_ST_IDLE;
        // P3OUT &= ~BIT6;                     // Turn off LED after Transmit 
  if ((*if_state) ==  MRF_ST_TX){
    _mrf_receive_enable();
    // we're now waiting for SACK
    ifp->status->state = MRF_ST_WAITSACK;    
  }
  else if((*if_state) ==  MRF_ST_ACK){ 
   if (mrf_if_can_sleep(i_f))
      mrf_rf_idle(i_f);
    else{
      _mrf_receive_enable(); 	 
    }
  }else { // some cockup
    ifp->status->stats.st_err ++;
  }

}


static unsigned int RF1AIVREG;

interrupt(CC1101_VECTOR) CC1101_ISR(void)
{
  int xbreceiving = mrf_if_recieving(RF0);
  int xbtransmitting = mrf_if_transmitting(RF0);
  int rx_ournet = 0;
  int rx_crcok = 0;
  //RSTAT = GetRF1ASTATB();
  
  // switch(__even_in_range(RF1AIV,32))        // Prioritizing Radio Core Interrupt 
  RF1AIVREG = RF1AIV;
  //  switch(RF1AIV,32)        // Prioritizing Radio Core Interrupt 
  switch(RF1AIVREG)        // Prioritizing Radio Core Interrupt 
  {
    case  0: break;                         // No RF core interrupt pending                                            
    case  2: break;                         // RFIFG0 
    case  4: break;                         // RFIFG1
    case  6: break;                         // RFIFG2
    case  8: break;                         // RFIFG3
    case 10: break;                         // RFIFG4
    case 12: break;                         // RFIFG5
    case 14: break;                         // RFIFG6          
    case 16: break;                         // RFIFG7
    case 18: break;                         // RFIFG8
    case 20:                                // RFIFG9
      if(xbreceiving)			    // RX end of packet
      {
	_xb_hw_rd_rx_fifo(RF0);
      }
      else if(xbtransmitting)		    // TX end of packet
      {
	_mrf_rf_tx_intr(RF0);

      }

      // else while(1); 			    // trap - sometimes gets in here - 
      break;
    case 22: break;                         // RFIFG10
    case 24: break;                         // RFIFG11
    case 26: break;                         // RFIFG12
    case 28: break;                         // RFIFG13
    case 30: break;                         // RFIFG14
    case 32: break;                         // RFIFG15
  }  
 
}

