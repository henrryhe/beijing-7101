/*****************************************************************************

File name: engine.c

Description:

COPYRIGHT (C) 2006 STMicroelectronics

*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#if !defined(ST_OSLINUX) || defined(STBLIT_LINUX_FULL_USER_VERSION)
#include <string.h>
#include <stdlib.h>
#endif

#include "blt_engine.h"
#include "sttbx.h"
#include "stos.h"

/* Private Types ---------------------------------------------------------- */

/* Private Constants ------------------------------------------------------ */

/* Private Variables ------------------------------------------------------ */

/* Private Function prototypes -------------------------------------------- */

/* Constants -------------------------------------------------------------- */

/* Global Variables ------------------------------------------------------- */
typedef struct
{
    int Index_i;
    int Index_j;
} MatrixCell_t;

MatrixCell_t SnakeArray[DEFAULT_DEMO_MATRIX_SIZE * DEFAULT_DEMO_MATRIX_SIZE];
MatrixCell_t ObstacleArray[DEFAULT_DEMO_MATRIX_SIZE];
MatrixCell_t Target;

int Index, LengthSnake;

extern ST_Partition_t *SystemPartition_p;

/* Fonctions -------------------------------------------------------------- */
void DemoEngine_InitSnakeArray(void);
void DemoEngine_InitAreaMatrix(void);
void DemoEngine_InitObstacleArray(void);
void DemoEngine_InitArrays(void);
void DemoEngine_UpdateSnakeArray(int Index_i, int Index_j);
void DemoEngine_UpdateAreaMatrix(void);

void DemoEngine_GrowSnakeArray(int Index_i, int Index_j);
int  DemoEngine_IsFreeCell(int Index_i, int Index_j);
int  DemoEngine_Random(int min, int max);


/*******************************************************************************
Name        : DemoEngine_Init
Parameters  :
Returns     :
*******************************************************************************/
void DemoEngine_Init()
{
    DemoEngine_InitArrays();
    DemoEngine_InitSnakeArray();
    DemoEngine_InitObstacleArray();
    DemoEngine_UpdateAreaMatrix();
}

/*******************************************************************************
Name        : DemoEngine_Term
Parameters  :
Returns     :
*******************************************************************************/
void DemoEngine_Term()
{
    int i;
    for(i=0;i<(SnakeDemoParameters.Complexity * SnakeDemoParameters.Complexity);i++)
    {
        STOS_MemoryDeallocate(SystemPartition_p,&SnakeArray[i]);
    }

    for(i=0;i<SnakeDemoParameters.NbrObstacles;i++)
    {
        STOS_MemoryDeallocate(SystemPartition_p,&ObstacleArray[i]);
    }
}

/*******************************************************************************
Name        : DemoEngine_InitArrays
Parameters  :
Returns     :
*******************************************************************************/
void DemoEngine_InitArrays()
{
    int i;
    MatrixCell_t  *MatrixCell;

    /* initialize SnakeArray */
    for(i=0;i<(SnakeDemoParameters.Complexity * SnakeDemoParameters.Complexity);i++)
    {
        MatrixCell = (MatrixCell_t *)STOS_MemoryAllocate(SystemPartition_p,sizeof(MatrixCell_t));
        SnakeArray[i] = *MatrixCell;
    }
    /* initialize ObstacleArray */
    for(i=0;i<SnakeDemoParameters.NbrObstacles;i++)
    {
        MatrixCell = (MatrixCell_t *)STOS_MemoryAllocate(SystemPartition_p,sizeof(MatrixCell_t));
        ObstacleArray[i] = *MatrixCell;
    }

}

/*******************************************************************************
Name        : DemoEngine_InitSnakeArray
Parameters  :
Returns     :
*******************************************************************************/
void DemoEngine_InitSnakeArray()
{
    SnakeArray[0].Index_i = 1;
    SnakeArray[0].Index_j = 4;

    SnakeArray[1].Index_i = 1;
    SnakeArray[1].Index_j = 3;

    SnakeArray[2].Index_i = 1;
    SnakeArray[2].Index_j = 2;

    SnakeArray[3].Index_i = 1;
    SnakeArray[3].Index_j = 1;

    Index = 4;
    LengthSnake = 4;
}

/*******************************************************************************
Name        : DemoEngine_InitObstacleArray()
Parameters  :
Returns     :
*******************************************************************************/
void DemoEngine_InitObstacleArray()
{
    int i;

    for (i=0; i<SnakeDemoParameters.NbrObstacles; i++)
    {
        ObstacleArray[i].Index_i = DemoEngine_Random(3,SnakeDemoParameters.Complexity -2);
        ObstacleArray[i].Index_j = DemoEngine_Random(2,SnakeDemoParameters.Complexity -2);
    }
}

 /*----------------------------------------------------------------------
 * Function : DemoEngine_InitTarget()

 *  Input    : none
 *  Output   : none
 *  Return   :
 * --------------------------------------------------------------------*/
void DemoEngine_InitTarget()
{
    do
    {
        Target.Index_i = DemoEngine_Random(2,SnakeDemoParameters.Complexity -2);
        Target.Index_j = DemoEngine_Random(2,SnakeDemoParameters.Complexity -2);

    } while(DemoEngine_IsFreeCell(Target.Index_i,Target.Index_j) == 1);

    DemoEngine_Matrix[Target.Index_i][Target.Index_j] = 't';
}


  /*----------------------------------------------------------------------
 * Function : DemoEngine_UpdateAreaMatrix

 *  Input    : none
 *  Output   : none
 *  Return   : none
 * --------------------------------------------------------------------*/
void DemoEngine_UpdateAreaMatrix()
{
    int i,j;
    i=0;
    j=0;

    /*set background*/
    for(i=0;i<SnakeDemoParameters.Complexity;i++)
    {
        for(j=0;j<SnakeDemoParameters.Complexity;j++)
        {
            DemoEngine_Matrix[i][j] = 'b';
        }
    }
    /*set a Wall*/
    for(i=0; i<SnakeDemoParameters.Complexity; i++)
    {
        DemoEngine_Matrix[i][0] = 'w';
        DemoEngine_Matrix[i][SnakeDemoParameters.Complexity-1] = 'w';
    }
    for(j=0;j<SnakeDemoParameters.Complexity;j++)
    {
         DemoEngine_Matrix[0][j] = 'w';
         DemoEngine_Matrix[SnakeDemoParameters.Complexity-1][j] = 'w';
    }

    /*set obstacles*/
    for(i=0;i<SnakeDemoParameters.NbrObstacles;i++)
    {
        DemoEngine_Matrix[ObstacleArray[i].Index_i][ObstacleArray[i].Index_j] = 'o';
    }

    /*set snake Head*/
    DemoEngine_Matrix[SnakeArray[0].Index_i][SnakeArray[0].Index_j] = 's';

    /*set snake body */
    for(i=1;i<LengthSnake;i++)
    {
        DemoEngine_Matrix[SnakeArray[i].Index_i][SnakeArray[i].Index_j] = 'x';
    }

    /*set target*/
    DemoEngine_Matrix[Target.Index_i][Target.Index_j] = 't';
}

/*******************************************************************************
Name        : DemoEngine_GrowSnake
              add a body structure to snake table
Parameters  :
Returns     :
*******************************************************************************/
void DemoEngine_GrowSnake(int i, int j)
{
    Index ++;
    SnakeArray[Index-1].Index_i = i;
    SnakeArray[Index-1].Index_j = j;
}

 /*----------------------------------------------------------------------
 * Function : DemoEngine_UpdateSnakeArray
              update a snake table
 *  Input    : none
 *  Output   : none
 *  Return   : none
 * --------------------------------------------------------------------*/
void DemoEngine_UpdateSnakeArray(int Index_i,int Index_j)
{
    int i;
    if ( Index > LengthSnake)
    {
        if ( (SnakeArray[LengthSnake].Index_i == SnakeArray[LengthSnake-1].Index_i) && (SnakeArray[LengthSnake].Index_j == SnakeArray[LengthSnake-1].Index_j))
        {
            LengthSnake ++;
        }
    }

    for(i=0;i<LengthSnake-1;i++)
    {
        SnakeArray[LengthSnake-i-1].Index_i = SnakeArray[LengthSnake-i-2].Index_i;
        SnakeArray[LengthSnake-i-1].Index_j = SnakeArray[LengthSnake-i-2].Index_j;
    }
    SnakeArray[0].Index_i=Index_i;
    SnakeArray[0].Index_j=Index_j;
}

/*----------------------------------------------------------------------
* Function : random
            give a number between min and max
*  Input    : int min, int max
*  Output   : none
*  Return   : int nbr
* --------------------------------------------------------------------*/
int DemoEngine_Random(int min, int max)
{
    srand (STOS_time_now());
    return(min + (int)(rand()%(max+1-min)));
}

/*******************************************************************************
Name        : DemoEngine_IsFreeCell()
Parameters  : ti,tj position of case in matrix
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
int DemoEngine_IsFreeCell(int Index_i, int Index_j)
{
    int i;
    for(i=0;i<Index;i++)
    {
        if ((SnakeArray[i].Index_i == Index_i) && (SnakeArray[i].Index_j == Index_j))
        {
                return(1);
        }
    }

    for(i=0; i<SnakeDemoParameters.NbrObstacles; i++)
    {
        if ((ObstacleArray[i].Index_i == Index_i) && (ObstacleArray[i].Index_j == Index_j))
        {
                return(1);
        }
    }
    return(0);
}
 /*----------------------------------------------------------------------
 * Function : snake_move
              give a next position of the snake
 *  Input    :
 *  Output   : none
 *  Return   : 1 if the next position is a target else 0
 * --------------------------------------------------------------------*/
int DemoEngine_MoveSnake()
{
    int snake_i,snake_j,i,j;
    int move_i,move_j;

    snake_i = SnakeArray[0].Index_i;
    snake_j = SnakeArray[0].Index_j;
    move_i = snake_i;
    move_j = snake_j;
    i = SnakeArray[0].Index_i;
    j = SnakeArray[0].Index_j;

    /*Determinate the next position*/
    if (snake_j < Target.Index_j)
    {
        if ((DemoEngine_Matrix[i][j+1] == 'b') || (DemoEngine_Matrix[i][j+1] == 't'))
        {
            DemoEngine_UpdateSnakeArray(i,j+1);
            DemoEngine_UpdateAreaMatrix();
            j ++;
            snake_j = j;
        }

        else if (((DemoEngine_Matrix[i+1][j] == 'b') || (DemoEngine_Matrix[i+1][j] == 't')) && (snake_i < Target.Index_i))
        {
            DemoEngine_UpdateSnakeArray(i+1,j);
            DemoEngine_UpdateAreaMatrix();
            i ++;
            snake_i = i;
        }
        else if (((DemoEngine_Matrix[i-1][j] == 'b') || (DemoEngine_Matrix[i-1][j] == 't')) && (snake_i > Target.Index_i))
        {
            DemoEngine_UpdateSnakeArray(i-1,j);
            DemoEngine_UpdateAreaMatrix();
            i --;
            snake_i = i;
        }
        else if ((DemoEngine_Matrix[i][j-1] == 'b') || (DemoEngine_Matrix[i][j-1] == 't'))
        {
            DemoEngine_UpdateSnakeArray(i,j-1);
            DemoEngine_UpdateAreaMatrix();
            j --;
            snake_j = j;
        }
        else if ((DemoEngine_Matrix[i+1][j] == 'b') || (DemoEngine_Matrix[i+1][j] == 't'))
        {
            DemoEngine_UpdateSnakeArray(i+1,j);
            DemoEngine_UpdateAreaMatrix();
            i ++;
            snake_i = i;
        }
        else if ((DemoEngine_Matrix[i-1][j] == 'b') || (DemoEngine_Matrix[i-1][j] == 't'))
        {
            DemoEngine_UpdateSnakeArray(i-1,j);
            DemoEngine_UpdateAreaMatrix();
            i --;
            snake_i = i;
        }
    }

    else if (snake_j > Target.Index_j)
    {
        if ((DemoEngine_Matrix[i][j-1] == 'b') || (DemoEngine_Matrix[i][j-1] == 't'))
        {
            DemoEngine_UpdateSnakeArray(i,j-1);
            DemoEngine_UpdateAreaMatrix();
            j --;
            snake_j = j;
        }
        else if (((DemoEngine_Matrix[i+1][j] == 'b') || (DemoEngine_Matrix[i+1][j] == 't')) && (snake_i < Target.Index_i))
        {
            DemoEngine_UpdateSnakeArray(i+1,j);
            DemoEngine_UpdateAreaMatrix();
            i ++;
            snake_i = i;
        }
        else if (((DemoEngine_Matrix[i-1][j] == 'b')||(DemoEngine_Matrix[i-1][j] == 't')) && (snake_i > Target.Index_i))
        {
            DemoEngine_UpdateSnakeArray(i-1,j);
            DemoEngine_UpdateAreaMatrix();
            i --;
            snake_i = i;
        }
        else if ((DemoEngine_Matrix[i][j+1] == 'b') || (DemoEngine_Matrix[i][j+1] == 't'))
        {
            DemoEngine_UpdateSnakeArray(i,j+1);
            DemoEngine_UpdateAreaMatrix();
            j ++;
            snake_j = j;
        }
        else if ((DemoEngine_Matrix[i+1][j] == 'b') || (DemoEngine_Matrix[i+1][j] == 't'))
        {
            DemoEngine_UpdateSnakeArray(i+1,j);
            DemoEngine_UpdateAreaMatrix();
            i ++;
            snake_i = i;
        }
        else if ((DemoEngine_Matrix[i-1][j] == 'b') || (DemoEngine_Matrix[i-1][j] == 't'))
        {
            DemoEngine_UpdateSnakeArray(i-1,j);
            DemoEngine_UpdateAreaMatrix();
            i --;
            snake_i = i;
        }
    }
    else if (snake_i < Target.Index_i)
    {
        if ((DemoEngine_Matrix[i+1][j] == 'b') || (DemoEngine_Matrix[i+1][j] == 't'))
        {
            DemoEngine_UpdateSnakeArray(i+1,j);
            DemoEngine_UpdateAreaMatrix();
            i ++;
            snake_i = i;
        }

        else if ((DemoEngine_Matrix[i][j+1] == 'b') || (DemoEngine_Matrix[i][j+1] == 't'))
        {
            DemoEngine_UpdateSnakeArray(i,j+1);
            DemoEngine_UpdateAreaMatrix();
            j ++;
            snake_j = j;
        }
        else if ((DemoEngine_Matrix[i][j-1] == 'b') || (DemoEngine_Matrix[i][j-1] == 't'))
        {
            DemoEngine_UpdateSnakeArray(i,j-1);
            DemoEngine_UpdateAreaMatrix();
            j --;
            snake_j = j;
        }
        else if ((DemoEngine_Matrix[i-1][j] == 'b') || (DemoEngine_Matrix[i-1][j] == 't'))
        {
            DemoEngine_UpdateSnakeArray(i-1,j);
            DemoEngine_UpdateAreaMatrix();
            i --;
            snake_i = i;
        }

    }

    else if (snake_i > Target.Index_i)
    {
        if ((DemoEngine_Matrix[i-1][j] == 'b') || (DemoEngine_Matrix[i-1][j] == 't'))
        {
            DemoEngine_UpdateSnakeArray(i-1,j);
            DemoEngine_UpdateAreaMatrix();
            i --;
            snake_i = i;
        }
        else if ((DemoEngine_Matrix[i][j-1] == 'b')||(DemoEngine_Matrix[i][j-1] == 't'))
        {
            DemoEngine_UpdateSnakeArray(i,j-1);
            DemoEngine_UpdateAreaMatrix();
            j --;
            snake_j = j;
        }
        else if ((DemoEngine_Matrix[i][j+1] == 'b') || (DemoEngine_Matrix[i][j+1] == 't'))
        {
            DemoEngine_UpdateSnakeArray(i,j+1);
            DemoEngine_UpdateAreaMatrix();
            j ++;
            snake_j =j;
        }
        else if ((DemoEngine_Matrix[i+1][j] == 'b') || (DemoEngine_Matrix[i+1][j] == 't'))
        {
            DemoEngine_UpdateSnakeArray(i+1,j);
            DemoEngine_UpdateAreaMatrix();
            i ++;
            snake_i = i;
        }
    }

    if ((snake_i == move_i) && (snake_j == move_j))
    {
        DemoEngine_InitSnakeArray();
        /*target_init()*/
        return (1);
    }

    if ((snake_i == Target.Index_i) && (snake_j == Target.Index_j))
    {
        DemoEngine_GrowSnake(Target.Index_i,Target.Index_j);
        return (1);
    }
    else
    {
        return (0);
    }
}
