// ==============================================================================
// 
// ff-utils - Common utilities by Fede
// 
// Federico Francescon <federico.francescon@higeco.com>
// 2015.11.09 - v0.0.1
// ==============================================================================


#ifndef FFUTILS_H
#define FFUTILS_H

// ~~~~~~~~~~~~~~~~~~~~~ INCLUDES
#include <stdarg.h>
#include <stdint.h>

#include <time.h>


// ~~~~~~~~~~~~~~~~~~~~~ DEFINITIONS
//#define EXIT_SUCCESS 0
#define ERR_GENERAL 		1
#define LINE_MAX_LENGTH 2048
#define CMD_MAX_ARGS 		256
#define LIST_LEN 				256
#define _TRUE_ 					1
#define _FALSE_ 				0
#define TS_NSEC_LIMIT 	1000000000		// Nanoseconds limit
#define MAX_FN_NAME 		64						// Max Length for function name( used for benchmarks )
#define ITEM_BYTES_MAX 	16            // MAX numer of bytes supported for a multitype 16 => 8bit * 16 = 128bits

// Term colors
#define COL_RESET         "\e[0m"      // RESET terminal style 
#define COL_BRIGHT_RED    "\e[0;91m"   // bright red
#define COL_BRIGHT_CYAN   "\e[0;96m"   // bright cyan
#define COL_BRIGHT_GREEN  "\e[0;92m"   // bright green
#define COL_BRIGHT_YELLOW "\e[0;93m"   // bright yellow
#define COL_BRIGHT_BLACK  "\e[0;90m"   // brigh black
#define COL_BRIGHT_BLUE   "\e[0;94m"   // brigh blue
#define COL_BRIGHT_PURPLE "\e[0;95m"   // brigh purple
#define COL_BRIGHT_WHITE  "\e[0;97m"   // brigh white

// Debug levels
#define DBG_NONE  0 	// No output at all!
#define DBG_ERR   1 	// Error
#define DBG_WAR   2 	// Warning
#define DBG_INF   3 	// Informations
#define DBG_VER   4 	// Verbose
#define DBG_DBG   5 	// Debug

// ~~~~~~~~~~~~~~~~~~~~~ STRUCTURES DEFINITIONS
// Doubly linked list node structure
typedef struct list list;
struct list {
	// char	name[ MAX_FN_NAME ];
	int 	id;
  char* element;
  list* prev;
  list* next;
};

// Tree structure that uses lists definition
typedef struct tree tree;
struct tree {
	// char	name[ MAX_FN_NAME ];
	int   id;
  char* element;
  tree* root;
  list* leaves;
};

// I use this to accept multiple types of input, to easily store the bit representation on my value
// ad read it as a uint8_t or uint16_t without casting problems that could change my item value
// this is needed because I could have multiple input formats but modbus wants only unsigned ints
// Best and cleanest way I've found to catch that without C automatically casting floats to int or vice-versa
union multitypes{
  short s;
  int   i;
  long  l;
  unsigned short     us;
  unsigned int       ui;
  unsigned long      ul;
  unsigned long long ull;

  float       f;
  double      d;
  long double ld;

  char c[ITEM_BYTES_MAX];

  // used as output interface here in modbus
  uint8_t   ui8;
  uint16_t  ui16;
  uint32_t  ui32;
};


// ~~~~~~~~~~~~~~~~~~~~~ LISTS FUNCTIONS
// Create a new empty list and returns it's address
list*   list_new();

// Destroying a whole list, calling function with pointer dfAdrr and passing pointer to pointer to element
void    list_destroy( list** l, void* dfAdrr );

// Append pointer to new element to list l
void    list_append( list** l, char* e );

// Moves an element of a list before a specified one
void    list_move_before( list** head, list* before, list* toMove );

// Removes given node from the list and returns pointer to it
list* 	list_remove( list** rl );

// Returns pointer to first element of a list
list*   list_get_first( list* l );

// Returns pointer to last element of a list
list*   list_get_last( list* l );

// Returns pointer to n-th node of a list( first index is 0 )
list*   list_index( list* l, int i );

// Return number of elements in a list
int     list_len( list* l );

// Tails second list to the first one
void    list_catenate( list* l_1, list* l_2 );


// ~~~~~~~~~~~~~~~~~~~~~ TREE FUNCTIONS
// Create a new empty tree and returns it's address
tree*		tree_new();

// Destroying a whole tree starting from given node, calling function with pointer dfAdrr and passing pointer to pointer to element
void    tree_destroy( tree** t, void* dfAdrr );

// Destroy all leaves in given tree
void   	tree_empty( tree* t, void* dfAdrr );

// Append a new leave to given tree
tree*   tree_addleaf( tree* t, char* el );


// ~~~~~~~~~~~~~~~~~~~~~ PARSING FUNCTIONS
// Parse a line identifying words divide by white space char
char**  argv_parse( char* line );

// Splits a string at a given separator and returns a list pointer containing all single strings( CHANGE ORIGINAL STR )
list* 	str_split( char* str, char separator );

// return true if given str is prefixed by the second given string false otherwise
int 		str_checkprefix( const char *str, const char *pfx );


// ~~~~~~~~~~~~~~~~~~~~~ LOG FUNCTIONS
// Sets debug level for messageds and colors
void set_debug( int lvl );
void set_msg_colors( int on ); 

// Return _TRUE_ if colors are on
int is_msg_colors();
// Return debug level as int
int get_debug();

// Prints output to console
void msg( int lvl, const char* fname, const char *logstr , ... );


// ~~~~~~~~~~~~~~~~~~~~~ BENCHMARK FUNCTIONS
// Gets difference between two times
struct timespec ts_diff( struct timespec start, struct timespec end );

// Return sum between of times
struct timespec ts_sum( struct timespec ts1, struct timespec ts2 );

// Return formatted string for given timespec
void ts_tostr( struct timespec ts, char* dest_str );

// Struct handler for each single calls
typedef struct bench_fcall{
	struct timespec start;		// Start of function call
	struct timespec end;			// End of function call
}	bench_fcall;

// Struct handler for each function
typedef struct bench_function{
	list* calls;								// List Storing each single call to that function
	char 	name[ MAX_FN_NAME ];	// Function name
}	bench_function;

// Adds a new function call to the proper list and sets start time
bench_fcall* bench_start( char* fname );

// Sets end time of a function call
void bench_end( bench_fcall* fc );

// Prints benchmarks
void bench_print();

// Destroy and releases all memory used
void bench_destroy();
	




#endif