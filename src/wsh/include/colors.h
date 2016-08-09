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

