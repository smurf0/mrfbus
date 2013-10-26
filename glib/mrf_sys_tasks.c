#include <mrf_sys.h>
#include <stdio.h>
#include <mrf_cmds.h>




MRF_CMD_RES mrf_task_ack(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){
  printf("mrf_task_ack : bnum %d  IF state %d\n",bnum,ifp->status.state);

  uint8 bn;



  IQUEUE *qp = &(ifp->status.txqueue);

  MRF_PKT_HDR *ackhdr = (MRF_PKT_HDR *)(_mrf_buff_ptr(bnum)+ 0L);
  MRF_PKT_HDR *txhdr; 
  MRF_BUFF_STATE *bs;

  // clear buff state of ack buffer
  _mrf_buff_state(bnum)->state = FREE;

  if ( ifp->status.state != MRF_ST_WAITSACK){    
    ifp->status.stats.unexp_ack++;
    return MRF_CMD_RES_ERROR;
  }


  printf("ack 1\n");
  if (queue_data_avail(qp)){
      bn = queue_head(qp);
      bs = _mrf_buff_state(bn);
      txhdr = (MRF_PKT_HDR *)(_mrf_buff_ptr(bn)+ 0L);
      printf(" qhead is %d state %d\n",bn,bs->state);

      if((bs->state == TX) &&((txhdr->msgid) == (ackhdr->msgid)))
        {
        printf("acknowledge recieved buffer %d \n",bnum);
        queue_pop(qp);
        ifp->status.state = MRF_ST_RX;

        bs->state = FREE;
        return;
        
        }
      else{
        printf("no ack 1\n");

      }
      
  }
  printf("i_f status %d da %d\n",ifp->status.state,queue_data_avail(&(ifp->status.txqueue)));
  printf("no ack 2\n");
  


}
MRF_CMD_RES mrf_task_retry(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){
  printf("mrf_task_retry\n");

}

MRF_CMD_RES mrf_task_resp(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){
  printf("mrf_task_resp\n");


  // send ack
  mrf_sack(bnum);

  MRF_PKT_HDR *hdr1;
  // hdr1 = (MRF_PKT_HDR *)(_mrf_buff_ptr(bnum)+0);
  hdr1 = (MRF_PKT_HDR *)(_mrf_buff_ptr(bnum)+ 0L); // why do we need this 0L???! //sizeof(MRF_PKT_HDR));

  MRF_PKT_RESP *resp = (MRF_PKT_RESP *)(_mrf_buff_ptr(bnum)+sizeof(MRF_PKT_HDR));

  // send a seg ack when we get resp
  printf("Response to packet %s len %d usrc %02X  udest %02X\n",mrf_cmds[resp->type].str,resp->rlen,resp->usrc,resp->udest);

  if (resp->type == mrf_cmd_device_info){
    MRF_PKT_DEVICE_INFO *dev_inf = (MRF_PKT_DEVICE_INFO *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    printf("DEVICE %s  version %s mrfbus version %s\n",dev_inf->dev_name,dev_inf->dev_version,dev_inf->mrfbus_version);
  }
  else if  (resp->type == mrf_cmd_if_info){
    MRF_PKT_IF_INFO *if_inf = (MRF_PKT_IF_INFO *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    printf("IF INFO  rx_pkts %u tx_pkts %u tx_retries %u \n",if_inf->rx_pkts,if_inf->tx_pkts,if_inf->tx_retries);
  }
  else if  (resp->type == mrf_cmd_if_status){
    IF_STATUS *if_status = (IF_STATUS *)((uint8*)resp + sizeof(MRF_PKT_RESP));
    printf("IF STATUS : state %d rx_pkts %u tx_pkts %u tx_retries %u \n",if_status->state,if_status->stats.rx_pkts,if_status->stats.tx_pkts,if_status->stats.tx_retries);
  }  
  _mrf_buff_state(bnum)->state = FREE;

}

MRF_CMD_RES mrf_task_if_info(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){
  printf("mrf_task_if_info\n");

  MRF_PKT_IF_INFO info;
  info.num_if = NUM_INTERFACES;
  info.errors = 0;
  info.tx_retries = 0;
  info.tx_pkts = 0;
  info.rx_pkts = 0;

  I_F i;
  uint32 rxp,txp;
  MRF_IF *i_f;
  for ( i = 0 ; i < NUM_INTERFACES ; i++ ) {
    i_f = mrf_if_ptr(i);
    info.tx_retries += i_f->status.stats.tx_retries;
    info.tx_pkts += i_f->status.stats.tx_pkts;
    info.rx_pkts += i_f->status.stats.rx_pkts;    
  }
  printf("mrf_task_if_info l1\n");

  mrf_data_response( bnum,(uint8 *)&info,sizeof(MRF_PKT_IF_INFO));  
  printf("mrf_task_if_info exit\n");

}
MRF_CMD_RES mrf_task_if_status(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){

  MRF_PKT_IF_STAT_REQ *streq = (MRF_PKT_IF_STAT_REQ *)(_mrf_buff_ptr(bnum)+sizeof(MRF_PKT_HDR));
  printf("mrf_task_if_status request for i_f %d\n",streq->i_f);

  if ( (streq->i_f) >= NUM_INTERFACES)
    return MRF_CMD_RES_ERROR;
  MRF_IF *i_f = mrf_if_ptr(streq->i_f);

  mrf_data_response( bnum,(uint8 *)&i_f->status,sizeof(IF_STATUS));  
  return MRF_CMD_RES_OK;

}
MRF_CMD_RES mrf_task_get_time(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){

}
MRF_CMD_RES mrf_task_test_1(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){

  printf("mrf_task_test_1\n");
}
MRF_CMD_RES mrf_task_test_2(MRF_CMD_CODE cmd,uint8 bnum, MRF_IF *ifp){
  printf("mrf_task_test_2\n");

}
