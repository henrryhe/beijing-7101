
#ifndef TETRIS_
#define TETRIS_

#define MAXX /*18*/12
#define MAXY /*30*/20
#define MAXTYPE 7
#define STEP  1
#define STEPX 20/*20*/
#define STEPY 20/*15*/
#define GAMEINIT  1
#define GAMESTART  2
#define GAMEHAPPY 3
#define GAMEOVER  4

#ifndef OSD_COLOR_TYPE_RGB16  
#define BLUEBACK_COLOR       		0xff305080
#define TETRIS_TEXT_COLOR				0xfff8f8f8
#define TETRIS_BACK_COLOR		0xff307050
#define TETRIS_BLOCK_COLOR		0xfff08020
#else
#define BLUEBACK_COLOR       		0x9950
#define TETRIS_TEXT_COLOR				0xFFFF
#define TETRIS_BACK_COLOR		0x99ca
#define TETRIS_BLOCK_COLOR		0xFa04
#endif
struct Li
{
int x;
int y;
int s;
};

struct MU
{
struct Li li[4][4];
int mtype;
int mdir;
int st;
int hit;
int nodown;
int noleft;
int noright;
};
struct JI
{
struct Li li[MAXX][MAXY];
int jfull;
int hit;
};
extern void elsmain(void);
extern struct MU activemu,nextmu,previosmu;
extern struct JI box;
#endif

