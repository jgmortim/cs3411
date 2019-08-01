/* ----------------------------------------------------------- */
/* NAME : John Mortimroe                     User ID: jgmortim */
/* DUE DATE : 04/21/2019                                       */
/* PROGRAM ASSIGNMENT 4                                        */
/* FILE NAME : memoryship.c                                    */
/* PROGRAM PURPOSE :                                           */
/*    Memory Mapped Battle Ship.                               */
/* ----------------------------------------------------------- */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/* Can be set to desired values. */
#define GRIDWIDTH 5
#define GRIDHEIGHT 5
#define MAXSHIPS 5

#define SIZEGRID sizeof(char)*(GRIDWIDTH*GRIDHEIGHT)
#define SIZEINT sizeof(int)


void setupShips(int PID);
int fire(int player, int x, int y);
int RNG(int low, int high);
void printGrid();
void printWinner();
int verifyMem();
void usageError();
void mmapError();
void forkError();
void openError();

char (*p1Grid)[GRIDWIDTH];	/* Player 1's grid. */
char (*p2Grid)[GRIDWIDTH];	/* player 2's grid. */
int *p1Ships;	/* Player 1's number of remaining ships. */
int *p2Ships;	/* Player 2's number of remaining ships. */
int *turn;		/* Odd for player 1; even for player 2. */

/* ----------------------------------------------------------- */
/* FUNCTION : main                                             */
/*    The main functino for the program.                       */
/* PARAMETER USAGE :                                           */
/*    argc {int} - number of argument.                         */
/*    argv {char**} - list of arguments.                       */
/*        argv[1] - time in microseconds between turns.        */
/* FUNCTIONS CALLED :                                          */
/*    atoi(), close(), exit(), fire(), fork(), getpid(),mmap(),*/
/*    open(), printGrid(), printWinner(), RNG(), setupShips(), */
/*    srand(), usleep(), wait()                                */
/* ----------------------------------------------------------- */
int main(int argc, char *argv[]) {
	int fd, pid, x, y, turnTime;

	/* If incorrect number of arguments, print usage and exit. */
	if (argc != 2) {
		usageError();
		return 1;
    }
	turnTime=atoi(argv[1]); 
	/* Get file descriptor. */
	if ((fd = open("/dev/zero", O_RDWR))==-1){
		openError();
		return 1;
	}
	/* Get shared memory. */
	p1Grid = (char (*)[GRIDWIDTH]) mmap(0, SIZEGRID, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	p2Grid = (char (*)[GRIDWIDTH]) mmap(0, SIZEGRID, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	p1Ships = (int*) mmap(0, SIZEINT, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	p2Ships = (int*) mmap(0, SIZEINT, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	turn = (int*) mmap(0, SIZEINT, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);
	if (verifyMem()==1) {
		mmapError();
		return 1;
	}

	/* Set up grid */
	for (y=0; y<GRIDHEIGHT; y++) {
		for (x=0; x<GRIDWIDTH; x++) {
			p1Grid[x][y]='.';
			p2Grid[x][y]='.';
		}
	}
	*p1Ships=0;
	*p2Ships=0;
	*turn=-1;

	/* Set up the ships and seed the RNG. */
	if ((pid=fork()) == -1) {
		forkError();
		return 1;
	}
	srand(getpid());
	setupShips(pid);
	
	(*turn)++;
	/* Wait until both players are ready to prevent race conditions and deadlock. */
	while (*turn != 1); 
	if (pid == 0) { /* Child: Player 2. */
		while (*p1Ships > 0) {
			while (*turn % 2 != 0); 	/* Wait your turn. */
			if (*p2Ships == 0) break; 	/* Break if you've lost. */
			/* Fire at the enemy. Don't fire at the same spot twice. */
			while (fire(2, RNG(0, GRIDWIDTH-1), RNG(0, GRIDHEIGHT-1))==-1);
			printGrid();				/* Print the result. */
			usleep(turnTime);			/* Wait the required time. */
			(*turn)++;					/* End your turn. */
		}
		exit(0);
	} else { /* Parent: Player 1. */
		while (*p2Ships > 0) {
			while (*turn % 2 != 1); 	/* Wait your turn. */
			if (*p1Ships == 0) break; 	/* Break if you've lost. */
			/* Fire at the enemy. Don't fire at the same spot twice. */
			while (fire(1, RNG(0, GRIDWIDTH-1), RNG(0, GRIDHEIGHT-1))==-1);
			printGrid(); 				/* Print the result. */
			usleep(turnTime); 			/* Wait the required time. */
			(*turn)++; 					/* End your turn. */
		}
	}
	(*turn)++;				/* Ensure no players deadlock. */
	(*turn)++;				/* Ensure no players deadlock. */
	wait(&pid);				/* Wait for the child to exit. */
	printWinner();			/* Declare the winner and quit. */
	return 0;
}

/* ----------------------------------------------------------- */
/* FUNCTION : printGrid                                        */
/*    Prints out the grid (game board).                        */
/* PARAMETER USAGE :                                           */
/*    N/A                                                      */
/* FUNCTIONS CALLED :                                          */
/*    sprintf(), strlen(), write()                             */
/* ----------------------------------------------------------- */
void printGrid() {
	char output[1024];
	int x,y;
	/* Print grid for Player 1. */
	for (y=0; y<GRIDHEIGHT; y++) {
		for (x=0; x<GRIDWIDTH; x++) {
			sprintf(output, "%c", p1Grid[x][y]);
			if (x != GRIDWIDTH-1) sprintf(output+strlen(output), " ");
			write(1, output, strlen(output));
		}
	write(1, "\n", strlen("\n"));
	}
	/* Print the divider. */
	for (x=0; x<GRIDWIDTH; x++) {
		write(1, "= ", strlen("= "));
	}
	write(1, "\n", strlen("\n"));
	/* Print grid for Player 2. */
	for (y=0; y<GRIDHEIGHT; y++) {
		for (x=0; x<GRIDWIDTH; x++) {
			sprintf(output, "%c", p2Grid[x][y]);
			if (x != GRIDWIDTH-1) sprintf(output+strlen(output), " ");
			write(1, output, strlen(output));
		}
		write(1, "\n", strlen("\n"));
	}
	/* Print extra line to separate the grid between rounds. */
	write(1, "\n", strlen("\n"));
}

/* ----------------------------------------------------------- */
/* FUNCTION : printWinner                                      */
/*    prints out the winning player.                           */
/* PARAMETER USAGE :                                           */
/*    N/A                                                      */
/* FUNCTIONS CALLED :                                          */
/*    sprintf(), strlen(), write()                             */
/* ----------------------------------------------------------- */
void printWinner() {
	char output[1024];
	if (*p1Ships == 0) sprintf(output, "\nPlayer 2 wins!\n");
	if (*p2Ships == 0) sprintf(output, "\nPlayer 1 wins!\n");
	write(1, output, strlen(output));
}

/* ----------------------------------------------------------- */
/* FUNCTION : fire                                             */
/*    player fires at position (x,y). Returns -1 if the player */
/*    has already fired at (x,y).                              */
/* PARAMETER USAGE :                                           */
/*    player {int} - the player that fires.                    */
/*    x {int} - the x position to fire at.                     */
/*    y {int} - the y position to fire at.                     */
/* FUNCTIONS CALLED :                                          */
/*    N/A                                                      */
/* ----------------------------------------------------------- */
int fire(int player, int x, int y) {
	if (player == 2) {  /* Player 2 fires. */
		if (p1Grid[x][y] == '*' || p1Grid[x][y] == 'X') {
			return -1;
		} else if (p1Grid[x][y] == '.') { /* Miss. */
			p1Grid[x][y] = '*';
		} else if (p1Grid[x][y] == 'O') { /* Hit. */
			(*p1Ships)--;
			p1Grid[x][y] = 'X';
		}
	} else {  /* Player 1 fires. */
		if (p2Grid[x][y] == '*' || p2Grid[x][y] == 'X') {
			return -1;
		} else if (p2Grid[x][y] == '.') { /* Miss. */
			p2Grid[x][y] = '*';
		} else if (p2Grid[x][y] == 'O') { /* Hit. */
			(*p2Ships)--;
			p2Grid[x][y] = 'X';
		}
	}
	return 0;
}

/* ----------------------------------------------------------- */
/* FUNCTION : setupShips                                       */
/*    Places ships in the grid.                                */
/* PARAMETER USAGE :                                           */
/*    PID {int} - the pid of the player that is placing ships. */
/*                0 for the child process.                     */
/* FUNCTIONS CALLED :                                          */
/*    RNG()                                                    */
/* ----------------------------------------------------------- */
void setupShips(int PID) {
	int x, y;
	if (PID != 0) { /* Player 1, parent. */
		while (*p1Ships < MAXSHIPS) {
				x = RNG(0, GRIDWIDTH-1);
				y = RNG(0, GRIDHEIGHT-1);
				if (p1Grid[x][y] != 'O') {
					p1Grid[x][y] = 'O';
					(*p1Ships)++;
				}
		}
	} else {		/* Player 2, child. */
		while (*p2Ships < MAXSHIPS) {
				x = RNG(0, GRIDWIDTH-1);
				y = RNG(0, GRIDHEIGHT-1);
				if (p2Grid[x][y] != 'O') {
					p2Grid[x][y] = 'O';
					(*p2Ships)++;
				}
		}
	}
}

/* ----------------------------------------------------------- */
/* FUNCTION : RNG                                              */
/*    Random number generator with limits.                     */
/* PARAMETER USAGE :                                           */
/*    low {int} - lower bound.                                 */
/*    high {int} - upper bound.                                */
/* FUNCTIONS CALLED :                                          */
/*    rand()                                                   */
/* ----------------------------------------------------------- */
int RNG(int low, int high) {
	return rand() % (high + 1 - low) + low;
}

/*---------------------------------------------*/
/* Error checking and messages below this line */
/*---------------------------------------------*/

/* ----------------------------------------------------------- */
/* FUNCTION : verifyMem                                        */
/*    verifies that the mmap memory was acquired. Returns 1 on */
/*    failure; 0 on success.                                   */
/* PARAMETER USAGE :                                           */
/*    N/A                                                      */
/* FUNCTIONS CALLED :                                          */
/*    N/A                                                      */
/* ----------------------------------------------------------- */
int verifyMem() {
	if (p1Grid == MAP_FAILED) return 1;
	if (p2Grid == MAP_FAILED) return 1;
	if (p1Ships == MAP_FAILED) return 1;
	if (p2Ships == MAP_FAILED) return 1;
	if (turn == MAP_FAILED) return 1;
	return 0;
}

/* ----------------------------------------------------------- */
/* FUNCTION : usageError                                       */
/*    Called when an program in not invoked properly.          */
/* PARAMETER USAGE :                                           */
/*    N/A                                                      */
/* FUNCTIONS CALLED :                                          */
/*    strlen(), write()                                        */
/* ----------------------------------------------------------- */
void usageError() {
	char *error1="Usage :\nmemoryship <turn time in microseconds> \n";
	write(1, error1, strlen(error1));
}


/* ----------------------------------------------------------- */
/* FUNCTION : forkError                                        */
/*    Called when an error occures with fork().                */
/* PARAMETER USAGE :                                           */
/*    N/A                                                      */
/* FUNCTIONS CALLED :                                          */
/*    strlen(), write()                                        */
/* ----------------------------------------------------------- */
void forkError() {
	char *error1="Error encountered while trying to create the second process.\n";
	char *error2="Execution terminated.\n";
	write(1, error1, strlen(error1));
	write(1, error2, strlen(error2));
}

/* ----------------------------------------------------------- */
/* FUNCTION : mmapError                                        */
/*    Called when an error occures with mmap().                */
/* PARAMETER USAGE :                                           */
/*    N/A                                                      */
/* FUNCTIONS CALLED :                                          */
/*    strlen(), write()                                        */
/* ----------------------------------------------------------- */
void mmapError() {
	char *error1="Error encountered using mmap().\n";
	char *error2="Execution terminated.\n";
	write(1, error1, strlen(error1));
	write(1, error2, strlen(error2));
}

/* ----------------------------------------------------------- */
/* FUNCTION : openError                                        */
/*    Called when an error occures with open().                */
/* PARAMETER USAGE :                                           */
/*    N/A                                                      */
/* FUNCTIONS CALLED :                                          */
/*    strlen(), write()                                        */
/* ----------------------------------------------------------- */
void openError() {
	char *error1="Error encountered using open().\n";
	char *error2="Execution terminated.\n";
	write(1, error1, strlen(error1));
	write(1, error2, strlen(error2));
}

