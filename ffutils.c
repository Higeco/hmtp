// ==============================================================================
// 
// ff-utils - Common utilities by Fede
// 
// Federico Francescon <federico.francescon@higeco.com>
// 2015.11.09 - v0.0.1
// ==============================================================================


// ~~~~~~~~~~~~~~~~~~~~~ INCLUDES
#include <unistd.h> // For system cores count (linux only)
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ffutils.h"


// ~~~~~~~~~~~~~~~~~~~~~ LIST FUNCTIONS
// Create a new empty list and returns it's address
int listid = 0;
list* list_new(){
  list* l = malloc( sizeof(list) );

  // l->name[0]  = '\0';

  l->id       = listid++;
  l->element  = NULL;
  l->next     = NULL;
  l->prev     = NULL;

  // printf( "list_new %d\n", (listid-1) );

  return l;
}

// Destroying a whole list, calling function with pointer dfAdrr and passing pointer to pointer to element
// If passing NULL function pointer element will not be cleaned
void list_destroy( list** l, void* dfAdrr ){
  if( l == NULL || *l == NULL ){ return; }

  void(*destroyFunc)( char** );
  destroyFunc = NULL;
  if( dfAdrr != NULL ){ destroyFunc = dfAdrr; }

  list  *tmp = NULL, 
        *dl = list_get_first( *l );

  while( dl != NULL ){
    // printf( "list destroy: '%s' %d %d \n", dl->name, dl->id, dl->next != NULL ? dl->next->id : -1 );

    //Calling destroy function for element within list node
    if( destroyFunc != NULL && dl->element != NULL ){ destroyFunc( &(dl->element) ); }
    // else{ printf( "Not destroy list element - func ? %s - el ? %s - id: %d\n", destroyFunc == NULL ? "NULL" : "ok", dl->element == NULL ? "NULL" : "ok", dl->id ); }

    tmp = dl;
    dl = dl->next;
    if( dl != NULL ){ dl->prev = NULL; }
    *l = dl;

    // // /*DBG-DELETE*/printf( "LIST DESTROY: '%s'\n", tmp->name );

    // Freeing malloc'ed memory
    tmp->next    = NULL;
    tmp->element = NULL;
    tmp->prev    = NULL;
    free( tmp );
  }

  // Set to NULL list pointer
  *l = NULL;
}

// Returns pointer to first element of a list
list* list_get_first( list* l ){
  if( l == NULL ){ return NULL; }

  while( l->prev != NULL ){ l = l->prev; }
  return l;
}

// Returns pointer to last element of a lis
list* list_get_last( list* l ){
  if( l == NULL ){ return NULL; }

  while( l->next != NULL){ l = l->next; }
  return l;
}

// Append pointer to new element to list l
// DO NOT BENCHMARK THIS FUNCTION BECAUSE IT'S CALLED INSIDE bench_start
void list_append( list** l, char* e ){
  if( l == NULL ){ return; }

  if( *l == NULL ){ 
    (*l) = list_new();
    (*l)->element = e;
  }
  else{
    list *tmpl = list_get_last( *l );
    tmpl->next = list_new();
    tmpl->next->element = e;
    tmpl->next->prev = tmpl;
  }
}

// Removes given node from the list and returns pointer to it
list* list_remove( list** rl ){ 
  if( rl == NULL || *rl == NULL ){ return NULL; }

  list *tmpl = *rl;

  if( tmpl->prev == NULL ){ *rl = tmpl->next; }
  else{ tmpl->prev->next = tmpl->next; }

  if( tmpl->next != NULL ){ tmpl->next->prev = tmpl->prev; }

  tmpl->next = NULL;
  tmpl->prev = NULL;
  return tmpl;
}

// Moves an element of a list before a specified one
void list_move_before( list** head, list* before, list* toMove ){
  if( head == NULL || *head == NULL || before == NULL || toMove == NULL ){ return; }

  if( toMove->next != NULL ){ toMove->next->prev = toMove->prev; }
  if( toMove->prev != NULL ){ toMove->prev->next = toMove->next; }
  
  toMove->prev = before->prev;
  toMove->next = before;
  before->prev = toMove;

  if( toMove->prev != NULL ){ toMove->prev->next = toMove; }
  else{ *head = toMove; }
}

// Returns pointer to n-th node of a list( first index is 0 )
list*  list_index( list* l, int i ){
  if( l == NULL ){ return NULL; }

  l = list_get_first( l );
  int x = 0;
  list* li = NULL;
  while( x <= i && l != NULL ){
    if( x == i ){ li = l; break; }
    
    x++;
    l = l->next;
  }

  return li;
}

// Return number of elements in a list
int list_len( list* l ){
  if( l == NULL ){ return 0; }

  l = list_get_first( l );
  int count = 0;
  while( l != NULL ){ count++; l = l->next; };

  return count;
}

// Tails second list to the first one
// Link first element of second list to last one of first list
void list_catenate( list* l_1, list* l_2 ){
  if( l_1 == NULL || l_2 == NULL ){ return; }

  // printf( "l_1: %d l_2: %d\n", l_1->id, l_2->id );

  list* end1    = list_get_last( l_1 );
  list* start2  = list_get_first( l_2 );

  end1->next = start2;
  start2->prev = end1;
}


// ~~~~~~~~~~~~~~~~~~~~~ TREE FUNCTIONS
// Create a new empty list and returns it's address
// 
int treeid = 0;
tree* tree_new(){
  tree* t     = malloc( sizeof(tree) );
  
  // t->name[0]  = '\0';

  t->id       = treeid++;
  t->element  = NULL;
  t->root     = NULL;
  t->leaves   = NULL;

  // printf( "tree init: %d\n", t->id );
  return t;
}

// Destroy all leaves in given tree
void tree_empty( tree* t, void* dfAdrr ){
  if( t == NULL ){ msg( DBG_DBG, "tree_empty", "NULL tree pointer" ); return; }
  // /*BENCHMARK*/ bench_fcall* bck = bench_start( "tree_empty" );
  for( list* l = list_get_first( t->leaves ); l != NULL; l = l->next ){ tree_destroy( (tree **)&(l->element), dfAdrr ); }
  list_destroy( &(t->leaves), NULL );
  // /*BENCHMARK*/ bench_end( bck );
}

// Append a new leave to given tree
tree* tree_addleaf( tree* t, char* el ){
  if( t == NULL ){ msg( DBG_DBG, "tree_addleaf", "NULL tree pointer" ); return NULL; }
  // /*BENCHMARK*/ bench_fcall* bck = bench_start( "tree_addleaf" );
  tree* newt    = tree_new();
  newt->root    = t;
  newt->element = el;
  list_append( &(t->leaves), (char *)newt );
  // /*BENCHMARK*/ bench_end( bck ); 
  return newt;
}

// Destroying a whole tree starting from given node, calling function with pointer dfAdrr and passing pointer to pointer to element
void tree_destroy( tree** t, void* dfAdrr ){
  if( t == NULL || *t == NULL ){ msg( DBG_DBG, "tree_destroy", "NULL tree pointer" ); return; }
  // /*BENCHMARK*/ bench_fcall* bck = bench_start( "tree_destroy" );
  
  void (*destroyFunc)( char** );
  destroyFunc = NULL;
  if( dfAdrr != NULL ){ destroyFunc = dfAdrr; }

  tree *realt = NULL; // USED TO STORE POINTER WHEN PASSED TREE HAS NOT THE REAL ONE
  tree *tmpt  = NULL;
  list *tmpl  = NULL,
       *dsl   = NULL;

  // 
  // CHECK this seems no not beeing used anymore
  // 
  // list *p = NULL, *n = NULL;
  
  // Trying to guess real treenode address by looking to his root leaves
  if( (*t)->root != NULL && (*t)->root->leaves != NULL ){
    for( list* rl = list_get_first( (*t)->root->leaves ); rl != NULL; rl = rl->next ){
      if( rl->element == (char *)(*t) ){
        msg( DBG_DBG, "tree_destroy", "Real t addr: %lu - given t: %lu", (unsigned long)&(rl->element), (unsigned long)t );
        realt = (tree *)rl->element;
        t = &realt;
        // rl->element = NULL; //     <--- ATTENTION! NULLING THE ELEMENT WILL SET POINTER t to NULL, leading to dangling reference
        tmpl = list_remove( &rl );
        list_destroy( &tmpl, NULL );
        break;
      }
    }
  }
  
  // Setting first tree on delete list and setting tu null pointer passed
  list_append( &dsl, (char *)*t );
  // /*DBG-DELETE*/strncpy( (list_get_last( dsl ))->name, "del list start", MAX_FN_NAME );

  for( list* l = dsl; l != NULL; l = l->next ){
    if( l->element == NULL ){ msg( DBG_DBG, "tree_destroy", "Destroy list element was NULL" ); continue; }

    tmpt = (tree *)l->element;

    // /*DBG-DELETE*/printf( "TREE DESTROY: '%s'\n", tmpt != NULL ? tmpt->name : "NULL" );
    
    tmpl = tmpt->leaves;
    while( tmpl != NULL ){
      if( tmpl->element != NULL ){
        list_append( &dsl, (char *)(tmpl->element) );
        // /*DBG-DELETE*/strncpy( (list_get_last( l ))->name, "appending here", MAX_FN_NAME );
        // /*DBG-DELETE*/printf( "td dsl added id: %d\n", (list_get_last( l ))->id );
        // /*DBG-DELETE*/printf( "Appended: %s\n", ((tree *)tmpl->element)->name );
      }
      
      tmpl->element = NULL;
      tmpl = tmpl->next;
    }
    
    // Destroying element
    if( tmpt->element != NULL && destroyFunc != NULL ){ destroyFunc( &(tmpt->element) ); }
    if( tmpt->leaves  != NULL ){ list_destroy( &(tmpt->leaves), NULL ); }

    tmpt->root    = NULL;
    tmpt->element = NULL;
    tmpt->leaves  = NULL;
    tmpt->id      = -1;
    
    // Free pointer
    free( l->element );
    l->element = NULL;
  }

  // Destroying Temp List
  list_destroy( &dsl, NULL );

  // Nulling pointer
  *t = NULL;

  // /*BENCHMARK*/ bench_end( bck );
}



// ~~~~~~~~~~~~~~~~~~~~~ STRING FUNCTIONS
// Parse a line identifying words divide by white space char
// This function reads a line char by char until a NULL TERMINATING or NEWLINE char is found
// When white spaces are found they are substituted by '' and pointer to first char of word is appendend to argv array
char** argv_parse( char* line ){
  // /*BENCHMARK*/ bench_fcall* bck = bench_start( "argv_parse" );
  static char* argv[ CMD_MAX_ARGS ];
  int argc = 0, i = 0, start = 0;

  while( line[i] != '\0' && line[i] != '\n' && argc < (CMD_MAX_ARGS -1) && i < LINE_MAX_LENGTH ){
    if( line[i] == ' ' ){
      line[i] = '\0';
      argv[argc] = line + start;
      argc++;
      start = i+1;
    }
    i++;
  }
  argc++;
  line[i] = '\0';
  argv[argc-1] = line + start; // Adding last command
  argv[argc] = '\0';

  // /*BENCHMARK*/ bench_end( bck );
  return &argv[0];
}

// Splits a string at a given separator and returns a list pointer containing all single strings
// N.B.: this will change original string
list* str_split( char* str, char separator ){
  // /*BENCHMARK*/ bench_fcall* bck = bench_start( "str_split" );
  list* splitlist = NULL;
  int i = 0, start = 0;

  while( str[i] != '\0' && i < LINE_MAX_LENGTH ){
    if( str[i] == separator ){
      str[i] = '\0';
      list_append( &splitlist, &str[start] );
      start = i+1;
    }
    i++;
  }
  if( str[i] != '\0' ){ str[i] = '\0'; }
  if( splitlist == NULL ){
    splitlist = list_new();
    splitlist->element = &str[start];

    // /*DBG-DELETE*/strncpy( splitlist->name, &str[start], MAX_FN_NAME );
  }
  else{
    list_append( &splitlist, &str[start] );
  }


  // /*BENCHMARK*/ bench_end( bck );
  return splitlist;
}

// return true if given str is prefixed by the second given string false otherwise
int str_checkprefix( const char *str, const char *pfx ){
  if( !(str  && pfx) ){ return _FALSE_; }
  size_t slen = strlen( str ),
         plen = strlen( pfx );

  return ( plen <= slen && strncmp( pfx, str, plen ) == 0 );
}


// ~~~~~~~~~~~~~~~~~~~~~ LOG FUNCTIONS
int msg_colors = _FALSE_;  // Global variable to set colored msg output
int msg_dbgl   = DBG_INF;  // Sets debug level, higher means more informations

// Sets debug level for messageds and colors
void set_debug( int lvl ){ msg_dbgl = lvl; }
void set_msg_colors( int on ){ msg_colors = (on == _TRUE_ ? _TRUE_ : _FALSE_); }

// Return _TRUE_ if colors are on
int is_msg_colors(){ return msg_colors; }
// Return debug level as int
int get_debug(){ return msg_dbgl; }

// Prints to stdout messages
void msg( int lvl, const char* fname, const char *logstr , ... ){
   if( msg_dbgl < lvl ){ return; }

  char    buff[20];
  struct  tm* tm_info;
  time_t  timer;
  int     multiline_pad_len = 20;

  // Printing timestamp
  time( &timer );
  tm_info = localtime( &timer );
  if( strftime( buff, sizeof buff, "%Y%m%d %H%M%S", tm_info ) ){
    printf( "[%s%s%s] ", ( msg_colors ? COL_BRIGHT_BLACK : "" ), buff, ( msg_colors ? COL_RESET : "" ) );
  }
 
  // Printing fname
  if( fname != NULL ){
    printf( "[%s%s%s] ", ( msg_colors ? COL_BRIGHT_BLACK : "" ), fname, ( msg_colors ? COL_RESET : "" ) );
    multiline_pad_len += strlen( fname ) + 3 ;
  }

  // Printing dbg level
  switch( lvl ){
    case DBG_ERR: printf( "[%sERR%s] ", ( msg_colors ? COL_BRIGHT_RED    : "" ), ( msg_colors ? COL_RESET : "" ) ); break;
    case DBG_WAR: printf( "[%sWAR%s] ", ( msg_colors ? COL_BRIGHT_YELLOW : "" ), ( msg_colors ? COL_RESET : "" ) ); break;
    case DBG_INF: printf( "[%sINF%s] ", ( msg_colors ? COL_BRIGHT_CYAN   : "" ), ( msg_colors ? COL_RESET : "" ) ); break;
    case DBG_VER: printf( "[%sVER%s] ", ( msg_colors ? COL_BRIGHT_PURPLE : "" ), ( msg_colors ? COL_RESET : "" ) ); break;
    case DBG_DBG: printf( "[%sDBG%s] ", ( msg_colors ? COL_BRIGHT_GREEN  : "" ), ( msg_colors ? COL_RESET : "" ) ); break;
    default:      printf( "[%sUNK%s] ", ( msg_colors ? COL_BRIGHT_BLACK  : "" ), ( msg_colors ? COL_RESET : "" ) ); break;
  }

  // Printing message
  va_list arglist;
  va_start( arglist, logstr );
  vprintf( logstr, arglist );
  va_end( arglist );

  // Forcing newline
  printf( "\n" );
}



// ~~~~~~~~~~~~~~~~~~~~~ BENCHMARK FUNCTIONS
list* bench_benchmarks = NULL;   // local variable that holds list of single function benchmarks

// Gets difference between two times
struct timespec ts_diff( struct timespec start, struct timespec end){
  struct timespec temp;

  if( (end.tv_nsec - start.tv_nsec) < 0 ){
    temp.tv_sec   = end.tv_sec - start.tv_sec - 1;
    temp.tv_nsec  = TS_NSEC_LIMIT + (end.tv_nsec - start.tv_nsec);
  }
  else{
    temp.tv_sec   = end.tv_sec  - start.tv_sec;
    temp.tv_nsec  = end.tv_nsec - start.tv_nsec;
  }

  return temp;
}

// Return sum between of times
struct timespec ts_sum( struct timespec ts1, struct timespec ts2){
  struct timespec temp;

  if( (ts2.tv_nsec + ts1.tv_nsec) < TS_NSEC_LIMIT ){
    temp.tv_sec   = ts2.tv_sec  + ts1.tv_sec;
    temp.tv_nsec  = ts2.tv_nsec + ts1.tv_nsec;
  }
  else{
    temp.tv_sec   = ts2.tv_sec  + ts1.tv_sec + 1;
    temp.tv_nsec  = (ts2.tv_nsec + ts1.tv_nsec) - TS_NSEC_LIMIT;
  }

  return temp;
}

// Return formatted string for given timespec
void ts_tostr( struct timespec ts, char* dest_str ){
  if( dest_str == NULL ){ return; }

  int h = ts.tv_sec / 3600;
  int m = (ts.tv_sec - h*3600 ) / 60;
  int s = (ts.tv_sec - h*3600 - m*60 );

  int len = 0;

  if( h > 0 ){ sprintf( dest_str + len, "%3dh ", h ); len += 5; }
  if( m > 0 ){ sprintf( dest_str + len, "%2dm ", m ); len += 4; }
  if( s > 0 ){ sprintf( dest_str + len, "%2ds ", s ); len += 4; }
               sprintf( dest_str + len, "%9ldns", ts.tv_nsec );
}
  
// Adds a new function call to the proper list and sets start time
bench_fcall* bench_start( char* fname ){
  char name[ MAX_FN_NAME ] = "";
  strncpy( name, fname, MAX_FN_NAME );

  bench_fcall* fcall = malloc( sizeof( bench_fcall ) );
  clock_gettime( CLOCK_MONOTONIC, &fcall->start );
  fcall->end = fcall->start;

  // if( bench_benchmarks == NULL ){ bench_benchmarks = list_new(); }

  bench_function* fn = NULL;
  for( list *lb = list_get_first( bench_benchmarks ); lb != NULL && lb->element != NULL; lb = lb->next ){
    if( strncmp( name, ((bench_function *)lb->element)->name, MAX_FN_NAME ) == 0 ){ 
      fn = (bench_function *)lb->element;
      break;
    }  // Found list item correspoding to function call name
  }

  // If not found create a new function
  if( fn == NULL ){
    fn = malloc( sizeof( bench_function ) );
    fn->calls = NULL;
    strncpy( fn->name, name, MAX_FN_NAME );
    list_append( &bench_benchmarks, (char *)fn );

    // /*DBG-DELETE*/strncpy( (list_get_last(bench_benchmarks))->name, name, MAX_FN_NAME );
  }

  // Appending function call to proper function calls' list
  list_append( &(fn->calls), (char *)fcall );

  // /*DBG-DELETE*/strncpy( (list_get_last(fn->calls))->name, name, MAX_FN_NAME );

  return fcall;
}

// Sets end time of a function call
void bench_end( bench_fcall* fc ){
  if( fc == NULL ){ return; }
  clock_gettime( CLOCK_MONOTONIC, &fc->end );
}

// Prints benchmarks
void bench_print(){
  struct timespec bck_print_start, bck_print_end;
  clock_gettime( CLOCK_MONOTONIC, &bck_print_start );

  const char tbl_top[]    = "┌──────────────────────────────────────────────────────────────────┬────────┬──────────────────────────┬──────────────────────────┐";
  const char tbl_middle[] = "├──────────────────────────────────────────────────────────────────┼────────┼──────────────────────────┼──────────────────────────┤";
  const char tbl_bottom[] = "└──────────────────────────────────────────────────────────────────┴────────┴──────────────────────────┴──────────────────────────┘";
  // Print benchmark headers
  printf( "\n\n**************************  BENCHMARKS BEGIN **************************\n" );
  printf( "%s\n", tbl_top );
  printf( "│ %-64s │ %-6s │ %-24s │ %-24s │\n", "FUNCTION NAME", "CALLS", "AVG EXEC TIME", "TOTAL EXEC TIME" );
  printf( "%s\n", tbl_middle );

  // Printing every function
  struct timespec sum, avg;
  char sum_str[60] = "",
       avg_str[60] = "";
  int fcall_count = 0;
  bench_function* fn    = NULL;
  bench_fcall*    fcall = NULL;
  
  for( list* bl = list_get_first( bench_benchmarks ); bl != NULL && bl->element != NULL; bl = bl->next ){
    sum.tv_sec   = 0;
    sum.tv_nsec  = 0;
    fcall_count  = 0;
    fn = (bench_function *)bl->element;

    for( list* fcl = list_get_first( fn->calls ); fcl != NULL && fcl->element != NULL; fcl = fcl->next ){
      fcall = (bench_fcall *)fcl->element;
      sum = ts_sum( sum, ts_diff( fcall->start, fcall->end ) );

      // ts_tostr( fcall->start, avg_str );
      // ts_tostr( fcall->end, sum_str );
      // printf( "name: %s - start: %s - end: %s\n", fn->name, avg_str, sum_str );
      
      // ts_tostr( ts_diff( fcall->start, fcall->end ), avg_str );
      // ts_tostr( ts_sum( sum, ts_diff( fcall->start, fcall->end ) ), sum_str );
      // printf( "name: %s - diff: %s - sum: %s\n", fn->name, avg_str, sum_str );
    }

    fcall_count = list_len( fn->calls );
    avg.tv_sec  = sum.tv_sec  / fcall_count;
    avg.tv_nsec = sum.tv_nsec / fcall_count;
    ts_tostr( avg, avg_str );
    ts_tostr( sum, sum_str );

    printf( "│ %-64s │ %6d │ %24s │ %24s │\n", fn->name, fcall_count, avg_str, sum_str );
    if( bl->next != NULL ){ printf( "%s\n", tbl_middle ); }
  }

  printf( "%s\n", tbl_bottom );
  clock_gettime( CLOCK_MONOTONIC, &bck_print_end );
  ts_tostr( ts_diff( bck_print_start, bck_print_end ), sum_str );
  printf( "bench_printf: %s\n", sum_str );
  printf( "**************************   BENCHMARKS END  **************************\n" );
}


// Destroy a bench_fcall
void bench_fcall_destroy( bench_fcall** fc ){
  if( fc == NULL || *fc == NULL ){ return; }
  free( *fc );
  *fc = NULL;
}

// Destroy a bench_function
void bench_function_destroy( bench_function** fn ){
  if( fn == NULL || *fn == NULL ){ return; }
  list_destroy( &((*fn)->calls), &bench_fcall_destroy );
  free( *fn );
  *fn = NULL;
}

// Destroy and releases all memory used
void bench_destroy(){ list_destroy( &bench_benchmarks, &bench_function_destroy ); }