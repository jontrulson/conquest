/*
 Cataboligne

 imagerec - dynamic array to contain all image data

 timerec - struct for timing arrays

*/

typedef struct _imagerec
{
  GLuint texture;         /* GL data for image */
  int img;                /* image ref # */
  float size;             /* scale size */
  int anim;               /* next image in animate seq */
  struct _imagerec *anext; /* next imagerec in anim seq */
  Unsgn32 wtime;          /* milliseconds till next animation */
  int ranim;              /* 0 - seq anim, 1 - randomize, >
                             1 - int seeds random */
  Unsgn32 atime;          /* milliseconds till next angular animation -
                             z axis rotate */
  float zanim;            /* angle to add (0.0 = none) */
  int scode;              /* special codes if any */
  Unsgn32 color;          /* color quad FF0000 = red, 00FF00 = green,
                             0000FF = blue, FFxxxxxx = alpha */
  struct _imagerec *next; /* pointer seq */
} imagerec_t;

typedef struct _timerec
{
  int index;               /* find our data again */
  struct _timerec *next;    /* more linked lists, these things are like a virus */
  Unsgn32 ttime;           /* ah yes, tea time...time till next action: currentmillis - ttime > actiontime */
  Unsgn32 atime;           /* time till next angular animation */
  imagerec_t *tstate;        /* current image state of animation */
  float lang;              /* last angle rotation */
} timerec_t ;

/* codes for scalar() */

#define GFX  1
#define TEXT 2
#define SHIP 4
#define STAT 8
#define STAM 16
#define REZ 32
