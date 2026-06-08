#include "main.h"
#include "delay.h"
#include "stm32f1xx.h"
//extern void delay(__IO uint32_t nCount);
//extern void delay_ms(uint16_t nms);  

uint8_t led_step = 0; 

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == K1_Pin)
    {
        delay_ms(5); /* Èí¼þÏû¶¶ */
        if (HAL_GPIO_ReadPin(K1_GPIO_Port, K1_Pin) == GPIO_PIN_RESET) 
        {
            /* µÈ´ý°´¼üËÉ¿ª£¨±ÜÃâ³¤°´Á¬Ðø´¥·¢£© */
            while (HAL_GPIO_ReadPin(K1_GPIO_Port, K1_Pin) == GPIO_PIN_RESET);

            /* ×´Ì¬»úÇÐ»»Âß¼­£º0 -> 1 -> 2 -> 3 -> 0 */
            led_step = (led_step + 1) % 4;

            /*¹æÂÉ¿ØÖÆ LED (¼ÙÉè RESET ÎªÁÁ£¬SET ÎªÃð) */
            switch (led_step)
            {
                case 0: // ×´Ì¬0£ºD1ÁÁ£¬D2ÁÁ
                    HAL_GPIO_WritePin(L1_GPIO_Port, L1_Pin, GPIO_PIN_RESET);
                    HAL_GPIO_WritePin(L2_GPIO_Port, L2_Pin, GPIO_PIN_RESET);
                    break;
                case 1: // ×´Ì¬1£ºD1ÁÁ£¬D2Ãð
                    HAL_GPIO_WritePin(L1_GPIO_Port, L1_Pin, GPIO_PIN_RESET);
                    HAL_GPIO_WritePin(L2_GPIO_Port, L2_Pin, GPIO_PIN_SET);
                    break;
                case 2: // ×´Ì¬2£ºD1Ãð£¬D2ÁÁ
                    HAL_GPIO_WritePin(L1_GPIO_Port, L1_Pin, GPIO_PIN_SET);
                    HAL_GPIO_WritePin(L2_GPIO_Port, L2_Pin, GPIO_PIN_RESET);
                    break;
                case 3: // ×´Ì¬3£ºD1Ãð£¬D2Ãð
                    HAL_GPIO_WritePin(L1_GPIO_Port, L1_Pin, GPIO_PIN_SET);
                    HAL_GPIO_WritePin(L2_GPIO_Port, L2_Pin, GPIO_PIN_SET);
                    break;
            }
        }
    }
}
