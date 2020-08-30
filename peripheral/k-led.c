#include "peripheral/k-led.h"
#include "algorithm/k-slist.h"
#include "string.h"
#include "stdlib.h"
#include "linux/gfp.h"
#include "core/k-memory.h"

//LED Handle list head.
static struct LED_TypeDef *LED_HeadHandler = NULL;

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/LEDn_Type/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

//GPIO_TypeDef* GPIO_PORT[LEDn] = {GPIOD,
//                                 GPIOG,
//                                 GPIOG,
//                                 GPIOG
//                                 };

//const uint16_t GPIO_PIN[LEDn] = {GPIO_PIN_7,
//                                 GPIO_PIN_10,
//                                 GPIO_PIN_13,
//                                 GPIO_PIN_14
//                                 };

GPIO_TypeDef *GPIO_PORT[LEDn] = {GPIOC,
        GPIOC,
        GPIOC,
        GPIOC
    };

const uint16_t GPIO_PIN[LEDn] = {GPIO_PIN_6,
        GPIO_PIN_7,
        GPIO_PIN_8,
        GPIO_PIN_9
    };

/**
  * @brief  Configures LED GPIO.
  * @param  LED: Specifies the LED to be configured.
  *   This parameter can be one of following parameters:
  *     @arg LEDx
  */
void K_LED_Init(LEDn_Type LED)
{
    GPIO_InitTypeDef  GPIO_InitStruct;

    /* Enable the GPIO_LED Clock */
    __HAL_RCC_GPIOG_CLK_ENABLE();

    /* Configure the GPIO_LED pin */
    GPIO_InitStruct.Pin = GPIO_PIN[LED];
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(GPIO_PORT[LED], &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIO_PORT[LED], GPIO_PIN[LED], GPIO_PIN_RESET);
}

/**
  * @brief  Start the LED work, add the handle into work list.
  * @param  btn: target handle strcut.
  * @retval 0: succeed. -1: already exist.
  */
int K_Add_LED(struct LED_TypeDef *Handle, uint8_t UniqueID, char *Color)
{
    struct LED_TypeDef *target = LED_HeadHandler;

    Handle->UniqueID = UniqueID;
    Handle->Color = (char *)kmalloc(strlen(Color), __GFP_ZERO);

    if(Handle->Color != NULL)
        strncpy(Handle->Color, Color, 10);
    else
        return -1;

    while(target)
    {
        if(target == Handle)
        {
            return -1;    //already exist.
        }

        target = target->Next;
    }

    Handle->Next = LED_HeadHandler;
    LED_HeadHandler = Handle;
    return 0;
}

/**
  * @brief  Stop the LED work, remove the handle off work list.
  * @param  Handle: target handle strcut.
  * @retval None
  */
void K_Remove_LED(struct LED_TypeDef *Handle)
{
    struct LED_TypeDef **curr;

    Handle->UniqueID = 0;
    kfree(Handle->Color);

    for(curr = &LED_HeadHandler; *curr;)
    {
        struct LED_TypeDef *entry = *curr;

        if(entry == Handle)
        {
            *curr = entry->Next;
            //kfree(entry);
        }
        else
            curr = &entry->Next;
    }
}

/**
  * @brief  Turns selected LED On.
  * @param  LED: Specifies the LED to be set on.
  *   This parameter can be one of following parameters:
  *     @arg LEDx
  */
void K_LED_On(LEDn_Type LED)
{
    HAL_GPIO_WritePin(GPIO_PORT[LED], GPIO_PIN[LED], GPIO_PIN_SET);
}

/**
  * @brief  Turns selected LED Off.
  * @param  LED: Specifies the LED to be set off.
  *   This parameter can be one of following parameters:
  *     @arg LEDx
  */
void K_LED_Off(LEDn_Type LED)
{
    HAL_GPIO_WritePin(GPIO_PORT[LED], GPIO_PIN[LED], GPIO_PIN_RESET);
}

/**
  * @brief  Toggles the selected LED.
  * @param  LED: Specifies the LED to be toggled.
  *   This parameter can be one of following parameters:
  *     @arg LEDx
  */
void K_LED_Toggle(LEDn_Type LED)
{
    HAL_GPIO_TogglePin(GPIO_PORT[LED], GPIO_PIN[LED]);
}

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

#ifdef DESIGN_VERIFICATION_LED
#include "dv/k-test.h"
#include "task/k-timtask.h"

struct TimTask_TypeDef LEDTask;

void LEDTask_Callback(void)
{
    K_LED_Toggle(LED1);
    K_LED_Toggle(LED2);
    K_LED_Toggle(LED3);
    K_LED_Toggle(LED4);
}

void LEDTask_Init(void)
{
    TimTask_Init(&LEDTask, LEDTask_Callback, 1000, 1000); //1s loop
    TimTask_Start(&LEDTask);
}

void LEDTask_Deinit(void)
{
    TimTask_Stop(&LEDTask);
}

int t_LED_Toggle(int argc, char **argv)
{
    LEDTask_Init();

    return PASS;
}

int t_LED_Delete(int argc, char **argv)
{
    LEDTask_Deinit();

    return PASS;
}
#endif
