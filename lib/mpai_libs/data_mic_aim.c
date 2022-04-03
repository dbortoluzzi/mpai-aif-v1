#include "data_mic_aim.h"

LOG_MODULE_REGISTER(MPAI_LIBS_DATA_MIC_AIM, LOG_LEVEL_INF);

/*************** DEFINE ***************/

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 7

/* Define The transmission interval [mSec] for Microphones dB Values */
#define MICS_DB_UPDATE_MS 50

#define SaturaLH(N, L, H) (((N)<(L))?(L):(((N)>(H))?(H):(N)))    

#define PRINT_WAV_ENABLED false
#define PUBLISH_BUFFER_ENABLED false

/*************** STATIC ***************/
static size_t TARGET_AUDIO_BUFFER_NB_SAMPLES = AUDIO_SAMPLING_FREQUENCY * 2;
#if PUBLISH_BUFFER_ENABLED == true || PRINT_WAV_ENABLED == true
    static int16_t *TARGET_AUDIO_BUFFER;
#endif
static size_t TARGET_AUDIO_BUFFER_IX = 0;
typedef struct {
  int32_t Z;
  int32_t oldOut;
  int32_t oldIn;
} HP_FilterState_TypeDef;
static HP_FilterState_TypeDef HP_Filter;

static uint16_t PCM_Buffer[PCM_BUFFER_LEN / 2];
static BSP_AUDIO_Init_t MicParams;

// we skip the first 50 events (100 ms.) to not record the button click
static size_t SKIP_FIRST_EVENTS = 50;
static size_t half_transfer_events = 0;
static size_t transfer_complete_events = 0;

static const struct device *led0;

void static start_recording() {

    int32_t ret;
    uint32_t state;

    ret = BSP_AUDIO_IN_GetState(AUDIO_INSTANCE, &state);
    if (ret != BSP_ERROR_NONE) {
        printk("Cannot start recording: Error getting audio state (%d)\n", ret);
        return;
    }
    if (state == AUDIO_IN_STATE_RECORDING) {
        printk("Cannot start recording: Already recording\n");
        return;
    }

    // reset audio buffer location
    TARGET_AUDIO_BUFFER_IX = 0;
    transfer_complete_events = 0;
    half_transfer_events = 0;

    ret = BSP_AUDIO_IN_Record(AUDIO_INSTANCE, (uint8_t *) PCM_Buffer, PCM_BUFFER_LEN);
    if (ret != BSP_ERROR_NONE) {
        printk("Error Audio Record (%ld)\n", ret);
        return;
    }
    else {
        printk("OK Audio Record\n");
    }

	LOG_DBG("Producing......\n\n");
}

#if PRINT_WAV_ENABLED == true
void static print_wav() 
{
	// create WAV file
    size_t wavFreq =  06*16000; /*AUDIO_SAMPLING_FREQUENCY*/
    size_t dataSize = (TARGET_AUDIO_BUFFER_NB_SAMPLES * 2);
    size_t fileSize = 44 + (TARGET_AUDIO_BUFFER_NB_SAMPLES * 2);

    uint8_t wav_header[44] = {
        0x52, 0x49, 0x46, 0x46, // RIFF
        fileSize & 0xff, (fileSize >> 8) & 0xff, (fileSize >> 16) & 0xff, (fileSize >> 24) & 0xff,
        0x57, 0x41, 0x56, 0x45, // WAVE
        0x66, 0x6d, 0x74, 0x20, // fmt
        0x10, 0x00, 0x00, 0x00, // length of format data
        0x01, 0x00, // type of format (1=PCM)
        0x01, 0x00, // number of channels
        wavFreq & 0xff, (wavFreq >> 8) & 0xff, (wavFreq >> 16) & 0xff, (wavFreq >> 24) & 0xff,
        0x00, 0x7d, 0x00, 0x00, // 	(Sample Rate * BitsPerSample * Channels) / 8
        0x02, 0x00, 0x10, 0x00,
        0x64, 0x61, 0x74, 0x61, // data
        dataSize & 0xff, (dataSize >> 8) & 0xff, (dataSize >> 16) & 0xff, (dataSize >> 24) & 0xff,
    };

    printk("Total complete events: %lu, index is %lu\n", transfer_complete_events, TARGET_AUDIO_BUFFER_IX);

    // print both the WAV header and the audio buffer in HEX format to serial
    // you can use the script in `hex-to-buffer.js` to make a proper WAV file again
    printk("WAV file:\n");
    for (size_t ix = 0; ix < 44; ix++) {
        printk("%02x", wav_header[ix]);
    }

	for (size_t iy=0; iy<(TARGET_AUDIO_BUFFER_IX*2)/64; iy++) {
		char logstring[64*2] = {};
		for (size_t ix = 0; ix < 64; ix++) {
			sprintf(logstring+2*ix, "%02x", ((uint8_t *)TARGET_AUDIO_BUFFER)[64*iy+ix]);
		}
		printk(logstring);
		k_sleep(K_MSEC(50));
	}
    printk("\n");
}
#endif

void static print_stats() {
    printk("Half %lu, Complete %lu, IX %lu\n", half_transfer_events, transfer_complete_events,
        TARGET_AUDIO_BUFFER_IX);
}

/**************** THREADS **********************/

static k_tid_t producer_mic_thread_id;

K_THREAD_STACK_DEFINE(thread_prod_mic_stack_area, STACKSIZE);
static struct k_thread thread_prod_mic_data;

volatile float RMS_Ch[AUDIO_CHANNELS];
float DBNOISE_Value_Old_Ch[AUDIO_CHANNELS];
uint16_t       DBNOISE_Value_Ch[AUDIO_CHANNELS];

// callback that gets invoked when TARGET_AUDIO_BUFFER is full
void target_audio_buffer_full() {
    // pause audio stream
    int32_t ret = BSP_AUDIO_IN_Pause(AUDIO_INSTANCE);
    if (ret != BSP_ERROR_NONE) {
        printk("Error Audio Pause (%d)\n", ret);
    }
    else {
        printk("OK Audio Pause\n");
    }

	publish_buffer_to_message_store();
}

static int64_t last_peak_recognized = 0;
static bool flag_peak_recognized = false;

/**
  * @brief  Send Audio Level Data (Ch1) to BLE
  * @param  None
  * @retval None
  */
static void SendAudioLevelData(void)
{
  int32_t NumberMic;
  int32_t DBNOISE_Value_Ch[AUDIO_CHANNELS];

  for(NumberMic=0;NumberMic<(AUDIO_CHANNELS);NumberMic++) {
    DBNOISE_Value_Ch[NumberMic] = 0;

    RMS_Ch[NumberMic] /= (16.0f*MICS_DB_UPDATE_MS);

    int32_t calc_median = 0;
    int32_t calc_peak = 0;
    median_filter((int32_t) RMS_Ch[NumberMic], &calc_median, &calc_peak);

    if (calc_peak >= 10000000 && calc_peak < 15000000 && 0.00006 >= (float)calc_median/calc_peak) {

        // int64_t now = k_uptime_get();
        // printk("PICCO RILEVATO %lld\n", now);  
        // gpio_pin_set(led0, DT_GPIO_PIN(DT_ALIAS(led0), gpios), 1);    

        if (flag_peak_recognized == true)
        {
            // gpio_pin_set(led0, DT_GPIO_PIN(DT_ALIAS(led0), gpios), 1);    
        } else 
        {
            // gpio_pin_set(led0, DT_GPIO_PIN(DT_ALIAS(led0), gpios), 1);    
            int64_t now = k_uptime_get();
            LOG_INF("PICCO RILEVATO %d: %lld\n", calc_peak, now);  
            flag_peak_recognized = true;

            publish_peak_to_message_store(calc_peak);
        }

    } else {
        // gpio_pin_set(led0, DT_GPIO_PIN(DT_ALIAS(led0), gpios), 0);
        flag_peak_recognized = false;
    }

    // DBNOISE_Value_Ch[NumberMic] = (uint16_t)((120.0f - 20 * log10f(32768 * (1 + 0.25f * (AUDIO_VOLUME_VALUE /*AudioInVolume*/ - 4))) + 10.0f * log10f(RMS_Ch[NumberMic])) * 0.3f + DBNOISE_Value_Old_Ch[NumberMic] * 0.7f);
    // DBNOISE_Value_Old_Ch[NumberMic] = DBNOISE_Value_Ch[NumberMic];
    RMS_Ch[NumberMic] = 0.0f;
    // printk("Volume at %lld: %d\n",k_uptime_get(), DBNOISE_Value_Ch[NumberMic]&0xFF);
  }
  
}

/**
 * 
* @brief  User function that is called when 1s ms of PDM data is available.
* @param  none
* @retval None
*/
void AudioProcess_DB_Noise(void)
{
  int32_t i;
  int32_t NumberMic;

  uint32_t buffer_size = PCM_BUFFER_LEN / 2; /* Half Transfer */
  uint32_t nb_samples = buffer_size / sizeof(int16_t); /* Bytes to Length */

  for(i = 0; i < 16; i++){
    for(NumberMic=0;NumberMic<AUDIO_CHANNELS;NumberMic++) {
      RMS_Ch[NumberMic] += (float)((int16_t)PCM_Buffer[i*AUDIO_CHANNELS+NumberMic] * ((int16_t)PCM_Buffer[i*AUDIO_CHANNELS+NumberMic]));
    }
  }
  SendAudioLevelData();
}

/**
* @brief  Half Transfer user callback, called by BSP functions.
* @param  None
* @retval None
*/
void BSP_AUDIO_IN_HalfTransfer_CallBack(uint32_t Instance) {
    half_transfer_events++;
    if (half_transfer_events < SKIP_FIRST_EVENTS) return;

    uint32_t buffer_size = PCM_BUFFER_LEN / 2; /* Half Transfer */
    uint32_t nb_samples = buffer_size / sizeof(int16_t); /* Bytes to Length */

#if PUBLISH_BUFFER_ENABLED == true || PRINT_WAV_ENABLED == true
    if ((TARGET_AUDIO_BUFFER_IX + nb_samples) > TARGET_AUDIO_BUFFER_NB_SAMPLES) {
        return;
    }
    /* Copy first half of PCM_Buffer from Microphones onto Fill_Buffer */
    memcpy(((uint8_t*)TARGET_AUDIO_BUFFER) + (TARGET_AUDIO_BUFFER_IX * 2), PCM_Buffer, buffer_size);
    TARGET_AUDIO_BUFFER_IX += nb_samples;

    if (TARGET_AUDIO_BUFFER_IX >= TARGET_AUDIO_BUFFER_NB_SAMPLES) {
        target_audio_buffer_full();
        return;
    }
#endif

    /* High-Pass filter to remove DC component and/or low frequency noise */
    for (uint32_t i = 0; i < nb_samples; i++)
    {
        HP_Filter.Z = (int32_t) PCM_Buffer[i];
        HP_Filter.oldOut = (0xFC * (HP_Filter.oldOut + HP_Filter.Z - HP_Filter.oldIn)) / 256;
        HP_Filter.oldIn = HP_Filter.Z;
        PCM_Buffer[i] = (uint16_t) SaturaLH(HP_Filter.oldOut, -32768, 32767);
    }

    AudioProcess_DB_Noise();
}

/**
* @brief  Transfer Complete user callback, called by BSP functions.
* @param  None
* @retval None
*/
void BSP_AUDIO_IN_TransferComplete_CallBack(uint32_t Instance) {
    transfer_complete_events++;
    if (transfer_complete_events < SKIP_FIRST_EVENTS) return;

    uint32_t buffer_size = PCM_BUFFER_LEN / 2; /* Half Transfer */
    uint32_t nb_samples = buffer_size / sizeof(int16_t); /* Bytes to Length */

#if PUBLISH_BUFFER_ENABLED == true || PRINT_WAV_ENABLED == true
    if ((TARGET_AUDIO_BUFFER_IX + nb_samples) > TARGET_AUDIO_BUFFER_NB_SAMPLES) {
        return;
    }

    /* Copy second half of PCM_Buffer from Microphones onto Fill_Buffer */
    memcpy((uint8_t*)TARGET_AUDIO_BUFFER + (TARGET_AUDIO_BUFFER_IX * 2),
       ((uint8_t*)PCM_Buffer) + (nb_samples * 2), buffer_size);
    TARGET_AUDIO_BUFFER_IX += nb_samples;

    if (TARGET_AUDIO_BUFFER_IX >= TARGET_AUDIO_BUFFER_NB_SAMPLES) {
		target_audio_buffer_full();
        return;
    }
#endif

    /* High-Pass filter to remove DC component and/or low frequency noise */
    for (uint32_t i = 0; i < nb_samples; i++)
    {
        HP_Filter.Z = (int32_t) PCM_Buffer[i + nb_samples];
        HP_Filter.oldOut = (0xFC * (HP_Filter.oldOut + HP_Filter.Z - HP_Filter.oldIn)) / 256;
        HP_Filter.oldIn = HP_Filter.Z;
        PCM_Buffer[i + nb_samples] = (uint16_t) SaturaLH(HP_Filter.oldOut, -32768, 32767);
    }

    AudioProcess_DB_Noise();
}

/**
  * @brief  Manages the BSP audio in error event.
  * @param  Instance Audio in instance.
  * @retval None.
  */
void BSP_AUDIO_IN_Error_CallBack(uint32_t Instance) {
    printk("BSP_AUDIO_IN_Error_CallBack\n");
}

/* PRODUCER */
void publish_buffer_to_message_store()
{
    #if PUBLISH_BUFFER_ENABLED == true
        mic_data_t *mic_data = (mic_data_t *)k_malloc(sizeof(mic_data_t));
        mic_data->size = (size_t *)k_malloc(sizeof(size_t));
        mic_data->data = (int16_t *)k_malloc(sizeof(int16_t));
        memcpy(mic_data->size, TARGET_AUDIO_BUFFER_NB_SAMPLES, sizeof(TARGET_AUDIO_BUFFER_NB_SAMPLES));
        memcpy(mic_data->data, TARGET_AUDIO_BUFFER, sizeof(TARGET_AUDIO_BUFFER));
        
        // Publish sensor message 
        mpai_parser_t msg = {
            .data = mic_data,
            .timestamp = k_uptime_get()
        };

        MPAI_MessageStore_publish(message_store_test_case_aiw, &msg, MIC_BUFFER_DATA_CHANNEL);

        free(mic_data);

        LOG_DBG("Message mic data published");

    #endif
}

void publish_peak_to_message_store(int64_t peak_value)
{
    mic_peak_t *mic_peak = (mic_peak_t *)k_malloc(sizeof(mic_peak_t));
    mic_peak->data = (int32_t *)k_malloc(sizeof(int32_t));
    memcpy(mic_peak->data, &peak_value, sizeof(peak_value));
    
    // Publish sensor message 
    mpai_parser_t msg = {
        .data = mic_peak,
        .timestamp = k_uptime_get()
    };

    MPAI_MessageStore_publish(message_store_data_mic_aim, &msg, MIC_PEAK_DATA_CHANNEL);

    free(mic_peak);

    LOG_DBG("Message peak published");

}

void th_produce_data_mic_data(void *dummy1, void *dummy2, void *dummy3)
{

	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

    #if PUBLISH_BUFFER_ENABLED == true || PRINT_WAV_ENABLED == true
        TARGET_AUDIO_BUFFER = (int16_t*)k_calloc(TARGET_AUDIO_BUFFER_NB_SAMPLES, sizeof(int16_t));
        if (!TARGET_AUDIO_BUFFER) {
           printk("Failed to allocate TARGET_AUDIO_BUFFER buffer\n");
           return 0;
        }
    #endif

    // set up the microphone
    MicParams.BitsPerSample = AUDIO_RESOLUTION_16b; // AUDIO_RESOLUTION_16b
    MicParams.ChannelsNbr = AUDIO_CHANNELS;
    MicParams.Device = AUDIO_IN_DIGITAL_MIC1; 
    MicParams.SampleRate =  AUDIO_SAMPLING_FREQUENCY;
    MicParams.Volume = AUDIO_VOLUME_VALUE;

    int32_t ret = BSP_AUDIO_IN_Init(AUDIO_INSTANCE, &MicParams);

    if (ret != BSP_ERROR_NONE) {
        printk("Error Audio Init (%ld)\r\n", ret);
        return 1;
    } else {
        printk("OK Audio Init\t(Audio Freq=%ld)\r\n", AUDIO_SAMPLING_FREQUENCY);
    }

	start_recording();


    #if PRINT_WAV_ENABLED == true
	    // k_sleep(K_MSEC(5000));

	    print_wav();
    #endif

}

/************** EXECUTIONS ***************/
mpai_error_t* data_mic_aim_subscriber()
{
	// NO-OP
}

mpai_error_t* data_mic_aim_start()
{
	// LED
	led0 = device_get_binding(DT_GPIO_LABEL(DT_ALIAS(led0), gpios));
	gpio_pin_configure(led0, DT_GPIO_PIN(DT_ALIAS(led0), gpios),
					   GPIO_OUTPUT_ACTIVE |
						   DT_GPIO_FLAGS(DT_ALIAS(led0), gpios));
    
	// CREATE PRODUCER
	producer_mic_thread_id = k_thread_create(&thread_prod_mic_data, thread_prod_mic_stack_area,
			K_THREAD_STACK_SIZEOF(thread_prod_mic_stack_area),
			th_produce_data_mic_data, NULL, NULL, NULL,
			PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&thread_prod_mic_data, "thread_prod");
	
	// START THREAD
	k_thread_start(producer_mic_thread_id);

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return &err;
}

mpai_error_t* data_mic_aim_stop() 
{
	k_thread_abort(producer_mic_thread_id);
	LOG_INF("Execution stopped");

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return &err;
}

mpai_error_t* data_mic_aim_resume() 
{
	k_thread_resume(producer_mic_thread_id);
	LOG_INF("Execution resumed");

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return &err;
}

mpai_error_t* data_mic_aim_pause() 
{
	k_thread_suspend(producer_mic_thread_id);
	LOG_INF("Execution paused");

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return &err;
}