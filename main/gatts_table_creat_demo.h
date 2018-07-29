/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Attributes State Machine */
enum
{
    IDX_SVC,

    IDX_CHAR_SCORE,
    IDX_CHAR_VAL_SCORE,

    IDX_CHAR_FLAG,
    IDX_CHAR_VAL_FLAG,
    
    IDX_CHAR_NAME,
    IDX_CHAR_VAL_NAME,

    IDX_CHAR_HLTH,
    IDX_CHAR_VAL_HLTH,

    IDX_CHAR_OLD1,
    IDX_CHAR_VAL_OLD1,
    IDX_CHAR_CFG_OLD1,

    IDX_CHAR_OLD2,
    IDX_CHAR_VAL_OLD2,
    IDX_CHAR_CFG_OLD2,

    IDX_CHAR_OLD3,
    IDX_CHAR_VAL_OLD3,
    IDX_CHAR_CFG_OLD3,

    IDX_CHAR_BART,
    IDX_CHAR_VAL_BART,
    IDX_CHAR_CFG_BART,

    IDX_CHAR_POMM,
    IDX_CHAR_VAL_POMM,
    IDX_CHAR_CFG_POMM,

    IDX_CHAR_HILT,
    IDX_CHAR_VAL_HILT,
    IDX_CHAR_CFG_HILT,

    IDX_CHAR_SCAB,
    IDX_CHAR_VAL_SCAB,
    IDX_CHAR_CFG_SCAB,

    IDX_CHAR_BLAD,
    IDX_CHAR_VAL_BLAD,
    IDX_CHAR_CFG_BLAD,

    IDX_CHAR_CROS,
    IDX_CHAR_VAL_CROS,
    IDX_CHAR_CFG_CROS,

    IDX_CHAR_BLKS,
    IDX_CHAR_VAL_BLKS,
    IDX_CHAR_CFG_BLKS,

    IDX_CHAR_TROL,
    IDX_CHAR_VAL_TROL,
    IDX_CHAR_CFG_TROL,

    IDX_CHAR_TERR,
    IDX_CHAR_VAL_TERR,
    IDX_CHAR_CFG_TERR,
  

    HRS_IDX_NB,

};
