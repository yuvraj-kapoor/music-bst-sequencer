/*
  This code contains an implementation of a simple note synthesizer.

  It was originally created for CSC B63, and has now been updated
  for use in A48.

  *** YOU DO NOT NEED TO READ OR UNDERSTAND THE CODE HERE ***

  It is provided only to complement the assignment you're working
  on.

  However - if you are curious about how to generate sound and
  how to write a sound file, feel free to study the code below,
  ONLY AFTER YOU:

       - Have completed and tested your assignment solution
       - Have made sure you have time for this!

  If you're curious about sound, sound generation, or any of the
  code in here, feel free to drop by and I can help explain
  what's going on during the office hour.

  (c) F. Estrada, updated Feb. 2025
*/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>
#include<math.h>

#define USE_KS        0         // Set to 1 the instrument you want to use, set
#define USE_SAW       0         // to zero everything else. If more than one is
#define USE_SQ        0         // set non wavetable instruments win...
#define USE_PIANO     0
#define USE_CELLO     0
#define USE_GUITAR    1
#define USE_TRUMPET   0
#define PI 3.1415926535
#define SP_ADJ 1                // Speed adjustment, 1.0 standard speed
#define FS 44100			    // Defines the sampling frequency for note generation
                                // in Hz. that means, we will generate 44100 samples
                                // for each note per second. This is the number of
                                // samples needed for CD-quality sound.
                                // Question: What is sampling anyway?

typedef struct note_struct{		// This holds the data representing a single note
	double freq;			    // This note's frequency
	int bar;			        // WHich bar in the song this note belongs to
	double index;			    // Time index within the bar (from 0 to 1.0)
	double *waveform;		    // A pointer to a dynamically generated array which will contain
                                // the note's waveform (double precision, single channel, in [-1 1]
    double out_tminus1;		    // Output at t-1
	int wave_length;		    // Length of the waveform in samples
	int input_idx, output_idx;	// Indices used to access data in the waveform array
	int n_sampled;			    // Counter indicating how many samples have been generated for
                                // this note. Used to stop playing notes after a specified duration
                                // has been reached.
    double wt_index;            // wavetable index
	struct note_struct *next;	
} note;

// GLOBAL data (because there is no main() here)
note    *playlist_head=NULL;		// GLOBAL variable for the playlist!
note    *playlist_head2=NULL;       // One more to test reverseSong
char    note_names[100][5];		    // Array of note names, there are 100 of them
double  note_freq[100];			    // Array of note frequencies, one per note name
double  waves[4][4*44100];          // Each row in this array holds the digital recording for
                                    // an A4 note played by one instrument. 4 seconds of it
                                    // at 44100Hz, single channel.

                                    
void readWaves(void)
{
  // Read wave data for available notes from files
  // Piano at index 0
  // Guitar at index 1
  // Cello at index 2
  // Trumpet at index 3
  //
  // To add more instruments, change the size of the array for wave data
  // accordingly, and provide the .dat files with the wave data, then
  // add the corresponding lines to read the data into the array.
  // In new_note - add the line to init the note's wave data
  
  FILE *f;

  fprintf(stderr,"Reading wave data from files...\n");
  
  memset(&waves[0][0],0,sizeof(waves));     // Initialize wave data to zeros

  f=fopen("piano.dat","r");
  if (f<=0)
  {
    fprintf(stderr,"Unable to read wave data for Piano, please make sure it's in the correct directory.\n");
    return;
  }
  fread(&waves[0][0],4*44100,sizeof(double),f);
  fclose(f);

  f=fopen("guitar.dat","r");
  if (f<=0)
  {
    fprintf(stderr,"Unable to read wave data for Guitar, please make sure it's in the correct directory.\n");
    return;
  }
  fread(&waves[1][0],4*44100,sizeof(double),f);
  fclose(f);

  f=fopen("cello.dat","r");
  if (f<=0)
  {
    fprintf(stderr,"Unable to read wave data for Cello, please make sure it's in the correct directory.\n");
    return;
  }
  fread(&waves[2][0],4*44100,sizeof(double),f);
  fclose(f);

  f=fopen("trumpet.dat","r");
  if (f<=0)
  {
    fprintf(stderr,"Unable to read wave data for Trumpet, please make sure it's in the correct directory.\n");
    return;
  }
  fread(&waves[3][0],4*44100,sizeof(double),f);
  fclose(f);

  fprintf(stderr,"All good!\n");  
}
                                    
void read_note_table(void)
{
    // Read note names and frequencies from file
    FILE *f;
    char line[1024];
    int idx;

    f=fopen("note_frequencies.txt","r");
    if (f==NULL)
    {
      printf("Unable to open note frequencies file!\n");
      return;
    }

    idx=0;
    while (fgets(&line[0],1024,f))
    {
      note_names[idx][0]=line[0];
      note_names[idx][1]=line[1];
      if (line[2]!='\t') note_names[idx][2]=line[2]; else note_names[idx][2]='\0';
      note_names[idx][3]='\0';

      if (line[2]=='\t') note_freq[idx]=strtod(&line[3],NULL);
      else note_freq[idx]=strtod(&line[4],NULL);

      printf("Read note for table with name %s, frequency=%f\n",&note_names[idx][0],note_freq[idx]);
      idx++;
    }
    printf("Processed %d notes!\n",idx);
    
    // Read wave data
    readWaves();
}

note *new_note(double freq, int bar, double index)
{
 // This function creates an initializes a new note from the input values
 // that describe the note's frequency, and its position in the song
 // bar:index
 note *n;

 n=(note *)calloc(1,sizeof(note));
 if (!n)
 {
  fprintf(stderr,"new_note(): Out of Memory!, are you trying to load a Great Symphony???\n");
  return(NULL);
 }

 // Set up the note's internal data
 n->freq=freq;
 n->bar=bar;
 n->index=index;
 n->input_idx=0;
 n->output_idx=1;
 n->out_tminus1=0;
 n->n_sampled=0;
 n->wt_index=0;
 n->next=NULL;

 // We need to set up a dynamically allocated array to hold the note's waveform.
 // The length of this array must correspond to the number of samples it takes to
 // represent a waveform with the note's frequency.
 //
 // The formula is length=sampling_rate/note_frequency.
 // e.g. a 440Hz note (A4) at a sampling rate of 44100Hz requires 44100/440=100.22 (round to nearest)
 // samples to represent, and so, we would create an array with 100 entries for its waveform

 if (USE_KS || USE_SAW || USE_SQ)
 {
  n->wave_length=round((double)FS/n->freq);                       // Length of 1 cycle at this frequency
  n->waveform=(double *)calloc(n->wave_length,sizeof(double));
  if (!n)
  {
   fprintf(stderr,"new_note(): Ran out of memory allocating waveform. Toast!\n");
   return(NULL);
  }
  // Initialize the wave data depending on the selected wave type (KS, SAW, or SQUARE)
  for (int i=0;i<n->wave_length;i++)
  {
    if (USE_SAW)*(n->waveform + i)=(1.0*(double)i/(double)n->wave_length)-.5;	   	// Saw tooth - sounds synthy!
    else if (USE_SQ)
    {
      if (i<n->wave_length/2) *(n->waveform + i)=.5;				                // Square - old 8-bit :)
      else *(n->waveform + i)=-.5;
    }
    else *(n->waveform + i)=(1.0*(double)rand()/(double)RAND_MAX)-.5; 		        // Orig KS.
  }
 }
 else
 {
   // Note comes from a digital recording read from file. These are standard, set to
   // 4 seconds @ 44100 Hz, single channel, double precision data
   n->wave_length=4*44100;
   n->waveform=(double *)calloc(n->wave_length,sizeof(double));
   if (!n->waveform)
   {
    fprintf(stderr,"new_note(): Ran out of memory allocating waveform. Toast!\n");
    return(NULL);
   }
   if (USE_PIANO) memcpy(n->waveform,&waves[0][0],4*44100);
   else if (USE_GUITAR) memcpy(n->waveform,&waves[1][0],4*44100);
   else if (USE_CELLO) memcpy(n->waveform,&waves[2][0],4*44100);
   else memcpy(n->waveform,&waves[3][0],4*44100);
 }

 return(n);
}

note *playlist_insert(note *head, double freq, int bar, double index)
{

 // This function adds a new note at the requested location. The node is
 // added at the end of the linked list, so this assumes the notes are
 // already sorted in sequential order of bar:index. If this is not
 // true, weird things may happen during playback.

 note *n_n, *p;

 n_n=new_note(freq,bar,index);
 if (n_n==NULL) return head;

 if (head==NULL) return n_n;

 p=head;
 while (p->next!=NULL) p=p->next;
 p->next=n_n;
 return head;
}

void delete_playlist(note *head)
{
   // Release memory allocated for a playlist
   note *t;

   while (head!=NULL)
   {
     t=head->next;
     free(head->waveform);
     free(head);
     head=t;
   }
}

double WaveSample(note *n)
{
  // Plays back the note, adjusting for frequency since the
  // digital sample is at 440Hz
  
  double rel_inc=n->freq/440.0;
  int i1,i2;
  double s1,s2;
  double d1,d2;

  n->n_sampled++;                             // Ensure the note ends after 3 seconds!
  
  if (n->wt_index>=3.99*44100.00) return 0;   // Do not exceeed note length!
    
  n->wt_index+=rel_inc;                       // Index of next sample
  i1=floor(n->wt_index);
  i2=ceil(n->wt_index);
  
  if (i2>=4*44100) return 0;

  s1=*(n->waveform+i1);
  s2=*(n->waveform+i2);
  d1=1-((double)i1-n->wt_index);
  d2=1-d1;

  return (d1*s1)+(d2*s2);
}

double KS_string_sample(note *n)
{
 /*
    This function implements the Karplus-Strong plucked string
    algorithm. Karplus-Strong belongs to the class of sound synthesis
    algorithms that attempt to simulate the physical behaviour of
    the sound-producing components of a musical instrument.

    The Karplus-Strong method is in effect a simple simulation of the
    behaviour and propagation of vibrations along a string. It has
    two main components:

    A delay line which represents the vibrating string. The length
    of the delay line must be equivalent to the wavelength at which
    the string vibrates.

    A feedback loop consisting of a low-pass filter. There are many
    ways to set up this filter, here we use the simplest form which
    averages two consecutive samples and feeds them back to the
    input with a gain less than 1.

    The delay line works like this:
    * At each time step, the right-most entry in the delay line is sent
      to the output. The contents of the entire delay line are shifted
      one space to the right, and the input for this time step is
      stored at the left-most space in the delay line.
    * The 'input' in this case comes from the feedback loop.

    A simple schematic of KS is shown below:


			Delay line of length n,
			sound samples move to the
 			right by one space at each
			step
                -------------------------------------
     --- in --->|  |  |  |  |  |  |  |  |  |  |  |  |-------------> out
     |          -------------------------------------      |
     |							   |
     |			delay of length 1		   |
   (*.995)  			 _			   |
     |			   /----|_|----\                   |
     |                     |           |                   |
     \-------------------- + ------------------------------/
		Here the new input is
		computed as a weighted sum of
                the current and previous output
		and then multiplied by .995 to
		avoid self-amplifying feedback

   Of course, displacing all data in the delay line would be slow,
   instead. The delay line is implemented as a circular array and two
   pointers are used to keep track of the current input and output.

   Much more complex physical models exist. If you are curious,
   visit the Stanford University's Center for Computer Research in
   Music and Acoustics (CCRMA). And look for 'Virtual Acoustic
   Synthesis' and 'waveguides'.

   Enjoy the sound of plucked strings!

   Incidentally, most software synthesizers rely on recordings of
   instrument waveforms which are played back at the appropriate
   frequency and over which effects and processing are performed.
   Physical simulation is still limited by the complexity and
   computational cost of its models.

   If you are curious about how modern software synthesizers work,
   read up on 'wavetable synthesis'.
 */

 double output, new_input;
 double alpha=n->freq/5000.0;
 double discount;

 if (alpha<.8) alpha=.8;
 if (alpha>.9995) alpha=.9995;

 n->n_sampled++;		// Keep track of how many times we've sampled this note
				        // to see if we have exceeded the note's duration

 // Get output
 output=*(n->waveform+n->output_idx);
 new_input=.9875*(((1.0-alpha)*n->out_tminus1)+(alpha*output));

 // Update internal state
 n->out_tminus1=output;
 *(n->waveform+n->input_idx)=new_input;

 n->output_idx++;
 if (n->output_idx==n->wave_length) n->output_idx=0;
 n->input_idx++;
 if (n->input_idx==n->wave_length) n->input_idx=0;

 return output;

}

void write_wav_header(FILE *f, unsigned int samples)
{
 /*
   Writes out the .wav header for the output file.
 */
 char txt[250];
 uint32_t chunk2size;
 uint32_t tmp;
 unsigned char tmp2;
 // Compute the chunk size in bytes
 chunk2size=samples*2*16/8;			// # samples, 2 channels, 16 bits/channel, 8 bits per byte

 strcpy(&txt[0],"RIFF");			// RIFF header
 fwrite(&txt[0],4*sizeof(unsigned char),1,f);
 tmp=chunk2size+36;
 fwrite(&tmp,4*sizeof(unsigned char),1,f);	// You will have to fix this for 64bit systems!
 strcpy(&txt[0],"WAVE");
 fwrite(&txt[0],4*sizeof(unsigned char),1,f);	// Wave file identifier
 strcpy(&txt[0],"fmt ");
 fwrite(&txt[0],4*sizeof(unsigned char),1,f);	// format section
 tmp=16;
 fwrite(&tmp,4*sizeof(unsigned char),1,f);	// Because I said so...
 tmp2=1;
 fwrite(&tmp2,sizeof(unsigned char),1,f);
 tmp2=0;
 fwrite(&tmp2,sizeof(unsigned char),1,f);	// Format identifier (01) for linear PCM (remember Endianness??)
 tmp2=2;
 fwrite(&tmp2,sizeof(unsigned char),1,f);
 tmp2=0;
 fwrite(&tmp2,sizeof(unsigned char),1,f);	// Number of channels (02) for two channel stereo
 tmp=FS;
 fwrite(&tmp,4*sizeof(unsigned char),1,f);	// Sampling rate
 tmp=FS*2*16/8;
 fwrite(&tmp,4*sizeof(unsigned char),1,f);	// Byte rate, sampling rate * num channels * bits/channel / 8 bits
 tmp2=2*16/8;
 fwrite(&tmp2,sizeof(unsigned char),1,f);
 tmp2=0;
 fwrite(&tmp2,sizeof(unsigned char),1,f);	// Block alignment = num_channels * bits/channel / 8 bits
 tmp2=16;
 fwrite(&tmp2,sizeof(unsigned char),1,f);
 tmp2=0;
 fwrite(&tmp2,sizeof(unsigned char),1,f);	// Bits per sample - 16
 strcpy(&txt[0],"data");
 fwrite(&txt[0],4*sizeof(unsigned char),1,f);	// data field follows...
 fwrite(&chunk2size,4*sizeof(unsigned char),1,f);	// But of course, first let's insert the chunk2size! (why not?)
 // Data follows this portion...
}

void play_notes(int bar_length)
{
 /*
   This is the main synthesis function. It starts a time counter from bar=1, idex=0
   and plays out the notes in the song at the specified times. Notes are placed in
   a temporary linked list and pointers are used to keep track of which notes are
   active at any given moment.

   Sound synthesis is done through each note's next_sample() function, which
   returns the value of the waveform for that note for each sampling step.

   bar_length indicates the duration in seconds for each bar, and controls the
   overall speed of playback.
 */
 note *q, *st, *ed;
 int bar, max_bar, max_length, s_idx;
 double index;
 unsigned int sample_idx, max_sample_idx;
 int16_t this_sample_int;
 double this_sample, this_sample_r, this_sample_l;
 double balance,fAmp;
 int done;
 int cc,tc;
 FILE *f;
 
 if (playlist_head==NULL)
 {
   printf("Input playlist is empty!\n");
   return;
 }

 max_bar=0;
 q=playlist_head;
 while (q!=NULL)
 {
  max_bar=q->bar;
  q=q->next;
 }

 // Let's play!
 max_length=4*FS;		// Maximum note length, number of seconds * sampling rate.
				        // A more complex synthesizer would allow each note to have a different lentgh.
				        // We would need to indicate note lengths in the input file and store them in
				        // the note data structure

 // Calculate song length in samples
 max_sample_idx=bar_length*FS*(max_bar+1);

 fprintf(stderr,"\nPlaying song. %d bars, max note length is %d samples. Output is 'output.wav'\n\n",max_bar,max_length); 

 f=fopen("output.wav","wb+");	// Open file 'output.wav' for writing. Mind the file type if you work on Windows
 if (f)
 {
  write_wav_header(f,max_sample_idx);

  // Main loop of the synthesis part. Keeps track of bar:index position and plays any notes
  // that are active within the interval. This includes notes that start at this index,
  // and previously active notes that have not finished playing.
  // For each sample, we ask each note to provide a sample via its next_sample() function
  // which is tied to the appropriate synthesis algorithm

  st=ed=playlist_head;			// Start and end pointers indicating range of active notes
					            // ed is manipulated via the bar:index combination.
					            // st moves forward when notes complete playing.
  sample_idx=0;				    // Global sample index, used to terminate if for some
					            // reason a note goes beyond our estimate for max_sample_idx
					            					            
  for (bar=playlist_head->bar;bar<=max_bar;bar++)
  {
   for (index=0;index<=1.0;index+=1.0*SP_ADJ/(bar_length*44100.0))
   {
     if (sample_idx==0) index=st->index;
     
     // Check if we have to update ed (i.e. we have to add notes for this time index)
     done=0;
     while(!done)
     {
      done=1;
      if (ed->next!=NULL)
      {
        if ((double)ed->next->bar+ed->next->index<(double)bar+index)
        {
	     ed=ed->next;
	     done=1;
        }
      }
      else done=1;
     }

    // For each note between st and ed, generate any needed samples, and output to file
    // Note that at this point it would be MUCH more efficient to have a buffer where to
    // store temporary output data, and write to disk only a few times instead of once
    // for every sample. For simplicity, we'll write directly, and use this as a chance
    // to see how good the OS is as buffering file output by itself.

    // Need to generate all the samples for this index
    tc=0;
    if (st!=NULL)
    {
      q=st;
      cc=0;
      this_sample_l=0;
      this_sample_r=0;
      
      while(ed->next!=q)
      {
       balance=(q->freq-525.0);			            // C5 at center
       if (balance<0) balance=(525.0+balance)/525.0;
       else balance=balance/(5000.0-525.0);
       fAmp=(pow(q->freq,.33)/18.0);		        // Amplitude boost for high freq. notes
  
       if (USE_KS || USE_SAW || USE_SQ)             // Get next sample as needed
        this_sample=KS_string_sample(q)*fAmp;
       else 
        this_sample=WaveSample(q)*fAmp;
        
       this_sample_l+=(balance)*this_sample;
       this_sample_r+=(1.0-balance)*this_sample;
       q=q->next;
       cc++;
      }

      if (cc>tc) tc=cc;
      //////////////////////////////////////////////////////////////////////////////////
      //   Note that I use position in the scale as a way to separate notes
      ///  across the L and R channels. What else could we think of doing here?
      //////////////////////////////////////////////////////////////////////////////////

      // Clip to [-1, 1] transform to a 2-byte signed integer for each channel, and write
      // to file
      this_sample_r=tanh(this_sample_r);
      if (this_sample_r>=.99999) this_sample_r=.99999;
      else if (this_sample_r<=-.99999) this_sample_r=-.99999;

      this_sample_int=(int16_t)(this_sample_r*32700);
      fwrite(&this_sample_int,2*sizeof(unsigned char),1,f);    	// R channel

      this_sample_l=tanh(this_sample_l);
      if (this_sample_l>=.99999) this_sample_l=.99999;
      else if (this_sample_l<=-.99999) this_sample_l=-.99999;

      this_sample_int=(int16_t)(this_sample_l*32700);
      fwrite(&this_sample_int,2*sizeof(unsigned char),1,f);    	// L channel

      sample_idx++;
    }
    else	// No more notes to play, fill out the remainder of the file with zeros.
    {
     // Generate all samples with zeros
     for (s_idx=0;s_idx<44100;s_idx++)
     {
      this_sample_int=0;
      fwrite(&this_sample,2*sizeof(unsigned char),1,f);    	// L channel
      fwrite(&this_sample,2*sizeof(unsigned char),1,f);    	// R channel
      sample_idx++;
     }
    }

    // Check if notes have ended
    done=0;
    while (!done)
    {
     done=1;
     if (st!=NULL&&st!=ed->next)				// Should never go beyond ed
      if (st->n_sampled>max_length)
      {
       st=st->next;
       done=0;
      }
    }
    if (sample_idx>max_sample_idx) break;
   }		// End for index...
   if (sample_idx>max_sample_idx) break;
  }		// End for bar...
  fclose(f);
 }
 else fprintf(stderr,"Unable to open file for output!\n");

 printf("Max concurrent notes: %d\n",tc);
 delete_playlist(playlist_head);
 playlist_head=NULL;
}

