#include "peripheral/bsp_led.h"
#include "string.h"
#include "stdlib.h"

//LED Handle list head.
static struct LED_TypeDef* LED_HeadHandler = NULL;

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/LEDn_Type/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "task/bsp_timtask.h"

GPIO_TypeDef* GPIO_PORT[LEDn] = {GPIOD,
                                 GPIOG,
                                 GPIOG,
                                 GPIOG
                                 };

const uint16_t GPIO_PIN[LEDn] = {GPIO_PIN_7,
                                 GPIO_PIN_10,
                                 GPIO_PIN_13,
                                 GPIO_PIN_14
                                 };

/**
  * @brief  Configures LED GPIO.
  * @param  LED: Specifies the LED to be configured. 
  *   This parameter can be one of following parameters:
  *     @arg LEDx
  */
void BSP_LED_Init(LEDn_Type LED)
{
  GPIO_InitTypeDef  GPIO_InitStruct;
  
  /* Enable the GPIO_LED Clock */
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /* Configure the GPIO_LED pin */
  GPIO_InitStruct.Pin = GPIO_PIN[LED];
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
  
  HAL_GPIO_Init(GPIO_PORT[LED], &GPIO_InitStruct);
  
  HAL_GPIO_WritePin(GPIO_PORT[LED], GPIO_PIN[LED], GPIO_PIN_RESET); 
}

/**
  * @brief  Start the LED work, add the handle into work list.
  * @param  btn: target handle strcut.
  * @retval 0: succeed. -1: already exist.
  */
int BSP_Add_LED(struct LED_TypeDef* Handle, uint8_t UniqueID, char *Color)
{
  struct LED_TypeDef* target = LED_HeadHandler;
  
  Handle->UniqueID = UniqueID;
  Handle->Color = (char *)malloc(strnlen(Color, 10));
  if(Handle->Color != NULL)
  {
    strncpy(Handle->Color, Color, 10);
  }
  else
  {
    return -1;
  }
  
  while(target) 
  {
    if(target == Handle) 
      return -1;  //already exist.
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
void BSP_Remove_LED(struct LED_TypeDef* Handle)
{
  struct LED_TypeDef** curr;
  
  Handle->UniqueID = 0;
  free(Handle->Color);
  
  for(curr = &LED_HeadHandler; *curr;) 
  {
    struct LED_TypeDef* entry = *curr;
    if (entry == Handle) 
    {
      *curr = entry->Next;
      //free(entry);
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
void BSP_LED_On(LEDn_Type LED)
{
  HAL_GPIO_WritePin(GPIO_PORT[LED], GPIO_PIN[LED], GPIO_PIN_SET); 
}

/**
  * @brief  Turns selected LED Off.
  * @param  LED: Specifies the LED to be set off. 
  *   This parameter can be one of following parameters:
  *     @arg LEDx
  */
void BSP_LED_Off(LEDn_Type LED)
{
  HAL_GPIO_WritePin(GPIO_PORT[LED], GPIO_PIN[LED], GPIO_PIN_RESET); 
}

/**
  * @brief  Toggles the selected LED.
  * @param  LED: Specifies the LED to be toggled. 
  *   This parameter can be one of following parameters:
  *     @arg LEDx  
  */
void BSP_LED_Toggle(LEDn_Type LED)
{
  HAL_GPIO_TogglePin(GPIO_PORT[LED], GPIO_PIN[LED]);
}

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

#if 0
struct TimTask_TypeDef LEDTask;

void LEDTask_Callback(void)
{
  BSP_LED_Toggle(LED1);
  BSP_LED_Toggle(LED2);
  BSP_LED_Toggle(LED3);
  BSP_LED_Toggle(LED4);
}

void LEDTask_Init(void)
{
  TimTask_Init(&LEDTask, LEDTask_Callback, 1000, 1000); //1s loop
  TimTask_Start(&LEDTask);
}

void LED_Test(void)
{
  LEDTask_Init();
}
#endif
