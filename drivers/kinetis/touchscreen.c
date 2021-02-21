#include "kinetis/touchscreen.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */
#include "GUI_Type.h"
#include "kinetis/gt9271.h"
#include "GUI.h"

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#define TS_SWAP_NONE                    0x00
#define TS_SWAP_X                       0x01
#define TS_SWAP_Y                       0x02
#define TS_SWAP_XY                      0x04
#define TS_I2C_ADDRESS                  0x82

/**
  * @brief  Provide the GUI with current state of the touch screen
  * @param  None
  * @retval None
  */
void K_Pointer_Update(void)
{
    GUI_PID_STATE TS_State;
    static TS_StateTypeDef prev_state;
    TS_StateTypeDef  ts;
    u16 xDiff, yDiff;

    K_TS_GetState(&ts);

    TS_State.Pressed = ts.TouchDetected;

    xDiff = (prev_state.X > ts.X) ? (prev_state.X - ts.X) : (ts.X - prev_state.X);
    yDiff = (prev_state.Y > ts.Y) ? (prev_state.Y - ts.Y) : (ts.Y - prev_state.Y);

    if (ts.TouchDetected) {
        if ((prev_state.TouchDetected != ts.TouchDetected) ||
            (xDiff > 3) ||
            (yDiff > 3)) {
            prev_state = ts;

            TS_State.Layer = 0;
            TS_State.x = ts.X;
            TS_State.y = ts.Y;

            GUI_TOUCH_StoreStateEx(&TS_State);
        }
    }
}

static TS_DrvTypeDef     *TsDrv;
static u16          TsXBoundary, TsYBoundary;

/**
  * @brief  Initializes and configures the touch screen functionalities and
  *         configures all necessary hardware resources (GPIOs, clocks..).
  * @param  XSize: The maximum X size of the TS area on LCD
  * @param  YSize: The maximum Y size of the TS area on LCD
  * @retval TS_OK: if all initializations are OK. Other value if error.
  */
//u8 K_TS_Init(u16 XSize, u16 YSize)
//{
//  u8 ret = TS_ERROR;
//
//  /* Initialize x and y positions boundaries */
//  TsXBoundary = XSize;
//  TsYBoundary = YSize;
//
//  /* Read ID and verify if the IO expander is ready */
//  if(gt9271_ts_drv.ReadID(TS_I2C_ADDRESS) == GT9271_ID)
//  {
//    /* Initialize the TS driver structure */
//    TsDrv = &gt9271_ts_drv;
//
//    ret = TS_OK;
//  }
//
//  if(ret == TS_OK)
//  {
//    /* Initialize the LL TS Driver */
//    TsDrv->Init(TS_I2C_ADDRESS);
//    TsDrv->Start(TS_I2C_ADDRESS);
//  }
//
//  return ret;
//}

/**
  * @brief  Configures and enables the touch screen interrupts.
  * @retval TS_OK: if ITconfig is OK. Other value if error.
  */
u8 K_TS_ITConfig(void)
{
    /* Enable the TS ITs */
    TsDrv->EnableIT(TS_I2C_ADDRESS);

    return TS_OK;
}

/**
  * @brief  Gets the TS IT status.
  * @retval Interrupt status.
  */
u8 K_TS_ITGetStatus(void)
{
    /* Return the TS IT status */
    return (TsDrv->GetITStatus(TS_I2C_ADDRESS));
}

/**
  * @brief  Returns status and positions of the touch screen.
  * @param  TsState: Pointer to touch screen current state structure
  */
void K_TS_GetState(TS_StateTypeDef *TsState)
{
    static u32 _x = 0, _y = 0;
    u16 xDiff, yDiff, x, y, xr, yr;

    TsState->TouchDetected = TsDrv->DetectTouch(TS_I2C_ADDRESS);

    if (TsState->TouchDetected) {
        TsDrv->GetXY(TS_I2C_ADDRESS, &x, &y);

        /* Y value first correction */
        y -= 360;

        /* Y value second correction */
        yr = y / 11;

        /* Return y position value */
        if (yr <= 0)
            yr = 0;
        else if (yr > TsYBoundary)
            yr = TsYBoundary - 1;
        else
        {}

        y = yr;

        /* X value first correction */
        if (x <= 3000)
            x = 3870 - x;
        else
            x = 3800 - x;

        /* X value second correction */
        xr = x / 15;

        /* Return X position value */
        if (xr <= 0)
            xr = 0;
        else if (xr > TsXBoundary)
            xr = TsXBoundary - 1;
        else
        {}

        x = xr;
        xDiff = x > _x ? (x - _x) : (_x - x);
        yDiff = y > _y ? (y - _y) : (_y - y);

        if (xDiff + yDiff > 5) {
            _x = x;
            _y = y;
        }

        /* Update the X position */
        TsState->X = _x;

        /* Update the Y position */
        TsState->Y = _y;
    }
}

/**
  * @brief  Clears all touch screen interrupts.
  */
void K_TS_ITClear(void)
{
    /* Clear TS IT pending bits */
    TsDrv->ClearIT(TS_I2C_ADDRESS);
}

u16 TP_State = 0;
u16 TP_X_Coordinates[10], TP_Y_Coordinates[10];
extern const u16 GT9271_TPX_TBL[10];

u8 TP_Scan(u8 mode)
{
    u8 buf[6];
    volatile u8 i = 0;
    u8 res = 0;
    u16 temp;
    u16 tempsta;
    static u8 t = 0;

    t++;

    if ((t % 10) == 0 || t < 10) {
        GTP_ReadCurrentTSCase(&mode);

        if (mode & 0x80 && ((mode & 0x0F) < 11)) {
            i = 0;
            GTP_WriteCurrentTSCase(i);
        }

        if ((mode & 0x0F) && ((mode & 0x0F) < 11)) {
            temp = 0xFFFF << (mode & 0x0F);
            tempsta = TP_State;
            TP_State = (~temp) | 0x80 | 0x40;
            TP_X_Coordinates[9] = TP_X_Coordinates[0];
            TP_Y_Coordinates[9] = TP_Y_Coordinates[0];

            for (i = 0; i < 10; i++) {
                if (TP_State & (1 << i)) {
                    GTP_ReadCurrentTSPoint(GT9271_TPX_TBL[i], buf, 6);
//          if(tp_dev.touchtype & 0x01)
//          {
                    TP_Y_Coordinates[i] = ((u16)buf[3] << 8) + buf[2];
                    TP_X_Coordinates[i] = ((u16)buf[1] << 8) + buf[0];
//          }
//          else
//          {
//            TP_X_Coordinates[i] = 800-(((u16)buf[3] << 8) + buf[2]);
//            TP_Y_Coordinates[i] = ((u16)buf[1] << 8) + buf[0];
//          }
                    //printk("x[%d]:%d,y[%d]:%d\r\n",i,TP_X_Coordinates[i],i,TP_Y_Coordinates[i]);
                }
            }

            res = 1;

            if (TP_X_Coordinates[0] > GTP_MAX_WIDTH || TP_Y_Coordinates[0] > GTP_MAX_HEIGHT) {
                if ((mode & 0x0F) > 1) {
                    TP_X_Coordinates[0] = TP_X_Coordinates[1];
                    TP_Y_Coordinates[0] = TP_Y_Coordinates[1];
                    t = 0;
                } else {
                    TP_X_Coordinates[0] = TP_X_Coordinates[9];
                    TP_Y_Coordinates[0] = TP_Y_Coordinates[9];
                    mode = 0x80;
                    TP_State = tempsta;
                }
            } else
                t = 0;
        }
    }

    if ((mode & 0x8F) == 0x80) {
        if (TP_State & 0x80)
            TP_State &= ~0x80;
        else {
            TP_X_Coordinates[0] = 0xffff;
            TP_Y_Coordinates[0] = 0xffff;
            TP_State &= 0xE000;
        }
    }

    if (t > 240)
        t = 10;

    return res;
}

/**
  * @brief EXTI line detection callbacks
  * @param GPIO_Pin: Specifies the pins connected EXTI line
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(u16 GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_7) {
//    GTP_TouchProcess();
    }
}
void GUI_TOUCH_X_ActivateX(void)
{

}


void GUI_TOUCH_X_ActivateY(void)
{

}

int  GUI_TOUCH_X_MeasureX(void)
{
    int32_t xvalue;

    TP_Scan(0);
    xvalue = TP_X_Coordinates[0];

    return xvalue;
}

int  GUI_TOUCH_X_MeasureY(void)
{
    int32_t yvalue;

    TP_Scan(0);
    yvalue = TP_Y_Coordinates[0];

    return yvalue;
}
#ifdef DESIGN_VERIFICATION_TOUCHSCREEN
{"test", fuction},
#endif