#include "k-chinese.h"

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/*
****************************************************************************
*                                   uC/GUI
*           Universal graphic software for embedded applications
*
*          (c) Copyright 2002, Micrium Inc., Weston, FL
*          (c) Copyright 2002, SEGGER Microcontroller Systeme GmbH
*
* ¦ÌC/GUI is protected by international copyright laws. Knowledge of the
* source code may not be used to write a similar product. This file may
* only be used in accordance with a license and should not be redistributed
* in any way. We appreciate your understanding and fairness.
*
----------------------------------------------------------------------
Purpose     : Implementation of Proportional fonts
---------------------------END-OF-HEADER------------------------------
*/
/* needed for definition of NULL */
#include <stddef.h>
#include "string.h"
#include "GUI_Private.h"
#include "ff.h"
#include "fatfs.h"

#define BYTES_PER_FONT  1024
#define GUI_FONTTYPE_PROP_USER                   \
    GUIPROP_X_DispChar,                       \
    (GUI_GETCHARDISTX*)GUIPROP_X_GetCharDistX,    \
    GUIMONO_GetFontInfo,                    \
    GUIMONO_IsInFont,                       \
    (GUI_GETCHARINFO *)0,                   \
    (tGUI_ENC_APIList*)0

//#include "ff.h"
//#include "fatfs.h"
//#include "k-chinese.h"
//__no_init char Bmp_Buffer[0x200000] @0xC1800000;
//uint32_t bread;
//f_open(&USERFile, (const TCHAR*)"0:/Pictures/ButtonOn.bmp", FA_READ);
//f_read(&USERFile, Bmp_Buffer, USERFile.obj.objsize, (UINT *)&bread);
//f_close(&USERFile);
//BUTTON_SetBMP(hItem, BUTTON_BI_UNPRESSED, Bmp_Buffer);
/*********************************************************/
GUI_CONST_STORAGE GUI_CHARINFO GUI_FontCN12_CharInfo[2] = {
    { 6, 6, 1, (void *)"0"},
    { 12, 12, 2, (void *)"0"},
};

GUI_CONST_STORAGE GUI_FONT_PROP GUI_FontCN12_PropCN = {
    0x4081,
    0xFFFF,
    &GUI_FontCN12_CharInfo[1],
    (void *)0,
};

GUI_CONST_STORAGE GUI_FONT_PROP GUI_FontCN12_PropASC = {
    0x0000,
    0x007F,
    &GUI_FontCN12_CharInfo[0],
    (void GUI_CONST_STORAGE *) &GUI_FontCN12_PropCN,
};

GUI_CONST_STORAGE  GUI_FONT GUI_FontCN12 = {
    GUI_FONTTYPE_PROP_USER,
    12,
    12,
    1,
    1,
    (void GUI_CONST_STORAGE *) &GUI_FontCN12_PropASC
};

GUI_CONST_STORAGE  GUI_FONT GUI_FontCN12x2 = {
    GUI_FONTTYPE_PROP_USER,
    12,
    12,
    2,
    2,
    (void GUI_CONST_STORAGE *) &GUI_FontCN12_PropASC
};

/*********************************************************/

GUI_CONST_STORAGE GUI_CHARINFO GUI_FontCN16_CharInfo[2] = {
    { 8, 8, 1, (void *)"0"},
    { 16, 16, 2, (void *)"0"},
};

GUI_CONST_STORAGE GUI_FONT_PROP GUI_FontCN16_PropCN = {
    0x4081,
    0xFFFF,
    &GUI_FontCN16_CharInfo[1],
    (void *)0,
};

GUI_CONST_STORAGE GUI_FONT_PROP GUI_FontCN16_PropASC = {
    0x0000,
    0x007F,
    &GUI_FontCN16_CharInfo[0],
    (void GUI_CONST_STORAGE *) &GUI_FontCN16_PropCN,
};

GUI_CONST_STORAGE  GUI_FONT GUI_FontCN16 = {
    GUI_FONTTYPE_PROP_USER,
    16,
    16,
    1,
    1,
    (void GUI_CONST_STORAGE *) &GUI_FontCN16_PropASC
};

GUI_CONST_STORAGE  GUI_FONT GUI_FontCN16x2 = {
    GUI_FONTTYPE_PROP_USER,
    16,
    16,
    2,
    2,
    (void GUI_CONST_STORAGE *) &GUI_FontCN16_PropASC
};

/*********************************************************/

GUI_CONST_STORAGE GUI_CHARINFO GUI_FontCN24_CharInfo[2] = {
    { 12, 12, 2, (void *)"0"},
    { 24, 24, 3, (void *)"0"},
};

GUI_CONST_STORAGE GUI_FONT_PROP GUI_FontCN24_PropCN = {
    0x4081,
    0xFFFF,
    &GUI_FontCN24_CharInfo[1],
    (void *)0,
};

GUI_CONST_STORAGE GUI_FONT_PROP GUI_FontCN24_PropASC = {
    0x0000,
    0x007F,
    &GUI_FontCN24_CharInfo[0],
    (void GUI_CONST_STORAGE *) &GUI_FontCN24_PropCN,
};

GUI_CONST_STORAGE  GUI_FONT GUI_FontCN24 = {
    GUI_FONTTYPE_PROP_USER,
    24,
    24,
    1,
    1,
    (void GUI_CONST_STORAGE *) &GUI_FontCN24_PropASC
};

GUI_CONST_STORAGE  GUI_FONT GUI_FontCN24x2 = {
    GUI_FONTTYPE_PROP_USER,
    24,
    24,
    2,
    2,
    (void GUI_CONST_STORAGE *) &GUI_FontCN24_PropASC
};

/*********************************************************/

GUI_CONST_STORAGE GUI_CHARINFO GUI_FontCN32_CharInfo[2] = {
    { 16, 16, 2, (void *)"0"},
    { 32, 32, 4, (void *)"0"},
};

GUI_CONST_STORAGE GUI_FONT_PROP GUI_FontCN32_PropCN = {
    0x4081,
    0xFFFF,
    &GUI_FontCN32_CharInfo[1],
    (void *)0,
};

GUI_CONST_STORAGE GUI_FONT_PROP GUI_FontCN32_PropASC = {
    0x0000,
    0x007F,
    &GUI_FontCN32_CharInfo[0],
    (void GUI_CONST_STORAGE *) &GUI_FontCN32_PropCN,
};

GUI_CONST_STORAGE  GUI_FONT GUI_FontCN32 = {
    GUI_FONTTYPE_PROP_USER,
    32,
    32,
    1,
    1,
    (void GUI_CONST_STORAGE *) &GUI_FontCN32_PropASC
};

GUI_CONST_STORAGE  GUI_FONT GUI_FontCN32x2 = {
    GUI_FONTTYPE_PROP_USER,
    32,
    32,
    2,
    2,
    (void GUI_CONST_STORAGE *) &GUI_FontCN32_PropASC
};

/*********************************************************/

U8 GUI_FontDataBuf[BYTES_PER_FONT];

static void GUI_GetDataFromMemory(const GUI_FONT_PROP GUI_UNI_PTR *pProp, U16 c)
{
    U8 cn_high, cn_low;
    U32 cn_offset;
    U8 t;
    U8 size, cn_size;
    U16 BytesPerFont;
    GUI_FONT EMWINFONT = *GUI_pContext->pAFont;
    U16 bread;

    BytesPerFont = GUI_pContext->pAFont->YSize * pProp->paCharInfo->BytesPerLine;

    if (BytesPerFont > BYTES_PER_FONT)
        BytesPerFont = BYTES_PER_FONT;

    if (memcmp(&EMWINFONT, &GUI_FontCN12, sizeof(GUI_FONT)) == 0)
        size = 12;
    else if (memcmp(&EMWINFONT, &GUI_FontCN16, sizeof(GUI_FONT)) == 0)
        size = 16;
    else if (memcmp(&EMWINFONT, &GUI_FontCN24, sizeof(GUI_FONT)) == 0)
        size = 24;
    else if (memcmp(&EMWINFONT, &GUI_FontCN32, sizeof(GUI_FONT)) == 0)
        size = 32;

    cn_size = (size / 8 + ((size % 8) ? 1 : 0)) * size;
    memset(GUI_FontDataBuf, 0, cn_size);

    if (c < 0x80) {
        switch (size) {
            case 12:
                for (t = 0; t < 12; t++) {
                    GUI_FontDataBuf[t] = 0;    //emwin_asc2_1206[c-0x20][t];
                }

                break;

            case 16:
                for (t = 0; t < 16; t++) {
                    GUI_FontDataBuf[t] = 0;    //emwin_asc2_1608[c-0x20][t];
                }

                break;

            case 24:
                for (t = 0; t < 48; t++) {
                    GUI_FontDataBuf[t] = 0;    //emwin_asc2_2412[c-0x20][t];
                }

                break;

            case 32:
                f_open(&USERFile, (const TCHAR *)"0:/Fonts/ASCII32Arial.bin", FA_READ);
                f_lseek(&USERFile, (c - 32) * 64);
                f_read(&USERFile, GUI_FontDataBuf, 64, (UINT *)&bread);
                f_close(&USERFile);
                break;
        }

        if (c == '\r' || c == '\n')
            memset(GUI_FontDataBuf, 0, cn_size);
    } else {
        cn_low = c / 256;
        cn_high = c % 256;

        if (cn_high < 0x81 || cn_low < 0x40 || cn_low == 0xFF || cn_high == 0xFF) {
            memset(GUI_FontDataBuf, 0, cn_size);
            return;
        }

        if (cn_low < 0x7F)
            cn_low -= 0x40;
        else
            cn_low -= 0x41;

        cn_high -= 0x81;
        cn_offset = ((U32)190 * cn_high + cn_low) * cn_size;

        switch (size) {
            case 12:
                f_open(&USERFile, (const TCHAR *)"0:/Fonts/GBK12.FON", FA_READ);
                f_lseek(&USERFile, cn_offset);
                f_read(&USERFile, GUI_FontDataBuf, cn_size, (UINT *)&bread);
                f_close(&USERFile);
                break;

            case 16:
                f_open(&USERFile, (const TCHAR *)"0:/Fonts/GBK16.FON", FA_READ);
                f_lseek(&USERFile, cn_offset);
                f_read(&USERFile, GUI_FontDataBuf, cn_size, (UINT *)&bread);
                f_close(&USERFile);
                break;

            case 24:
                f_open(&USERFile, (const TCHAR *)"0:/Fonts/GBK24.FON", FA_READ);
                f_lseek(&USERFile, cn_offset);
                f_read(&USERFile, GUI_FontDataBuf, cn_size, (UINT *)&bread);
                f_close(&USERFile);
                break;

            case 32:
                f_open(&USERFile, (const TCHAR *)"0:/Fonts/GBK32.FON", FA_READ);
                f_lseek(&USERFile, cn_offset);
                f_read(&USERFile, GUI_FontDataBuf, cn_size, (UINT *)&bread);
                f_close(&USERFile);
                break;
        }
    }
}
/*********************************************************************
*
*       Public code
*
**********************************************************************
*/
/*********************************************************************
*
*       GUIPROP_DispChar
*
* Purpose:
*   This is the routine that displays a character. It is used by all
*   other routines which display characters as a subroutine.
*/
void GUIPROP_X_DispChar(U16 c)
{
    int BytesPerLine;
    GUI_DRAWMODE DrawMode = GUI_pContext->TextMode;
    const GUI_FONT_PROP GUI_UNI_PTR *pProp = GUI_pContext->pAFont->p.pProp;

    for (; pProp; pProp = pProp->pNext) {
        if ((c >= pProp->First) && (c <= pProp->Last))
            break;
    }

    if (pProp) {
        GUI_DRAWMODE OldDrawMode;
        const GUI_CHARINFO GUI_UNI_PTR *pCharInfo = pProp->paCharInfo;
        GUI_GetDataFromMemory(pProp, c);
        BytesPerLine = pCharInfo->BytesPerLine;
        OldDrawMode  = LCD_SetDrawMode(DrawMode);
        LCD_DrawBitmap(GUI_pContext->DispPosX, GUI_pContext->DispPosY,
            pCharInfo->XSize, GUI_pContext->pAFont->YSize,
            GUI_pContext->pAFont->XMag, GUI_pContext->pAFont->YMag,
            1,     /* Bits per Pixel */
            BytesPerLine,
            &GUI_FontDataBuf[0],
            &LCD_BKCOLORINDEX
        );

        /* Fill empty pixel lines */
        if (GUI_pContext->pAFont->YDist > GUI_pContext->pAFont->YSize) {
            int YMag = GUI_pContext->pAFont->YMag;
            int YDist = GUI_pContext->pAFont->YDist * YMag;
            int YSize = GUI_pContext->pAFont->YSize * YMag;

            if (DrawMode != LCD_DRAWMODE_TRANS) {
                LCD_COLOR OldColor = GUI_GetColor();
                GUI_SetColor(GUI_GetBkColor());
                LCD_FillRect(GUI_pContext->DispPosX, GUI_pContext->DispPosY + YSize,
                    GUI_pContext->DispPosX + pCharInfo->XSize,
                    GUI_pContext->DispPosY + YDist);
                GUI_SetColor(OldColor);
            }
        }

        LCD_SetDrawMode(OldDrawMode); /* Restore draw mode */
        GUI_pContext->DispPosX += pCharInfo->XDist * GUI_pContext->pAFont->XMag;
    }
}

/*********************************************************************
*
*       GUIPROP_GetCharDistX
*/
int GUIPROP_X_GetCharDistX(U16 c)
{
    const GUI_FONT_PROP GUI_UNI_PTR *pProp = GUI_pContext->pAFont->p.pProp;

    for (; pProp; pProp = pProp->pNext) {
        if ((c >= pProp->First) && (c <= pProp->Last))
            break;
    }

    return (pProp) ? (pProp->paCharInfo)->XSize * GUI_pContext->pAFont->XMag : 0;
}

/*********************************************************************
*
*       _GetCharCode
*
* Purpose:
*   Return the UNICODE character code of the current character.
*/
static U16 _GetCharCode(const char GUI_UNI_PTR *s)
{
    if ((*s) >= 0x81)
        return *(const U16 GUI_UNI_PTR *)s;

    return *(const U8 GUI_UNI_PTR *)s;
}

/*********************************************************************
*
*       _GetCharSize
*
* Purpose:
*   Return the number of bytes of the current character.
*/
static int _GetCharSize(const char GUI_UNI_PTR *s)
{
    GUI_USE_PARA(s);

    if ((*s) >= 0x81)
        return 2;

    return 1;
}

/*********************************************************************
*
*       _CalcSizeOfChar
*
* Purpose:
*   Return the number of bytes needed for the given character.
*/
static int _CalcSizeOfChar(U16 Char)
{
    GUI_USE_PARA(Char);

    if (Char > 0x4081)
        return 2;

    return 1;
}

/*********************************************************************
*
*       _Encode
*
* Purpose:
*   Encode character into 1/2/3 bytes.
*/
static int _Encode(char *s, U16 Char)
{
    if (Char > 0x4081) {
        *((U16 *)s) = (U16)(Char);
        return 2;
    }

    *s = (U8)(Char);
    return 1;
}

/*********************************************************************
*
*       Static data
*
*       _API_Table
*/
const GUI_UC_ENC_APILIST GUI_UC_None = {
    _GetCharCode,     /*  return character code as U16 */
    _GetCharSize,     /*  return size of character: 1 */
    _CalcSizeOfChar,  /*  return size of character: 1 */
    _Encode           /*  Encode character */
};

const GUI_UC_ENC_APILIST GUI__API_TableNone = {
    _GetCharCode,     /*  return character code as U16 */
    _GetCharSize,     /*  return size of character: 1 */
    _CalcSizeOfChar,  /*  return size of character: 1 */
    _Encode
};
