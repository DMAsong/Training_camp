
/*
  2020.06.28，lwt,
 *****************************************************************************************/
#include "ci_audio_wrapfft.h"
#include "sdk_default_config.h"

#ifndef WIN32
#   include "../../fft/ci_fft.h"
#endif
//#include "../ci_alg_malloc.h"
//#include "ci112x_system.h"

#include "ci_log.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* macros
***************************************************************************************/
#define CI_AUDIO_WRAPFFT_VERSION  10001
#define INTERNAL_DEVELOPER_VERSION 2020070901

// For fft and ifft
#define HPF_ENABLE         0//默认使用高通滤波器，0为关闭高通滤波器

#if HAS_POWER_FREQUENCY_DISTURB
#undef HPF_ENABLE
#define HPF_ENABLE (1)
#define HPF_CUT_OFF_FREQ   3//31.25
#endif



#define USE_HALF_FRAME_WIN (0)//
#define USE_SIN_WINDOW (0)

//#define USE_IFFT (1)

#undef M_PI
#define M_PI (3.14159265358979323846f)

#ifdef WIN32
#   define inline 
#   define malloc(a) malloc(a)
#endif

/* function definitions
***************************************************************************************/
#ifdef WIN32
static inline float roundf( float d )
{
    return floorf(d+0.5f);
}
#endif


int ci_audio_wrafft_version( void )
{
    return CI_AUDIO_WRAPFFT_VERSION;
}

int ci_audio_wrafft_developer_version( void )
{
    return INTERNAL_DEVELOPER_VERSION;
}


/*
static inline void hanming_window( float *w, int wlen )
{
    int i = 0;
    float step = 2*M_PI / (float)wlen;
    float v = 0.5f * M_PI / (float)wlen;
    for (i = 0; i < wlen; i++)
    {
        w[i] = 0.54f - 0.46f * cosf(v);
        v = v + step;
    }
}
*/

/*static void sin_window( float *w, int wlen )//存窗长的一半
{
    int i = 0;
    float step = M_PI / (float)wlen;
    float v = 0.5f * M_PI / (float)wlen;
	//for (i=0; i<wlen; i++)
#if USE_HALF_FRAME_WIN
     for (i=0; i<wlen/2; i++)
    {
        w[i] = sinf( v );
        v = v + step;
    }
#else
    for (i=0; i<wlen; i++)
    {
        w[i] = sinf( v );
        v = v + step;
    }
#endif
}*/
static int hanmming_window_fixedpoint(short* out_ptr,int frame_len)
{

   float alpha = 0.46f;
#if USE_HALF_FRAME_WIN
    for(int i = 0;i < frame_len/2;i++)
    {
        float x = (1 - alpha) -alpha*cosf(2*3.1415926f*i/(frame_len -1));
        int y = (int)(x*32767.0f);
        if(y > 32767)
        {
            y = 32767;
        }
        else if(y < -32767)
        {
            y = -32767;
        }
        out_ptr[i] = (short)y;
    }
#else
    for(int i = 0;i < frame_len;i++)
    {
        float x = (1 - alpha) -alpha*cosf(2*3.1415926f*i/(frame_len -1));
        int y = (int)(x*32767.0f);
        if(y > 32767)
        {
            y = 32767;
        }
        else if(y < -32767)
        {
            y = -32767;
        }
        out_ptr[i] = (short)y;
    }
#endif
    return 0;
}


/****************************************************************************
 * read the *.h file please
 ****************************************************************************/

extern int ci_software_fft_w1024_s512_init( void );

ci_wrapfft_audio* ci_warpfft_create(int mic_channel_num,int ref_channel_num, int frame_size, int enable_hardware_fft)
{
    int i = 0;
    int ret = 0;  
    ci_wrapfft_audio *h = (ci_wrapfft_audio*)malloc( sizeof(ci_wrapfft_audio) );
    if (!h)
    {
        return NULL;
    }
    memset( h, 0, sizeof(ci_wrapfft_audio) );
    
      
    h->frame_size         = 800;//frame_size;
    h->frame_shift        = 320;//FRAME_SHIFT; 
    h->frame_overlap_size = 480;//frame_size - FRAME_SHIFT;
    h->fft_size           = 512;//FFT_SIZE+1;//FFT_SIZE+1 = 257

#if USE_SIN_WINDOW

#if USE_HALF_FRAME_WIN
    h->window = (float*)malloc( frame_size/2 * sizeof(float) );
  	sin_window( h->window, h->frame_size );
#else
    h->window = (float*)malloc( frame_size * sizeof(float) );
	sin_window( h->window, h->frame_size );
#endif

#else
#if USE_HALF_FRAME_WIN//使用半长的窗
    h->fix_window = (short*)malloc( frame_size/2 * sizeof(short) );
    hanmming_window_fixedpoint(h->fix_window,h->frame_size);
#else
    h->fix_window = (short*)malloc( frame_size * sizeof(short) );
    hanmming_window_fixedpoint(h->fix_window,h->frame_size);
#endif

#endif

    h->psd= (float*)malloc( (h->fft_size/2+1) * sizeof(float) );
    memset(h->psd, 0, (h->fft_size/2+1)  * sizeof(float) );
#if !USE_FFT_FUN
    ret = ci_software_fft_w1024_s512_init();
#endif
    
#if USE_FFT_FUN
        h->tmp_buf = (float*)malloc( FRM_FFT_LENTH * sizeof(float) );
        h->fft_tmp_buf = (float*)malloc( FRM_FFT_LENTH * 2  *  sizeof(float) );
         if ((!h->tmp_buf)||(!h->fft_tmp_buf))
        {
            return NULL;
        }
        memset( h->tmp_buf, 0, FRM_FFT_LENTH * sizeof(float) ); 
        memset(h->fft_tmp_buf, 0, FRM_FFT_LENTH * 2  * sizeof(float) );   
#endif    
    //根据通道数确认fft中需要用到的buffer
    h->mic_channels       = mic_channel_num;
    h->ref_channels       = ref_channel_num;
    h->num_channels       = h->mic_channels+h->ref_channels;
   
    for ( i = 0; i < h->mic_channels; i++ )//num_channels,目前通道数最多支持6个通道
    {
        h->frame_mic_data[i] = (short*)malloc( h->frame_size * sizeof(short) );
        h->fft_mic_out[i] = (float*)malloc( h->fft_size * 2  *  sizeof(float) );
        if ((!h->fft_mic_out[i])||(!h->frame_mic_data[i]))
        {
            return NULL;
        }
        memset( h->frame_mic_data[i], 0, frame_size * sizeof(short) ); 
        memset(h->fft_mic_out[i], 0, h->fft_size * 2  * sizeof(float) );   
    }
    
    for ( i = 0; i < h->ref_channels; i++ )//num_channels,目前通道数最多支持6个通道
    {
        h->frame_ref_data[i] = (short*)malloc( frame_size * sizeof(short) );
        h->fft_ref_out[i] = (float*)malloc( h->fft_size * 2  *  sizeof(float) );
        if ((!h->fft_ref_out[i])||(!h->frame_ref_data[i]))
        {
            return NULL;
        }
        memset( h->frame_ref_data[i], 0, frame_size * sizeof(short) ); 
        memset(h->fft_ref_out[i], 0, h->fft_size * 2  * sizeof(float) );   
    }
#if USE_IFFT//ifft_malloc


    // ifft需要的buffer
    h->ifft_result_buf = (float*)malloc( h->fft_size * 2 * sizeof(float) );
    h->ifft_tmp_buf    = (float*)malloc( h->frame_shift * sizeof(float) );
    
    if (!h->ifft_result_buf || !h->ifft_tmp_buf )
    {
        return NULL;
    }
      
    memset( h->ifft_result_buf, 0, h->fft_size * 2 * sizeof(float) );
    memset( h->ifft_tmp_buf, 0, h->frame_shift * sizeof(float) );
	
#if (FRAME_SHIFT != FFT_SIZE)  
    h->adjust_gain = (float*)malloc( h->frame_shift * sizeof(float) );
    if (!h->adjust_gain)
    {
        return NULL;
    }
#if USE_HALF_FRAME_WIN//半长窗
    int first_index =  h->frame_size/2-(h->frame_size -  2*h->frame_shift);
    int first_cur_start_index =h->frame_overlap_size - h->frame_shift;;//全长窗
    float scale = 1.0f/32767.0f;
    
    for ( i = 0; i <first_index; i++ )
    {      
    	#if USE_SIN_WINDOW
		h->adjust_gain[i] = 1.0f / (h->window[i+first_cur_start_index]*h->window[i+first_cur_start_index] +
			h->window[h->frame_shift-1-i]*h->window[h->frame_shift-1-i]);
		#else
        h->adjust_gain[i] = 1.0f / (scale*h->fix_window[i+first_cur_start_index]*scale*h->fix_window[i+first_cur_start_index] +
			scale*h->fix_window[h->frame_shift-1-i]*scale*h->fix_window[h->frame_shift-1-i]);    
		#endif
    }
    for ( i = first_index ; i < h->frame_shift; i++ )
    {      
        int j = i - first_index;
		#if USE_SIN_WINDOW
		h->adjust_gain[i] = 1.0f / (h->window[h->frame_size/2-1-j]*h->window[h->frame_size/2-1-j] +
			h->window[h->frame_size/2-h->frame_shift-1-j]*h->window[h->frame_size/2-h->frame_shift-1-j]);
		#else
        h->adjust_gain[i] = 1.0f / (scale*h->fix_window[h->frame_size/2-1-j]*scale*h->fix_window[h->frame_size/2-1-j] +
			scale*h->fix_window[h->frame_size/2-h->frame_shift-1-j]*scale*h->fix_window[h->frame_size/2-h->frame_shift-1-j]);
		#endif
    }
#else  //全长窗
    int cur_start_index =  h->frame_overlap_size - h->frame_shift;
    int last_start_index = h->frame_size-h->frame_shift;
    
    int first_index =  h->frame_size/2-(h->frame_size -  2*h->frame_shift);
    float scale = 1.0f/32767.0f;
    for ( i = 0; i < h->frame_shift; i++ )
    {      
    	#if USE_SIN_WINDOW
		h->adjust_gain[i] = 1.0f / (h->window[i+cur_start_index]*h->window[i+cur_start_index] +
        		h->window[i+last_start_index]*h->window[i+last_start_index]);
		#else
        h->adjust_gain[i] = 1.0f / (h->fix_window[i+cur_start_index]*scale*h->fix_window[i+cur_start_index]*scale +
        		h->fix_window[i+last_start_index]*scale*h->fix_window[i+last_start_index]*scale);
		#endif
    }
     
#endif                        
#endif

#endif 
    
    return h;
}

extern int ci_software_fft_w1024_s512( const short *audio_data, const float *window_data, float *result );

static int ci_wrapfft_fft( void *handle, short *pcm, short *frame_date,float *fft)
{
    int ret  = 0;
    int i    = 0;
    short *x = NULL;
    
    ci_wrapfft_audio *h = (ci_wrapfft_audio *)handle;
	#if 0
    if ( !h || !pcm || !fft )
    {
        return -__LINE__;
    }
	#endif
    
    /*预加重*/

    x = frame_date;

    memcpy( x , x+h->frame_shift ,h->frame_overlap_size * sizeof(short) );
    memcpy( x+h->frame_overlap_size, pcm , h->frame_shift * sizeof(short) );

	ret = ci_software_fft_w1024_s512( x, (float*)h->fix_window, fft );
	short psd_index = (h->fft_size/2+1);

	fft[1] = 0.0f;

	for(int i = 0 ;i < psd_index ;i++)
	{
			h->psd[i] =  fft[2*i]*fft[2*i] + fft[2*i+1]*fft[2*i+1];
	}

#if HPF_ENABLE//高通滤波
     
    for(int i = 0;i<HPF_CUT_OFF_FREQ;i++)
    {
       fft[2*i] =  0;
       fft[2*i+1] =  0;
    }
    
    for(int i = 256;i<512;i++)
    {
       fft[2*i] =  0;
       fft[2*i+1] =  0;
    }
    
#endif
  
    if(ret != 0)
    {
        return -__LINE__;
    }
    
    return 0;
}

extern int ci_software_ifft_w1024_s512(const float *fft, float *result);


int ci_wrapfft_ifft( void *handle,  float *fft,short *pcm_out )
{
    int ret = 0;
    int i = 0;
    float temp = 0.0f;
    ci_wrapfft_audio *h = (ci_wrapfft_audio *)handle;
    if (!h || !pcm_out || !fft)
    {
        //return -__LINE__;
    }
#if !USE_FFT_FUN
    if (h->enable_hardware_fft)
    {
       //ret = ci_hardware_ifft_w512_s256( fft, h->ifft_result_buf);
    }
    else
    {
    #if   (512 == FRM_FFT_LENTH)
           ret = ci_software_ifft_w512_s256( fft ,h->ifft_result_buf);
	#elif (1024 == FRM_FFT_LENTH)
		   ret = ci_software_ifft_w1024_s512( fft ,h->ifft_result_buf);
	#endif
    }
#else
    for ( i=0;i<FRM_FFT_LENTH/2;i++)
    {                   
        h->fft_tmp_buf[2*i+0] = fft[2*i+0];
        h->fft_tmp_buf[2*i+1] = fft[2*i+1];
        if(i!=0||i!=FRM_FFT_LENTH)
        {
            h->fft_tmp_buf[FRM_FFT_LENTH*2 -2*i] = fft[2*i];
            h->fft_tmp_buf[FRM_FFT_LENTH*2+1-2*i] = -fft[2*i+1];
        }
    } 
     //快速傅里叶反变换
   fft_fun(h->fft_tmp_buf,FRM_FFT_LENTH,-1);
	 
   for ( i=0;i<FRM_FFT_LENTH;i++)
    {                   
       h->ifft_result_buf[i] = h->fft_tmp_buf[2*i]/FRM_FFT_LENTH;
    } 
    
#endif
    
    if (ret != 0)
    {
        return -__LINE__;
    }


#   if (FRAME_SHIFT == FFT_SIZE)
   float scale = 1.0f/32767.0f; 
   for ( int i = 0; i < FRAME_SHIFT; i++ )//h->frame_overlap_size
    {
    	#if USE_SIN_WINDOW
		temp = (h->ifft_result_buf[i] * h->window[i] + h->ifft_tmp_buf[i]);
		#else
        temp = (h->ifft_result_buf[i] * scale * h->fix_window[i] + h->ifft_tmp_buf[i]);
        #endif
        int tt = (int)temp;
        
        if(tt > 32767 )
        {
            pcm_out[i] = 32767;
        }
        else if(tt < -32768)
        {
            pcm_out[i] = -32768;
        }
        else
        {
            pcm_out[i] = (short)tt;         
        }
		#if USE_SIN_WINDOW
        h->ifft_tmp_buf[i] = ( (h->ifft_result_buf[i+FRAME_SHIFT] ) * h->window[i+FRAME_SHIFT]);//h->frame_shift
        #else
		h->ifft_tmp_buf[i] = ( (h->ifft_result_buf[i+FRAME_SHIFT] ) * scale * h->fix_window[i+FRAME_SHIFT]);//h->frame_shift
        #endif
    }         
#   else
#if !USE_HALF_FRAME_WIN
    short pcm_out_start_index = h->frame_overlap_size-h->frame_shift;
    float scale = 1.0f/32767.0f;
    for (i = 0; i < h->frame_shift; i++) 
    {
       #if  USE_SIN_WINDOW
       temp = (h->ifft_result_buf[i+pcm_out_start_index]*h->window[i + pcm_out_start_index] + h->ifft_tmp_buf[i]) * h->adjust_gain[i];
	   #else	
	   temp = (h->ifft_result_buf[i+pcm_out_start_index]*h->fix_window[i + pcm_out_start_index]* scale+
    			 h->ifft_tmp_buf[i]) * h->adjust_gain[i];
	   #endif
       
        int tt = (int)temp; 
        if(tt > 32767 )
        {
            pcm_out[i] = 32767;
        }
        else if(tt < -32768)
        {
            pcm_out[i] = -32768;
        }
        else
        {
            pcm_out[i] = (short)tt;         
        }
		#if  USE_SIN_WINDOW
		h->ifft_tmp_buf[i] = (h->ifft_result_buf[i+h->frame_overlap_size] * h->window[h->frame_shift - 1-i]);
		#else
        h->ifft_tmp_buf[i] = (h->ifft_result_buf[i+h->frame_overlap_size] * h->fix_window[h->frame_shift - 1-i]* scale);
		#endif
                         
    }
#else
    short pcm_out_start_index = h->frame_overlap_size-h->frame_shift;
   
    short first_num = h->frame_size/2-(h->frame_size - 2*h->frame_shift);
    float scale = 1.0f/32767.0f;
    for (i = 0; i < h->frame_shift; i++) 
    {
        if(i<first_num)//120
        {
        #if  USE_SIN_WINDOW	
          temp = (h->ifft_result_buf[i+pcm_out_start_index]*h->window[i + pcm_out_start_index] + h->ifft_tmp_buf[i]) * h->adjust_gain[i];
		#else
		  temp = (h->ifft_result_buf[i+pcm_out_start_index]*h->fix_window[i + pcm_out_start_index]*scale + h->ifft_tmp_buf[i]) * h->adjust_gain[i];
		#endif
        }
        else
        {
        	#if  USE_SIN_WINDOW	
          	temp = (h->ifft_result_buf[i+pcm_out_start_index]*h->window[ h->frame_size/2 - 1 - first_num +i] + h->ifft_tmp_buf[i]) * h->adjust_gain[i];
      		#else
			temp = (h->ifft_result_buf[i+pcm_out_start_index]*h->fix_window[ h->frame_size/2 - 1 - first_num +i]*scale + h->ifft_tmp_buf[i]) * h->adjust_gain[i];
	  		#endif
        }
        int tt = (int)temp; 
        if(tt > 32767 )
        {
            pcm_out[i] = 32767;
        }
        else if(tt < -32768)
        {
            pcm_out[i] = -32768;
        }
        else
        {
            pcm_out[i] = (short)tt;         
        }
       
	  
        //h->ifft_tmp_buf[i] = (h->ifft_result_buf[i+h->frame_overlap_size] * h->window[i+h->frame_overlap_size]); 
        #if  USE_SIN_WINDOW	
        h->ifft_tmp_buf[i] = (h->ifft_result_buf[i+h->frame_overlap_size] * h->window[h->frame_shift - 1-i]); 
        #else
		h->ifft_tmp_buf[i] = (h->ifft_result_buf[i+h->frame_overlap_size] * h->fix_window[h->frame_shift - 1-i]*scale); 
		#endif
		
		
    }
#endif

#   endif
     
    return 0;
}

int ci_fft_deal(void *handle)
{
    int ret = 0;
    ci_wrapfft_audio *st = (ci_wrapfft_audio*)handle;
      
   //mic_in_mic_fft_out
    for (int i = 0; i < st->mic_channels; i++ )
    {
      
      ret = ci_wrapfft_fft( st, st->mic[i], st->frame_mic_data[i],st->fft_mic_out[i]);
      
        if ( ret != 0 )
        {
            return -__LINE__;
        }
    } 
   
    
    
    //ref_in_mic_fft_out
    for (int i = 0; i < st->ref_channels; i++ )
    {
         ret = ci_wrapfft_fft( st, st->ref[i], st->frame_ref_data[i],st->fft_ref_out[i]);
        if ( ret != 0 )
        {
            return -__LINE__;
        }
    } 
   
    return ret;
}


int ci_warpfft_free( void *handle)
{
    /*int i = 0;
    int ret = 0;
    ci_wrapfft_audio *h = (ci_wrapfft_audio*)handle;
    if (!h)
        goto error_exit; 

    // window for fft
    if ( h->window )
        ci_algbuf_free( h->window );
    
    if (h->enable_hardware_fft)
    {
       ret = ci_hardware_fft_w512_s256_uninit();
        if (h->fft_window)
            ci_algbuf_free( h->fft_window );
    }
    else
    {
       ret = ci_software_fft_w512_s256_uninit();
    }

    // fft 
    
    for ( i = 0; i < h->mic_channels; i++ )//mic_channels,目前通道数最多支持4个通道
    {
         if (h->fft_mic_out[i]&&h->frame_mic_data[i])
         {
            ci_algbuf_free( h->frame_mic_data[i] );
            ci_algbuf_free( h->fft_mic_out[i] );
            
         }
    }
    for ( i = 0; i < h->ref_channels; i++ )//ref_channels,目前通道数最多支持4个通道
    {
         if (h->fft_ref_out[i]&&h->frame_ref_data[i])
         {
            ci_algbuf_free( h->frame_ref_data[i] );   
            ci_algbuf_free( h->fft_ref_out[i] );   
         }
    }
    
    // ifft  
    if (h->ifft_result_buf)
        ci_algbuf_free( h->ifft_result_buf );
    if (h->ifft_tmp_buf)
        ci_algbuf_free( h->ifft_tmp_buf );

#if (FRAME_SHIFT != FFT_SIZE)
    if (h->adjust_gain)
        ci_algbuf_free( h->adjust_gain );
#endif

    ci_algbuf_free( h );
error_exit:*/
    return 0;
  
}





