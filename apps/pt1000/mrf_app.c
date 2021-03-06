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
#include <mrf_debug.h>


#include "mrf_spi.h"
#include  <msp430.h>
#include "mrf_pinmacros.h"

// all gpio pins to TI EVM 


#define _MR_PORT P2
#define _MR_BIT  0


#define _START_PORT P2
#define _START_BIT  1

#define _CS_PORT P2
#define _CS_BIT  2


#define _RESET_PORT P2
#define _RESET_BIT  3

#define _DRDY_PORT P2
#define _DRDY_BIT 4


volatile int dbg22;

volatile uint8 dbg_u8;
volatile uint8 _rxcnt;
void debfunc(uint8 data){
  dbg_u8 = data;
  _rxcnt += 1;
}

uint8 ads1148_read(uint8 reg){
  _rxcnt = 0 ;
  uint8 b1 = 0x20 + ( reg & 0xf );
  PINLOW(CS);
  __delay_cycles(10);
  mrf_spi_flush_rx();

  mrf_spi_tx(b1);
  mrf_spi_tx(0); // 1 byte
  mrf_spi_tx(0xff); // NOP
  b1 = mrf_spi_rx();
  debfunc(b1);
  while(mrf_spi_data_avail()){
    b1 = mrf_spi_rx();
    debfunc(b1);
  }
  return b1;
}
uint8 ads1148_write(uint8 reg,uint8 data){
  _rxcnt = 0 ;
  uint8 b1 = 0x40 + ( reg & 0xf );
  PINLOW(CS);
  __delay_cycles(10);
  mrf_spi_flush_rx();

  mrf_spi_tx(b1);
  mrf_spi_tx(0); // 1 byte
  mrf_spi_tx(data);

  mrf_spi_flush_rx();

}

int ads1148_init(){
  // start output
  PINHIGH(START);
  OUTPUTPIN(START);
  // cs output
  PINHIGH(CS);
  OUTPUTPIN(CS);
  //reset output
  PINHIGH(RESET);
  OUTPUTPIN(RESET);

  // MR output
  PINHIGH(MR);
  OUTPUTPIN(MR);

  // DRDY INPUT
  INPUTPIN(DRDY);

  __delay_cycles(10);
  PINLOW(RESET);
  PINLOW(MR);
  __delay_cycles(10);
  PINHIGH(RESET);
  PINHIGH(MR);
  __delay_cycles(10);
  mrf_spi_flush_rx();
}


int mrf_app_init(){
  dbg22 = 101; 
  mrf_spi_init();
}

MRF_CMD_RES mrf_task_usr_resp(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){
  _mrf_buff_free(bnum);
  return MRF_CMD_RES_OK;
}


MRF_CMD_RES mrf_app_task_test(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){
  mrf_debug("mrf_app_task_test entry\n");
  uint8 *rbuff = mrf_response_buffer(bnum);
  mrf_rtc_get(rbuff);
  mrf_send_response(bnum,sizeof(TIMEDATE));
  mrf_debug("mrf_app_task_test exit\n");
  return MRF_CMD_RES_OK;
}

MRF_CMD_RES mrf_app_spi_read(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){
  mrf_debug("mrf_app_read_spi entry bnum %d\n",bnum);
  MRF_PKT_UINT8 *data = (MRF_PKT_UINT8 *)((uint8 *)_mrf_buff_ptr(bnum) + sizeof(MRF_PKT_HDR));
  uint8 rd = ads1148_read(data->value);
  MRF_PKT_UINT8 *rbuff = (MRF_PKT_UINT8 *)mrf_response_buffer(bnum);
  rbuff->value  = rd;
  mrf_send_response(bnum,sizeof(MRF_PKT_UINT8));
  mrf_debug("mrf_app_task_app_read_spi exit\n");
  return MRF_CMD_RES_OK;
}

MRF_CMD_RES mrf_app_spi_write(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){
  mrf_debug("mrf_app_read_spi entry bnum %d\n",bnum);
  MRF_PKT_UINT8_2 *data = (MRF_PKT_UINT8_2 *)((uint8 *)_mrf_buff_ptr(bnum) + sizeof(MRF_PKT_HDR));
  ads1148_write(data->d0,data->d1);
  mrf_send_response(bnum,0);
  mrf_debug("mrf_app_task_app_write_spi exit\n");
  return MRF_CMD_RES_OK;
}


