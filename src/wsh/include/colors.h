/**
*
* Witchcraft Compiler Collection
*
* Author: Jonathan Brossard - endrazine@gmail.com
*
*******************************************************************************
* The MIT License (MIT)
* Copyright (c) 2016-2025 Jonathan Brossard
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*******************************************************************************
*
*/

// Bold + color
#define RED	"\033[1;31m"
#define CYAN	"\033[1;36m"
#define GREEN	"\033[1;32m"
#define BLUE	"\033[1;34m"
#define BLACK	"\033[1;30m"
#define BROWN	"\033[1;33m"
#define MAGENTA	"\033[1;35m"
#define GRAY	"\033[1;37m"
#define DARKGRAY "\033[1;30m"
#define YELLOW	"\033[1;33m"

// Normal text
#define NORMAL	"\033[0m"        /* flush the previous properties */


#define CLEAR	"\033[2J"



/*
such as printf("\033[8;5Hhello");  // Move to (8, 5) and output hello

other commands:

printf("\033[XA"); // Move up X lines;
printf("\033[XB"); // Move down X lines;
printf("\033[XC"); // Move right X column;
printf("\033[XD"); // Move left X column;
printf("\033[2J"); // Clear screen
*/

