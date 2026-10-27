/*
 * ili9341.c
 *
 *  Created on: Aug 20, 2024
 *      Author: Pablo Mazariegos
 */
#include <stdlib.h> // malloc()
#include <string.h> // memset()
#include "stm32f4xx_ll_spi.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_gpio.h"
#include "pgmspace.h"
#include "ili9341.h"
#include "main.h"

/*  RST 	- PC1
 *  RS/DC 	- PA4
 *  CS  	- PB0
 *  MOSI 	- PA7
 *  MISO 	- PA6
 *  SCK 	- PA5
 * */

#define LCD_DC_PORT    GPIOC
#define LCD_DC_PIN     GPIO_PIN_1
#define LCD_DC_L()     (LCD_DC_PORT->BSRR = (LCD_DC_PIN<<16))	//
#define LCD_DC_H()     (LCD_DC_PORT->BSRR =  LCD_DC_PIN)		//Poner el modo RS en high es para mandar dato

#define LCD_CS_PORT    GPIOB
#define LCD_CS_PIN     GPIO_PIN_6
#define LCD_CS_L()     (LCD_CS_PORT->BSRR = (LCD_CS_PIN<<16)) 	//Pone el chip select en activo para empezar la comunicación con la pantalla
#define LCD_CS_H()     (LCD_CS_PORT->BSRR =  LCD_CS_PIN)		//

extern const uint8_t smallFont[1140];
extern const uint16_t bigFont[1520];
extern SPI_HandleTypeDef hspi1;
//***************************************************************************************************************************************
// Función para inicializar LCD
//***************************************************************************************************************************************
void LCD_Init(void)
{
    // Estado inicial de los pines
    LCD_CS_H();
    LCD_DC_H();

    // Reiniciar pantalla (PA4)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
    HAL_Delay(10);

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
    HAL_Delay(20);

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
    HAL_Delay(150);

    // Secuencia de inicializacion ILI9341

    LCD_CMD(0xEF);
    LCD_DATA(0x03);
    LCD_DATA(0x80);
    LCD_DATA(0x02);

    LCD_CMD(0xCF);
    LCD_DATA(0x00);
    LCD_DATA(0xC1);
    LCD_DATA(0x30);

    LCD_CMD(0xED);
    LCD_DATA(0x64);
    LCD_DATA(0x03);
    LCD_DATA(0x12);
    LCD_DATA(0x81);

    LCD_CMD(0xE8);
    LCD_DATA(0x85);
    LCD_DATA(0x00);
    LCD_DATA(0x78);

    LCD_CMD(0xCB);
    LCD_DATA(0x39);
    LCD_DATA(0x2C);
    LCD_DATA(0x00);
    LCD_DATA(0x34);
    LCD_DATA(0x02);

    LCD_CMD(0xF7);
    LCD_DATA(0x20);

    LCD_CMD(0xEA);
    LCD_DATA(0x00);
    LCD_DATA(0x00);

    // Control de alimentacion
    LCD_CMD(0xC0);
    LCD_DATA(0x23);

    LCD_CMD(0xC1);
    LCD_DATA(0x10);

    // Control de voltaje VCOM
    LCD_CMD(0xC5);
    LCD_DATA(0x3E);
    LCD_DATA(0x28);

    LCD_CMD(0xC7);
    LCD_DATA(0x86);

    // Orientacion horizontal: 320 x 240
    LCD_CMD(0x36);
    LCD_DATA(0x28);

    // Color RGB565 (16 bits)
    LCD_CMD(0x3A);
    LCD_DATA(0x55);

    // Frecuencia de refresco
    LCD_CMD(0xB1);
    LCD_DATA(0x00);
    LCD_DATA(0x18);

    // Control de pantalla
    LCD_CMD(0xB6);
    LCD_DATA(0x08);
    LCD_DATA(0x82);
    LCD_DATA(0x27);

    // Gamma
    LCD_CMD(0xF2);
    LCD_DATA(0x00);

    LCD_CMD(0x26);
    LCD_DATA(0x01);

    // Salir del modo reposo
    LCD_CMD(0x11);
    HAL_Delay(150);

    // Encender pantalla
    LCD_CMD(0x29);
    HAL_Delay(50);

    LCD_CS_H();
}
//***************************************************************************************************************************************
// Función para enviar comandos a la LCD - parámetro (comando)
//***************************************************************************************************************************************
void LCD_CMD(uint8_t cmd) {
    LCD_CS_L();
    LCD_DC_L();

    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);

    LCD_CS_H();
}

//***************************************************************************************************************************************
// Función para enviar datos a la LCD - parámetro (dato)
//***************************************************************************************************************************************
void LCD_DATA(uint8_t data) {
    LCD_CS_L();
    LCD_DC_H();

    HAL_SPI_Transmit(&hspi1, &data, 1, 100);

    LCD_CS_H();
}

//***************************************************************************************************************************************
// Función para definir rango de direcciones de memoria con las cuales se trabajara (se define una ventana)
//***************************************************************************************************************************************
void SetWindows(unsigned int x1, unsigned int y1, unsigned int x2,
		unsigned int y2) {
	LCD_CMD(0x2a); // Set_column_address 4 parameters
	LCD_DATA(x1 >> 8);
	LCD_DATA(x1);
	LCD_DATA(x2 >> 8);
	LCD_DATA(x2);
	LCD_CMD(0x2b); // Set_page_address 4 parameters
	LCD_DATA(y1 >> 8);
	LCD_DATA(y1);
	LCD_DATA(y2 >> 8);
	LCD_DATA(y2);
	LCD_CMD(0x2c); // Write_memory_start
}
//***************************************************************************************************************************************
// Función para borrar la pantalla - parámetros (color)
//***************************************************************************************************************************************
void LCD_Clear(unsigned int c) {
	unsigned int x, y;
//	HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
//	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
	LCD_DC_H();
	LCD_CS_L();
	SetWindows(0, 0, 319, 239);

	for (x = 0; x < 320; x++)
		for (y = 0; y < 240; y++) {
			LCD_DATA(c >> 8);
			LCD_DATA(c);
		}
//	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
	LCD_CS_H();

}
//***************************************************************************************************************************************
// Función para dibujar una línea horizontal - parámetros ( coordenada x, cordenada y, longitud, color)
//***************************************************************************************************************************************
void H_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c) {
	unsigned int i;
	LCD_CMD(0x02c); //write_memory_start
	//	HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
	//	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
	LCD_DC_H();
	LCD_CS_L();
	//l = l + x;
	SetWindows(x, y, l + x, y);
	//j = l; // * 2;
	for (i = 0; i < l; i++) {
		LCD_DATA(c >> 8);
		LCD_DATA(c);
	}
	//	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
	LCD_CS_H();
}
//***************************************************************************************************************************************
// Función para dibujar una línea vertical - parámetros ( coordenada x, cordenada y, longitud, color)
//***************************************************************************************************************************************
void V_line(unsigned int x, unsigned int y, unsigned int l, unsigned int c) {
	unsigned int i;
	LCD_CMD(0x02c); //write_memory_start
	//	HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
	//	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
	LCD_DC_H();
	LCD_CS_L();
	//l = l + y;
	SetWindows(x, y, x, y + l);
	//j = l; //* 2;
	for (i = 1; i <= l; i++) {
		LCD_DATA(c >> 8);
		LCD_DATA(c);
	}
	//	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
	LCD_CS_H();
}
//***************************************************************************************************************************************
// Función para dibujar un rectángulo - parámetros ( coordenada x, cordenada y, ancho, alto, color)
//***************************************************************************************************************************************
void Rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h,
		unsigned int c) {
	H_line(x, y, w, c);
	H_line(x, y + h, w, c);
	V_line(x, y, h, c);
	V_line(x + w, y, h, c);
}
//***************************************************************************************************************************************
// Función para dibujar un rectángulo relleno - parámetros ( coordenada x, cordenada y, ancho, alto, color)
//***************************************************************************************************************************************
void FillRect(unsigned int x, unsigned int y, unsigned int w, unsigned int h,
		unsigned int c) {
	LCD_CMD(0x02c); // write_memory_start
	//	HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
	//	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
	LCD_DC_H();
	LCD_CS_L();

	SetWindows(x, y, x + w - 1, y + h - 1);
	unsigned int k = w * h * 2 - 1;
	for (int i = 0; i < w; i++) {
		for (int j = 0; j < h; j++) {
			LCD_DATA(c >> 8);
			LCD_DATA(c);

			//LCD_DATA(bitmap[k]);
			k = k - 2;
		}
	}
	//	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
	LCD_CS_H();
}
//***************************************************************************************************************************************
// Función para dibujar texto - parámetros ( texto, coordenada x, cordenada y, color, background)
//***************************************************************************************************************************************
void LCD_Print(char *text, int x, int y, int fontSize, int color,
		int background) {

	int fontXSize;
	int fontYSize;

	if (fontSize == 1) {
		fontXSize = fontXSizeSmal;
		fontYSize = fontYSizeSmal;
	}
	if (fontSize == 2) {
		fontXSize = fontXSizeBig;
		fontYSize = fontYSizeBig;
	}
	if (fontSize == 3) {
		fontXSize = fontXSizeNum;
		fontYSize = fontYSizeNum;
	}

	char charInput;
	int cLength = strlen(text);
	int charDec;
	int c;
	//int charHex;
	char char_array[cLength + 1];
	for (int i = 0; text[i] != '\0'; i++) {
		char_array[i] = text[i];
	}

	//text.toCharArray(char_array, cLength + 1);

	for (int i = 0; i < cLength; i++) {
		charInput = char_array[i];
		charDec = (int) charInput;
		//HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
		LCD_CS_L();
		SetWindows(x + (i * fontXSize), y, x + (i * fontXSize) + fontXSize - 1,
				y + fontYSize);
		long charHex1;
		for (int n = 0; n < fontYSize; n++) {
			if (fontSize == 1) {
				charHex1 = pgm_read_word_near(
						smallFont + ((charDec - 32) * fontYSize) + n);
			}
			if (fontSize == 2) {
				charHex1 = pgm_read_word_near(
						bigFont + ((charDec - 32) * fontYSize) + n);
			}
			for (int t = 1; t < fontXSize + 1; t++) {
				if ((charHex1 & (1 << (fontXSize - t))) > 0) {
					c = color;
				} else {
					c = background;
				}
				LCD_DATA(c >> 8);
				LCD_DATA(c);
			}
		}
		//HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
		LCD_CS_H();
	}
}
//***************************************************************************************************************************************
// Función para dibujar una imagen a partir de un arreglo de colores (Bitmap) Formato (Color 16bit R 5bits G 6bits B 5bits)
//***************************************************************************************************************************************
void LCD_Bitmap(unsigned int x, unsigned int y, unsigned int width,
		unsigned int height, const uint16_t *bitmap) {
	//LCD_CMD(0x02c); // write_memory_start
	//	HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
	//	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
	LCD_DC_H();
	LCD_CS_L();

	SetWindows(x, y, x + width - 1, y + height - 1);
	unsigned int total_pixels = width * height;

	// Enviar datos en bloques si es posible
	for (unsigned int i = 0; i < total_pixels; i++) {
		uint16_t pixel = bitmap[i];
		LCD_DATA(pixel >> 8);
		LCD_DATA(pixel & 0xFF);
	}
	//HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
	LCD_CS_H();
}
//***************************************************************************************************************************************
// Función para dibujar una imagen a partir de un arreglo de colores (Bitmap) Formato (Color 16bit R 5bits G 6bits B 5bits) con color transparente
//***************************************************************************************************************************************
void LCD_BitmapTransparent(uint16_t x, uint16_t y, uint16_t width,
		uint16_t height, const uint16_t *bitmap, uint16_t transparentColor) {
	for (unsigned int j = 0; j < height; j++) {
		for (unsigned int i = 0; i < width; i++) {
			unsigned int index = j * width + i;
			uint16_t pixel = bitmap[index];

			if (pixel != transparentColor) {
				// Comandos mínimos para un solo píxel
				LCD_CS_L();
				SetWindows(x + i, y + j, x + i, y + j);
				LCD_CMD(0x02c);
				LCD_DC_H();
				LCD_DATA(pixel >> 8);
				LCD_DATA(pixel & 0xFF);
				LCD_CS_H();
			}
		}
	}
}
//***************************************************************************************************************************************
// Función para dibujar una imagen sprite - los parámetros columns = número de imagenes en el sprite, index = cual desplegar, flip = darle vuelta
//***************************************************************************************************************************************
void LCD_Sprite(int x, int y, int width, int height, const uint16_t *bitmap,
		int columns, int index, char flip, char offset) {
	//LCD_CMD(0x02c); // write_memory_start
	//	HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
	//	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
	LCD_DC_H();
	LCD_CS_L();

	SetWindows(x, y, x + width - 1, y + height - 1);

	int ancho = width * columns;
	int k = 0;

	if (flip) {
		for (int j = 0; j < height; j++) {
			k = j * ancho + index * width - 1 - offset;
			k = k + width;
			for (int i = 0; i < width; i++) {
				uint16_t pixel = bitmap[k];
				LCD_DATA(pixel >> 8);
				LCD_DATA(pixel & 0xFF);
				k--;
			}
		}
	} else {
		for (int j = 0; j < height; j++) {
			k = j * ancho + index * width + 1 + offset;
			for (int i = 0; i < width; i++) {
				uint16_t pixel = bitmap[k];
				LCD_DATA(pixel >> 8);
				LCD_DATA(pixel & 0xFF);
				k++;
			}
		}
	}

	//HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
	LCD_CS_H();
}

