#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include "system.h"
//#include "periphs.h"
//#include "iob-uart.h"
#include "printf.h"

#include <twolame.h>
#include "audio_wave.h"

#define MP2BUFSIZE      (16384)
#define AUDIOBUFSIZE    (9216)

// set pointer to DDR base
/*#if (RUN_EXTMEM==0)  //running firmware from SRAM
  #define DATA_BASE_ADDR (EXTRA_BASE)
#else //running firmware from DDR
  #define DATA_BASE_ADDR ((1<<(FIRM_ADDR_W)))
#endif*/

int main()
{

    //init uart
    //uart_init(UART_BASE,FREQ/BAUD);

    char *recvfile = NULL;
    char *outfile = NULL;
    char *prev_outfile = NULL;
    int recv_file_size = 0;
    //int send_file_size = 0;
    //char filepath[] = "../../../sine.wav"; // valid path for sim-run

    //printf("\n LETS GOOOO\n");

    //uart_puts ("Waiting to receive wave file...\n");

    //file receive
    
    //recvfile = (char*) DATA_BASE_ADDR;
    //recv_file_size = uart_recvfile(filepath, recvfile);
    FILE *file = NULL;
    if ((file = fopen("sine.wav", "rb")) == NULL) {
        printf("\nWAV: cannot open input file\n");
        return 1;
    }
    //printf("\n STEP 10\n");
    if ((recvfile = (char *) calloc(220, sizeof(char))) == NULL) {
        fprintf(stderr, "mp2buffer alloc failed\n");
        exit(99);
    }
    recv_file_size = fread(recvfile, sizeof(char), 220, file);
    //printf("\n STEP 1111\n");
    fclose(file);
    //printf("\n STEP 11\n");
    
   
    //twolame encoding
    
    twolame_options *encodeOptions;
    //char *inputfilename = argv[1];
    //char *outputfilename = argv[2];
    //FILE *outfile;
    //short int *pcmaudio;
    //unsigned char *mp2buffer;
    int num_samples = 0;
    int mp2fill_size = 0;
    int frames = 0;
    wave_info_t *wave_info = NULL;


    /* grab a set of default encode options */
    encodeOptions = twolame_init();
    printf("Using libtwolame version %s.\n", get_twolame_version());

    /* Read the wave file */
    if ((wave_info = wave_init(recvfile)) == NULL) {
        fprintf(stderr, "Not a recognised WAV file.\n");
        exit(99);
    }
    // Use sound file to over-ride preferences for
    // mono/stereo and sampling-frequency
    twolame_set_num_channels(encodeOptions, wave_info->channels);
    if (wave_info->channels == 1) {
        twolame_set_mode(encodeOptions, TWOLAME_MONO);
    } else {
        twolame_set_mode(encodeOptions, TWOLAME_STEREO);
    }


    /* Set the input and output sample rate to the same */
    twolame_set_in_samplerate(encodeOptions, wave_info->samplerate);
    twolame_set_out_samplerate(encodeOptions, wave_info->samplerate);

    /* Set the bitrate to 192 kbps */
    twolame_set_bitrate(encodeOptions, 192);

    /* initialise twolame with this set of options */
    if (twolame_init_params(encodeOptions) != 0) {
        fprintf(stderr, "Error: configuring libtwolame encoder failed.\n");
        exit(99);
    }

    /* Open the output file for the encoded MP2 data */
    //outfile = (char*) (DATA_BASE_ADDR + recv_file_size);
    
    if ((outfile = (char *) calloc(630, sizeof(char))) == NULL) {
        fprintf(stderr, "mp2buffer alloc failed\n");
        exit(99);
    }
    prev_outfile = outfile;

    int unread_data = recv_file_size;
    short int *soundfile = wave_info->soundfile;

    FILE * only_test = NULL;

    //printf("\n STEP 0\n");

     /* Open the output file for the encoded MP2 data */
    if ((only_test = fopen("test.mp2", "wb")) == 0) {
        fprintf(stderr, "error opening output file\n");
        exit(99);
    }

    //printf("\n STEP 1\n");

    // Read num_samples of audio data *per channel* from the input file
    while ((num_samples = wave_get_samples(wave_info, soundfile,  &unread_data, AUDIOBUFSIZE)) != 0) {

        //printf("\n STEP 2\n");

        // Encode the audio!
        mp2fill_size =
            twolame_encode_buffer_interleaved(encodeOptions, soundfile, num_samples, (unsigned char *) outfile,
                                              MP2BUFSIZE);

        //printf("\n STEP 3\n");

        // Write the MPEG bitstream to the file
        fwrite(outfile, sizeof(unsigned char), mp2fill_size, only_test);

        //printf("\n STEP 4\n");

        soundfile = (short int *) (soundfile + num_samples);
        prev_outfile = outfile;
        outfile = (char *) (outfile + mp2fill_size);

        //printf("\n STEP 5\n");

        // Write the MPEG bitstream to the file
        //fwrite(mp2buffer, sizeof(unsigned char), mp2fill_size, outfile);

        // Display the number of MPEG audio frames we have encoded
        frames += (num_samples / TWOLAME_SAMPLES_PER_FRAME);
        printf("[%04i]\r", frames);
        fflush(stdout);

        //printf("\n STEP 6\n");
    }

    //printf("\n STEP 7\n");

    /* flush any remaining audio. (don't send any new audio data) There should only ever be a max
       of 1 frame on a flush. There may be zero frames if the audio data was an exact multiple of
       1152 */
    mp2fill_size = twolame_encode_flush(encodeOptions, (unsigned char *) prev_outfile, MP2BUFSIZE);
    //fwrite(mp2buffer, sizeof(unsigned char), mp2fill_size, outfile);
    fwrite(outfile, sizeof(unsigned char), mp2fill_size, only_test);

    //printf("\n STEP 8\n");

    twolame_close(&encodeOptions);

    printf("\nFinished nicely.\n");

    //file send
    //uart_sendfile("../../../sine.mp2");

    //uart_finish();

    return (0);
}
