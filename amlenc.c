#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include "vp_multi_codec_1_0.h"

void populateEncoderInfo(vl_encode_info_t *encode_info, int width, int height, int inputFmt, int nbr) {
        encode_info->width= width;
        encode_info->height= height;
        encode_info->bit_rate =  nbr;
        encode_info->frame_rate =  30;
        encode_info->gop =  0;
        encode_info->img_format =  inputFmt;
        encode_info->qp_mode =  0;
        encode_info->profile =  2 /*main*/;
        encode_info->forcePicQpEnable =  0;
        encode_info->forcePicQpI =  0;
        encode_info->forcePicQpB =  0;
        encode_info->forcePicQpP =  0;
        encode_info->frame_rotation =  0;
        encode_info->frame_mirroring =  0;
        encode_info->intra_refresh_mode =  0;
        encode_info->intra_refresh_arg =  0;
        encode_info->prepend_spspps_to_idr_frames =  true;
        encode_info->enc_feature_opts = ENABLE_PARA_UPDATE;
}

void populateQpTbl(qp_param_t *qp_tbl) {
        qp_tbl->qp_min = 0;
        qp_tbl->qp_max = 51;
        qp_tbl->qp_I_base = 30;
        qp_tbl->qp_I_min = 0;
        qp_tbl->qp_I_max = 51;
        qp_tbl->qp_P_base = 30;
        qp_tbl->qp_P_min = 0;
        qp_tbl->qp_P_max = 51;
}
int prepInBuffer(vl_buffer_info_t *inb,int width,int height, int inputFmt){
	int ysize;
	int usize;
	int vsize;
	int uvsize;
        int framesize;
	void *iba;

	memset(inb, 0, sizeof(vl_buffer_info_t));
	if (inputFmt == IMG_FMT_RGB888) {
            framesize = (width * height * 3);
        } else if (inputFmt == IMG_FMT_RGBA8888) {
            framesize = (width * height * 4);
        } else {
            framesize = (width * height * 3 / 2);
        }
	iba = malloc(framesize);
	if (iba == NULL) {
		fprintf(stderr,"malloc failed for %d bytes \n",framesize);
		return 0;
	}
        ysize = width * height;
        usize = width * height / 4;
        vsize = width * height / 4;
        uvsize = width * height / 2;
        inb->buf_type =0 ;//malloc'd
	inb->buf_stride = 0;
	inb->buf_fmt=inputFmt;
	inb->buf_info.in_ptr[0]=(long unsigned int)iba;
	inb->buf_info.in_ptr[1]=(long unsigned int)(iba + ysize);
	inb->buf_info.in_ptr[2]=(long unsigned int)(iba + ysize +usize);
	return framesize;
}

int main(char** argv, int argc){
	vl_encode_info_t encode_info;
	qp_param_t qp_tbl; 
	int inputFmt =IMG_FMT_NV12;
        int codec_id = CODEC_ID_H264;
    	int width = 1920;
    	int height = 1080;
	long int handle;
	vl_buffer_info_t inbuf ;
	vl_buffer_info_t retbuf ;
	int rfd;
	void *fbuff;
	void *hbuff;
	int hbuffsz;
	int enc_frame_type;
	encoding_metadata_t hsz;
	int red;
	int framesize;
	int i;
	int br;
	int nbr;
	long bits ;
	long mean;
	int count;
	
	useconds_t fint;

        nbr = 5000000;
	populateEncoderInfo(&encode_info,width,height,inputFmt,nbr);
	populateQpTbl(&qp_tbl);
	handle = vl_multi_encoder_init(codec_id, encode_info, &qp_tbl);
	if (handle == 0){
		fprintf(stderr,"Can't alloc codec handle, got 0\n");
		exit(1);
	} 
	fint = 33000;
	br = nbr/2;

	printf("Alloced codec ok\n");
	framesize = prepInBuffer(&inbuf,width,height,inputFmt);
        fbuff = (void *) inbuf.buf_info.in_ptr[0];
	rfd = open("/dev/random",O_RDONLY);
	if (rfd > 0){
		hbuffsz = 8 * 1024 * 1024;
		hbuff = malloc(hbuffsz);
		enc_frame_type = FRAME_TYPE_IDR;
		bits =0;
		count =0;
		for (i=0;i<300;i++){
			red = read(rfd,fbuff,framesize);
			if (red < framesize){
				fprintf(stderr,"frame too small %d\n",red);
				break;
			}
			if (i == 150){
			        if (0 == vl_video_encoder_change_bitrate(handle, br)){
					nbr = br;
					enc_frame_type = FRAME_TYPE_IDR;
					bits =0;
					count = 0;
				} else {
					printf("\n");
					fprintf(stderr,"Can't change bitrate\n");
					break;
				}
			}

			hsz = vl_multi_encoder_encode(handle, enc_frame_type, hbuff, &inbuf, &retbuf);
			count++;
			bits += hsz.encoded_data_length_in_bytes;
			mean = 30 * bits *8 / count;
			printf("\r %3d mean is %8ld bitrate is %8d ",i,mean,nbr);
                        usleep(fint);
			fflush(stdout);
			enc_frame_type =  FRAME_TYPE_AUTO;
		}
	}else {
		fprintf(stderr,"can't open dev random for reading\n");
	}
	if (handle != 0){
		vl_multi_encoder_destroy(handle);
	}
	printf("\ndone\n");

}

