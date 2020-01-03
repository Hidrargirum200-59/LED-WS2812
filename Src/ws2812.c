#include "ws2812.h"

uint16_t BUF_DMA [ARRAY_LEN] = {0};
extern TIM_HandleTypeDef htim2;

void ws2812_init(void)
{
  int i;
  for(i=DELAY_LEN;i<ARRAY_LEN;i++) BUF_DMA[i] = LOW;
}

void ws2812_pixel_rgb_to_buf_dma(uint8_t Rpixel , uint8_t Gpixel, uint8_t Bpixel, uint16_t posX)
{
	volatile uint16_t i;
	for (i = 0; i < 8; i++)
	{
		if(BitIsSet(Rpixel, (7 - i)) == 1)
				BUF_DMA[DELAY_LEN+posX*24+i+8] = HIGH;
			else
				BUF_DMA[DELAY_LEN+posX*24+i+8] = LOW;
			
		if (BitIsSet(Gpixel, (7 - i)) == 1)
				BUF_DMA[DELAY_LEN+posX*24+i+0] = HIGH;
			else
				BUF_DMA[DELAY_LEN+posX*24+i+0] = LOW;
			
		if (BitIsSet(Bpixel, (7 - i)) == 1)
				BUF_DMA[DELAY_LEN+posX*24+i+16] = HIGH;
			else
				BUF_DMA[DELAY_LEN+posX*24+i+16] = LOW;
	}
}

void ws2812_single_led(uint8_t red, uint8_t green, uint8_t blue, uint16_t pos)
{
	ws2812_pixel_rgb_to_buf_dma(red, green, blue, pos);
	HAL_TIM_PWM_Start_DMA(&htim2,TIM_CHANNEL_2,(uint32_t*)&BUF_DMA,ARRAY_LEN);
}
void ws2812_all_leds(uint8_t red, uint8_t green, uint8_t blue)
{
	for (uint16_t i = 0; i < LED_COUNT; i++)
		ws2812_single_led(red, green, blue, i);
}

void ws2812_led_light()
{
	HAL_TIM_PWM_Start_DMA(&htim2,TIM_CHANNEL_2,(uint32_t*)&BUF_DMA,ARRAY_LEN);
}

void ws2812_pattern_2(uint8_t r0, uint8_t g0, uint8_t b0,
											uint8_t r1, uint8_t g1, uint8_t b1)
{
	for (uint16_t num = 0; num < 300; num++)
	{
		ws2812_single_led(r0, g0, b0, num);
		num++;
		ws2812_single_led(r1, g1, b1, num);
	}
}

void ws2812_pattern(const Colour* colours, uint16_t chunk_size) //DO NOT USE IMPLICITLY!
{
	for(uint16_t num = 0;  num < 300; num++ )
		ws2812_single_led(colours[num % chunk_size].red, colours[num % chunk_size].green, colours[num % chunk_size].blue, num);
}

// '2#_0_0_8#0_8_0#'
void ws2812_set_pattern(char* input_line)
{
	char line[200];
	strcpy(line, (char*)input_line);
	char* subline = strtok(line, "#");
	int array_size = strtol(subline, NULL, 10);
	if ((array_size <= 0) || (array_size > LED_COUNT))
		return;
	Colour* cs = (Colour*)malloc(array_size*sizeof(Colour)); //SEE YA IN HELL
	for (int i = 0; i < array_size; i++)
	{
		subline = strtok(NULL, "#");
		if (subline == NULL)
		{
			Colour tempcol = {0, 0, 0};
			cs[i] = tempcol;
 		} 
		else
		{
			Colour tempcol = {0, 0, 0};
			long code = strtol(subline, NULL, 10);
		/*	for(int k = 0; k < 3; k++)
			{
				char* numcode = strtok((char*)subline, "_");
				int code = atoi(numcode);
				if ((code < 0)||(code > 64))
					code = 0;
				switch (k){
					case 0: 
						tempcol.red = code;
					break;
					case 1:
						tempcol.green = code;
					break;
					case 2:
						tempcol.blue = code;
					break;
					}
			} */
			int rcode = code / 1000000;
			int gcode = (code / 1000) % 1000;
			int bcode = code % 1000;
			if ((rcode > 32)||(rcode<0))
				cs[i].red = 0;
			else 
				cs[i].red = rcode;
			if ((gcode > 32)||(gcode<0))
				cs[i].green = 0;
			else 
				cs[i].green = gcode;
			if ((bcode > 32)||(bcode<0))
				cs[i].blue = 0;
			else 
				cs[i].blue = bcode;
			cs[i] = tempcol;
		}
	}
	ws2812_pattern(cs, array_size);
	free(cs);
}

void ws2812_light_cascade(uint8_t red, uint8_t green, uint8_t blue, uint16_t init_pos, uint16_t number)
{
	for (int i = init_pos; ((i < init_pos + number)&&(i < LED_COUNT)); i++ )
		ws2812_single_led(red, green, blue, i);
}

void ws2812_light_pattern(char* pattern_line)
{
		char str[300];
	strcpy(str, (char*)pattern_line);
	char* subline = strtok(str, "$_ ");
		int array_size = atoi(subline);
	if ((array_size <= 0) || (array_size > LED_COUNT))
		return;
	Colour* cs = (Colour*)calloc(array_size, sizeof(Colour)); //SEE YA IN HELL
	int cc = 0;
	for (int i = 0; ((i< array_size*3)&&(subline != NULL)); i++)
	{
		subline = strtok(NULL, "$_ ");
		if (strstr(subline, "\r\n"))
			continue;
		int code = atoi(subline);
		if ((code > 32)||(code<0))
				code = 0;
		if (cc == 0)
		{
			cs[i / 3].red = code;
			cc++;
		}
		else if (cc == 1)
		{
			cs[i / 3].green = code;
			cc++;
		}
		else if (cc == 2)
		{
			cs[i / 3].blue = code;
			cc = 0;
		}
	}
	ws2812_pattern(cs, array_size);
	free(cs);
	return;
}
void ws2812_light_from_string(char* istr)
{
	char str[160];
	strcpy(str, (char*)istr);
	char* subline = strtok(str, "$_ ");
	if (!strstr(subline, "CMD"))
		return;
	subline = strtok(NULL, "$_ ");
	if (strstr(subline, "LED"))
	{
		subline = strtok(NULL, "$_ ");
		int led_num = atoi(subline);
		if ((led_num <= 0) || (led_num > LED_COUNT))
			return;
		Colour cs;
		for (int i = 0; ((i < 3)&&(subline != NULL)); i++)
		{
			subline = strtok(NULL, "$_ ");
			if (strstr(subline, "\r\n"))
				continue;
			int code = atoi(subline);
			if ((code > SAFE_LEVEL)||(code<0))
				code = 0;
			if (i == 0)
				cs.red = code;
			else if (i == 1)
				cs.green = code;
			else if (i == 2)
				cs.blue = code;
		}
		ws2812_single_led(cs.red, cs.green, cs.blue, led_num);
		return;
	}
	else if (strstr(subline, "PAT")) // Pattern lighting
	{
		subline = strtok(NULL, "$_ ");
		int array_size = atoi(subline);
	if ((array_size <= 0) || (array_size > LED_COUNT))
		return;
	Colour* cs = (Colour*)calloc(array_size, sizeof(Colour)); //SEE YA IN HELL
	int cc = 0;
	for (int i = 0; ((i< array_size*3)&&(subline != NULL)); i++)
	{
		subline = strtok(NULL, "$_ ");
		if (strstr(subline, "\r\n"))
			continue;
		int code = atoi(subline);
		if ((code > 32)||(code<0))
				code = 0;
		if (cc == 0)
		{
			cs[i / 3].red = code;
			cc++;
		}
		else if (cc == 1)
		{
			cs[i / 3].green = code;
			cc++;
		}
		else if (cc == 2)
		{
			cs[i / 3].blue = code;
			cc = 0;
		}
	}
	ws2812_pattern(cs, array_size);
	free(cs);
	return;
	}
	else if (strstr(subline, "DEL"))
	{
		subline = strtok(NULL, "$_ ");
		int delay_ms = atoi(subline);
		if ((delay_ms <= 0) || (delay_ms > 60000))
			return;
		HAL_Delay(delay_ms);
		return;
	}
	else if (strstr(subline, "OFF"))
	{
		ws2812_all_leds(0, 0, 0);
		return;
	}
	else if (strstr(subline, "SET"))
	{
		subline = strtok(NULL, "$_ ");
		int init_led = atoi(subline);
		if ((init_led <= 0) || (init_led > LED_COUNT))
			return;
		subline = strtok(NULL, "$_ ");
		int led_num = atoi(subline);
		if ((init_led <= 0) || (init_led > LED_COUNT))
			return;
		Colour cs;
		for (int i = 0; ((i < 3)&&(subline != NULL)); i++)
		{
			subline = strtok(NULL, "$_ ");
			if (strstr(subline, "\r\n"))
				continue;
			int code = atoi(subline);
			if ((code > SAFE_LEVEL)||(code<0))
				code = 0;
			if (i == 0)
				cs.red = code;
			else if (i == 1)
				cs.green = code;
			else if (i == 2)
				cs.blue = code;
		}
		ws2812_light_cascade(cs.red, cs.green, cs.blue, init_led, led_num);
	}
	else if (strstr(subline, "ALL"))
	{
		Colour cs;
		for (int i = 0; ((i < 3)&&(subline != NULL)); i++)
		{
			subline = strtok(NULL, "$_ ");
			if (strstr(subline, "\r\n"))
				continue;
			int code = atoi(subline);
			if ((code > SAFE_LEVEL)||(code<0))
				code = 0;
			if (i == 0)
				cs.red = code;
			else if (i == 1)
				cs.green = code;
			else if (i == 2)
				cs.blue = code;
		}
		ws2812_light_cascade(cs.red, cs.green, cs.blue, 0, LED_COUNT);
	}
}

