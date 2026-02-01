static void vRgbTask(void *pvParameters)
{
    const uint8_t color = RGB_CYAN;
	const TickType_t xPeriod = 100;
    TickType_t xDelay = xPeriod / 2;
	xil_printf("\nxPeriod: %d\n", xPeriod);

    while (1){
        XGpio_DiscreteWrite(&rgbLedInst, RGB_CHANNEL, color);
        vTaskDelay(xDelay);
        XGpio_DiscreteWrite(&rgbLedInst, RGB_CHANNEL, 0);
        vTaskDelay(xDelay);
    }
}