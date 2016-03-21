// 
// 
// Higeco - hmtp
// 
// Higeco's utility to navigate inside a mtp share
// Send/get files to mobile phone
// 
// 16.09.2015 - v0.0.1
// 

#include <string.h>
#include <libmtp.h>
#include <libgen.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "ffutils.h"


// ~~~~~~~~~~~~~~~~~~~~~ DEFINITIONS
#define PTP_GOH_ROOT_PARENT 0xffffffff  // Root directory ID ( stolen from ptp.h )
#define MTP_PATH_LEN        1024        // defines max mtp path lenght
#define MTP_FNAME_LEN       255         // Max filename length

// Errors
#define ERR_NODEVS      1    // No connected devices
#define ERR_PARAM       2    // Invalid parameters
#define ERR_OPENDEV    10    // Error opening device
#define ERR_GETSTRG    20    // Error getting device's storage
#define ERR_EM_PARAM   30    // Error exploreMTP exploring device fs, no parent_tree device or storage set
#define ERR_GMT_FILE   40    // get_mtp_file error, no file
#define ERR_GMT_PATH   41    // get_mtp_file error, NULL save_path or write problem
#define ERR_GMT_GET    42    // get_mtp_file error, file not get
#define ERR_SMT_PATH   50    // send_mtp_file error, NULL save_path or write problem
#define ERR_SMT_GET    51    // send_mtp_file error, file not sent
#define ERR_SMT_DDIR   52    // send_mtp_file error, destination directory doesn't exists
#define ERR_SMT_DDE    53    // send_mtp_file error, Destination Directory exists
#define ERR_SMT_LSTAT  54    // send_mtp_file error, local file stat error, file may not exists or is not readable
#define ERR_SMT_SEND   55    // send_mtp_file error, file not sent
#define ERR_SMT_FEO    56    // send_mtp_file error, file already exists and failed to overwrite
#define ERR_DMT_FILE   60    // del_mtp_file error, no file
#define ERR_DMT_DEL    61    // del_mtp_file error, failed deletion

// Actions ( send, get, delete )
#define ACT_NONE        0    // No action specified, default initialization
#define ACT_SEND        1    // Action Send file
#define ACT_GET         2    // Action Get file
#define ACT_DEL         3    // Action Del file
#define ACT_LIST        4    // Action List file


// ~~~~~~~~~~~~~~~~~~~~~ GLOBAL VARS
int bench   = _FALSE_;    // Enable benchmark
int phelp   = _FALSE_;    // Set's help variable


// ~~~~~~~~~~~~~~~~~~~~~ FUNCTIONS
void help(){
  printf( "hmtp\n" );
  printf( "libmtp version: " LIBMTP_VERSION_STRING "\n\n");
  printf( "Command: \n\t./hmtp <action> <source> <destination> [options]\n\n" );
  printf( "Actions: \n" );
  printf( "\tget:   <source_mtp>   <destination_local>   gets file from mtp device\n" );
  printf( "\tsend:  <source_local> <destination_mtp>     send local file to mtp path\n" );
  printf( "\tdel:   <source_mtp>                         delete mtp path\n" );
  printf( "\tlist:  <source_mtp>                         list mtp file / directory content\n" );
  printf( "\n" );
  printf( "Options: \n" );

  printf( "\t-l, --level:     Sets verbosity level [ error, warning, info, verbose, debug, none ] ( default = info )\n" );
  printf( "\t-b, --benchmark: Enable benchmarks ( default no )\n" );
  printf( "\t-c, --colors:    Set colored output on\n" );
  printf( "\t-d, --devinfo:   Prints device info' and exits\n" );
  printf( "\t-s, --storages:  Prints storages info's and exits\n" );
  printf( "\t-h, --help:      Print help\n" );

  printf( "\n" );
}


// ~~~~~~~~~~~~~~~~~~~~~ MTP FUNCTIONS
int mtp_isdir( tree* tf ){
  return ( tf!= NULL && tf->element != NULL && ((LIBMTP_file_t*)tf->element)->filetype == LIBMTP_FILETYPE_FOLDER ) ? _TRUE_ : _FALSE_ ;
}
int mtp_isfile( tree* tf ){ 
  return  ( tf != NULL && tf->element != NULL && mtp_isdir(tf) == _FALSE_) ? _TRUE_ : _FALSE_;
}

void print_storage_info( LIBMTP_devicestorage_t* storage ){
  if( storage == NULL ){ msg( DBG_WAR, "print_storage_info", "storage is NULL" ); return; }

  printf( "STORAGE: %s-%s\n", storage->VolumeIdentifier, storage->StorageDescription );
  printf( "       Max capacity: %lu\n", storage->MaxCapacity        );
  printf( "         Free space: %lu\n", storage->FreeSpaceInBytes   );
  printf( "          Free objs: %lu\n", storage->FreeSpaceInObjects );
  printf( "       Storage Type: %u\n",  storage->StorageType        );
  printf( "    Filesystem Type: %u\n",  storage->FilesystemType     );
  printf( "  Access Capability: %u\n",  storage->AccessCapability   );
}

void print_dev_info( LIBMTP_mtpdevice_t* dev ){
  if( dev == NULL ){ msg( DBG_WAR, "print_dev_info", "Device is NULL" ); return; }
  printf( "libmtp version: " LIBMTP_VERSION_STRING "\n\n");
  uint8_t battery_max = 0, battery_current = 0;
  LIBMTP_Get_Batterylevel( dev, &battery_max, &battery_current ) ;

  char  *name   = LIBMTP_Get_Friendlyname( dev ),
        *model  = LIBMTP_Get_Modelname( dev ),
        *sn     = LIBMTP_Get_Serialnumber( dev ),
        *manuf  = LIBMTP_Get_Manufacturername( dev ),
        *devver = LIBMTP_Get_Deviceversion( dev );

  printf( "DEVICE INFOS: \n" );
  printf( "          Name: %s\n", name    ); free( name   );
  printf( "         Model: %s\n", model   ); free( model  );
  printf( "    Serial Num: %s\n", sn      ); free( sn     );
  printf( "  Manufacturer: %s\n", manuf   ); free( manuf  );
  printf( "    Device Ver: %s\n", devver  ); free( devver );
  printf( "   Battery Lev: %d/%d\n", battery_current, battery_max );
}

int print_mtp_obj( tree* t ){
  if( t == NULL || t->element == NULL ){ msg( DBG_VER, "print_mtp_obj", "NULL tree" ); return ERR_PARAM; }
  /*BENCHMARK*/ bench_fcall* bck = bench_start( "print_mtp_obj" );
  
  LIBMTP_file_t* f = (LIBMTP_file_t *)t->element;
  printf( "pid: %5u - id: %5u - %15lu B  %s %-65s\n", f->parent_id, f->item_id, f->filesize, (f->filetype == LIBMTP_FILETYPE_FOLDER ? "D" : "F"), f->filename );
  
  /*BENCHMARK*/ bench_end( bck );
  return EXIT_SUCCESS;
}

int print_mtp_dir( tree* t ){
  if( t == NULL ){ msg( DBG_VER, "print_mtp_dir", "NULL tree" ); return ERR_PARAM; }
  if( t->element == NULL && t->root != NULL ){ msg( DBG_VER, "print_mtp_dir", "NULL tree file" ); return ERR_PARAM; }
  /*BENCHMARK*/ bench_fcall* bck = bench_start( "print_mtp_dir" );

  LIBMTP_file_t* el = (LIBMTP_file_t *)t->element;
  printf( " » %s  %d\n", t->root == NULL ? "/" : el->filename, list_len( t->leaves ) );

  if( mtp_isfile( t ) ){ print_mtp_obj( t ); }
  else{
    if( t->leaves == NULL ){ printf( "empty\n" ); }

    for( list* l = list_get_first( t->leaves ); l != NULL; l = l->next ){
      t = (tree *)l->element;
      if( t == NULL || t->element == NULL ){continue; }
      print_mtp_obj( t );
    }
  }

  /*BENCHMARK*/ bench_end( bck );
  return EXIT_SUCCESS;
}

void destroy_mtp_tree( LIBMTP_file_t** mtpf ){
  if( mtpf == NULL || *mtpf == NULL ){ msg( DBG_DBG, "destroy_mtp_tree", "mtpf NULL. Not destroying" ); return; }
  /*BENCHMARK*/ bench_fcall* bck = bench_start( "LIBMTP_destroy_file_t" );
  LIBMTP_destroy_file_t( *mtpf );
  *mtpf = NULL;
  /*BENCHMARK*/ bench_end( bck );
}

int treezer( tree* t, LIBMTP_file_t* flist ){
  if( flist == NULL ){ return EXIT_SUCCESS; }
  if( t == NULL ){ return ERR_PARAM; }

  /*BENCHMARK*/ bench_fcall* bck = bench_start( "treezer" );
  tree *newt = NULL,
       *tmpt = NULL;
  LIBMTP_file_t *tmpf = NULL;

  // Cleaning already in leaves
  tree_empty( t, &destroy_mtp_tree );

  while( flist != NULL ){
    newt = tree_addleaf( t, (char *)flist );
    // /*DBG-DELETE*/strncpy( newt->name, flist->filename, MAX_FN_NAME );
    flist = flist->next; // Going to next file entry
  }

  /*BENCHMARK*/ bench_end( bck );
  return EXIT_SUCCESS;
}


tree* mtp_get_obj( tree* t, char* fname ){
  /*BENCHMARK*/ bench_fcall* bck = bench_start( "mtp_get_obj" );

  LIBMTP_file_t* f  = NULL;
  tree* filefound   = NULL;
  tree* tmptree     = NULL;

  // Support for '.' and '..' special dirs
  if( strncmp( fname, ".",  MTP_FNAME_LEN ) == 0 ){ filefound = t; }
  if( strncmp( fname, "..", MTP_FNAME_LEN ) == 0 && t != NULL ){ filefound = t->root; }

  for( list* flist = list_get_first( t->leaves ); flist != NULL; flist = flist->next ){
    if( flist->element == NULL || ((tree*)flist->element)->element == NULL ){ continue; }
    tmptree = (tree *)flist->element;

    f = (LIBMTP_file_t*)(tmptree->element);
    if( strncmp( fname, f->filename, MTP_FNAME_LEN ) == 0 ){ filefound = tmptree; break; }
  }

  /*BENCHMARK*/ bench_end( bck );
  return filefound;
}

int exploreMTP( tree* parent_tree, LIBMTP_mtpdevice_t* dev, uint32_t mtp_id ){
  if( parent_tree == NULL || dev == NULL || dev->storage == NULL ){
    msg( DBG_WAR, "exploreMTP", "No parent_tree, device or storage to explore" );
    return ERR_EM_PARAM;
  }
  /*BENCHMARK*/ bench_fcall *bck = bench_start( "exploreMTP" );
  
  /*BENCHMARK*/ bench_fcall *bck_tmp = bench_start( "LIBMTP_Get_Files_And_Folders" );
  LIBMTP_file_t* flist = LIBMTP_Get_Files_And_Folders( dev, dev->storage->id, mtp_id );
  /*BENCHMARK*/ bench_end( bck_tmp );

  if( flist != NULL ){ treezer( parent_tree, flist ); }

  /*BENCHMARK*/ bench_end( bck );
  return EXIT_SUCCESS;
}

tree* obj_from_path( LIBMTP_mtpdevice_t* dev, char* mtp_path, tree* parent_tree ){
  /*BENCHMARK*/ bench_fcall* bck = bench_start( "obj_from_path" );

  if( dev == NULL || mtp_path == NULL || parent_tree == NULL ){
    msg( DBG_VER, "obj_from_path", "Wrong params" );
    return NULL;
  }

  // Walking through path
  char path[ MTP_PATH_LEN ];
  strncpy( path, mtp_path, MTP_PATH_LEN ); 
  
  list* pl  = str_split( path, '/' );
  tree* t   = parent_tree;


  for( list* l = list_get_first( pl ); l != NULL; l = l->next ){
    if( l->element == NULL || l->element == '\0'       ||
        strncmp( l->element, "",  MTP_FNAME_LEN ) == 0 ||  
        strncmp( l->element, ".", MTP_FNAME_LEN ) == 0 ){ continue; }
    t = mtp_get_obj( t, l->element );
    if( t == NULL ){ msg( DBG_VER, "obj_from_path", "Not found: '%s'", l->element ); break; }
    
    if( mtp_isfile( t ) ){
      if( l->next != NULL ){
        msg( DBG_VER, "obj_from_path", "Found file but path is not ended. (Should be a directory)" );
        t = NULL;
      }
      break;
    }
    else{
      // Root has element NULL
      t->element != NULL && exploreMTP( t, dev, ((LIBMTP_file_t*)t->element)->item_id );
    }
  }

  // cleaning up
  list_destroy( &pl, NULL );
  /*BENCHMARK*/ bench_end( bck );

  return t;
}

int del_mtp_file( LIBMTP_mtpdevice_t* dev, tree* parent_tree, char* delfile ){
  if( delfile == NULL ){
    msg( DBG_WAR, "del_mtp_file", "NULL file" );
    return ERR_DMT_FILE;
  }
  /*BENCHMARK*/ bench_fcall *bck = bench_start( "del_mtp_file" ), *bck_tmp = NULL;

  tree* df = obj_from_path( dev, delfile, parent_tree );
  if( df == NULL || df->element == NULL ){
    msg( DBG_INF, "del_mtp_file", "No MTP file/dir to delete" );
    return EXIT_SUCCESS;
  }

  /*BENCHMARK*/ bck_tmp = bench_start( "LIBMTP_Delete_Object" );
  int delete_object_status = LIBMTP_Delete_Object( dev, ( (LIBMTP_file_t *)df->element )->item_id );
  /*BENCHMARK*/ bench_end( bck_tmp );
  

  if( delete_object_status != 0 ){ 
    msg( DBG_WAR, "del_mtp_file", "Error deleting mtp file: '%s'", delfile );
    /*BENCHMARK*/ bench_end( bck );
    return ERR_DMT_DEL;
  }
 
  // LIBMTP_file_t *tmpf = (LIBMTP_file_t *)df->element;
  tree_destroy( &df, &destroy_mtp_tree );
  msg( DBG_VER, "del_mtp_file", "File '%s\' deleted", delfile );

  /*BENCHMARK*/ bench_end( bck );
  return EXIT_SUCCESS;
}


int get_mtp_file( LIBMTP_mtpdevice_t* dev, tree* parent_tree, char* source_mtp, char* dest_local ){
  if( source_mtp == NULL || dest_local == NULL ){
    msg( DBG_WAR, "get_mtp_file", "NULL path" );
    return ERR_GMT_PATH;
  }

  /*BENCHMARK*/ bench_fcall *bck = bench_start( "get_mtp_file" ), *bck_tmp = NULL;

  tree* mtp_file = obj_from_path( dev, source_mtp, parent_tree );
  if( mtp_file == NULL || mtp_file->element == NULL ){
    msg( DBG_WAR, "get_mtp_file", "Invalid or NULL file" );
    /*BENCHMARK*/ bench_end( bck );
    return ERR_GMT_FILE;
  }
    
  LIBMTP_file_t* f = (LIBMTP_file_t * )mtp_file->element;
  /*BENCHMARK*/ bck_tmp = bench_start( "LIBMTP_Get_File_To_File" );
  int get_file_status = LIBMTP_Get_File_To_File( dev, f->item_id, dest_local, NULL, NULL );
  /*BENCHMARK*/ bench_end( bck_tmp );
  if( f == NULL || get_file_status != 0 ){ 
    msg( DBG_ERR, "get_mtp_file", "Error getting mtp file: '%s' -> '%s'", source_mtp, dest_local );
    /*BENCHMARK*/ bench_end( bck );
    return ERR_GMT_GET;
  }

  msg( DBG_VER, "get_mtp_file", "File '%s\' saved to '%s'", source_mtp, dest_local );
  /*BENCHMARK*/ bench_end( bck );
  return EXIT_SUCCESS;
}

int send_mtp_file( LIBMTP_mtpdevice_t* dev, tree* parent_tree, char* source_local, char* dest_mtp ){
  if( source_local == NULL || dest_mtp == NULL ){
    msg( DBG_WAR, "send_mtp_file", "NULL path" );
    return ERR_SMT_PATH;
  }
  /*BENCHMARK*/ bench_fcall *bck = bench_start( "send_mtp_file" ), *bck_tmp = NULL;

  char *mtp_fname = basename( dest_mtp ),
       *mtp_path  = dirname( dest_mtp );
  tree *mtp_dir   = obj_from_path( dev, mtp_path, parent_tree );
  
  // Destination directory is NULL
  if( mtp_dir == NULL ){
    msg( DBG_WAR, "send_mtp_file", "Invalid or NULL destination directory" );
    /*BENCHMARK*/ bench_end( bck );
    return ERR_SMT_DDIR;
  }

  // Checking that destination is not a directory
  tree* mtp_dest_file = mtp_get_obj( mtp_dir, mtp_fname );
  LIBMTP_file_t * newF = NULL;

  // If file exists delete it
  if( mtp_dest_file != NULL ){
    if( mtp_isdir( mtp_dest_file ) ){
      msg( DBG_ERR, "send_mtp_file", "Destination mtp file name exists and is a directory" );
      /*BENCHMARK*/ bench_end( bck );
      return ERR_SMT_DDE;
    }

    // if( mtp_isfile( mtp_dest_file ) && del_mtp_file( dev, mtp_dir, mtp_fname ) != 0 ){
    if( mtp_isfile( mtp_dest_file ) && del_mtp_file( dev, mtp_dest_file, "." ) != 0 ){
      msg( DBG_ERR, "send_mtp_file", "File '%s' already exists, failed to overwrite", mtp_fname );
      /*BENCHMARK*/ bench_end( bck );
      return ERR_SMT_FEO;
    }
  }

  // Setting file size
  struct stat sb;
  if( stat( source_local, &sb ) == -1 ){
    msg( DBG_WAR, "send_mtp_file", "Stat error on local file: '%s'", source_local);
    /*BENCHMARK*/ bench_end( bck );
    return ERR_SMT_LSTAT;
  }

  /*BENCHMARK*/ bck_tmp = bench_start( "LIBMTP_new_file_t" );
  newF = LIBMTP_new_file_t();
  /*BENCHMARK*/ bench_end( bck_tmp );
  newF->filename    = strdup( mtp_fname );
  newF->filesize    = sb.st_size;
  newF->parent_id   =  mtp_dir->element == NULL ? PTP_GOH_ROOT_PARENT : ((LIBMTP_file_t *)mtp_dir->element)->item_id;
  newF->storage_id  = dev->storage->id;
  // newF->filetype = find_filetype( source_local );

  // Sending File
  /*BENCHMARK*/ bck_tmp = bench_start( "LIBMTP_Send_File_From_File" );
  int send_file_status = LIBMTP_Send_File_From_File( dev, source_local, newF, NULL, NULL );
  /*BENCHMARK*/ bench_end( bck_tmp );
  if( send_file_status != 0 ){ 
    msg( DBG_WAR, "send_mtp_file", "Error sending local file: '%s' -> '%s'", source_local, mtp_fname );
    /*BENCHMARK*/ bench_end( bck );
    return ERR_SMT_SEND;
  }

  // Adding file to tree structure
  mtp_dest_file = tree_addleaf( mtp_dir, (char *)newF );
 
  // /*DBG-DELETE*/strncpy( mtp_dest_file->name, newF->filename, MAX_FN_NAME );

  msg( DBG_VER, "send_mtp_file", "File '%s\' saved to '%s'", source_local, mtp_fname );
  /*BENCHMARK*/ bench_end( bck );
  return EXIT_SUCCESS;
}

void htmp_before_exit( LIBMTP_mtpdevice_t** dev, tree** fstree, bench_fcall* bck_main ){
  // Cleaning the toilette
  if( fstree != NULL && *fstree != NULL ){ tree_destroy( fstree, &destroy_mtp_tree ); msg( DBG_DBG, "mtp_before_exit", "fstree freed" ); }
  else{ msg( DBG_DBG, "mtp_before_exit", "fstree NULL. Not freed" ); }


  if( dev != NULL && *dev != NULL ){
    msg( DBG_DBG, "mtp_before_exit", "Releasing device" );
    /*BENCHMARK*/ bench_fcall* bck_tmp = bench_start( "LIBMTP_Release_Device" );
    LIBMTP_Release_Device( *dev );
    *dev = NULL;
    /*BENCHMARK*/ bench_end( bck_tmp );
  }
  else{ msg( DBG_DBG, "mtp_before_exit", "Device NULL. Not released" ); }

  // Printing benches
  /*BENCHMARK*/ bench_end( bck_main );
  if( bench ){ bench_print(); }
  bench_destroy();
}


// ~~~~~~~~~~~~~~~~~~~~~ MAIN CODE
int main( int argc, char **argv ){
  /*BENCHMARK*/ bench_fcall *bck_main = bench_start( "main" ), *bck_tmp  = NULL;

  int   exit_status   = EXIT_SUCCESS;   // hmtp exit status
  tree* fstree        = NULL;           // Will hold fs structure

  int   action        = ACT_NONE;       // Specify action to do
  int   info_storages = _FALSE_;        // Parameters to print storage info
  int   info_dev      = _FALSE_;        // Parameters to print storage info
  
  char source[ MTP_PATH_LEN ] = "",     // Source path
       dest[ MTP_PATH_LEN ]   = "";     // Destination path

  LIBMTP_raw_device_t*  devlist = NULL; // List of devices
  LIBMTP_mtpdevice_t*   dev     = NULL; // Opened device
  int ndevs = 0;                        // Number of connected devices

  // BEGIN: Command line args
    // Available cmdline options
    // send:            send file to mtp device
    // get:             get file from mtp device
    // list:            list directory / file info
    // del,delete:      get file from mtp device
    // 
    // -l, --level:     Sets verbosity level [ error, warning, info, verbose, debug, none ]
    // -b, --benchmark: Enable benchmarks
    // -c, --colors:    set colored output on
    // -d, --devinfo:   prints device info' and exits
    // -s, --storages:  prints storages info's and exits
    // -h, --help:      print help
    // default option (no arg specified, argc==1): -h
  for( int i = 1; i < argc; i++ ){
    if(       strcmp( argv[i], "-h" ) == 0 || strcmp( argv[i], "--help"       ) == 0 ){ phelp = _TRUE_;           }
    else if(  strcmp( argv[i], "-c" ) == 0 || strcmp( argv[i], "--colors"     ) == 0 ){ set_msg_colors( _TRUE_ ); }
    else if(  strcmp( argv[i], "-d" ) == 0 || strcmp( argv[i], "--devinfo"    ) == 0 ){ info_dev = _TRUE_;        }
    else if(  strcmp( argv[i], "-s" ) == 0 || strcmp( argv[i], "--storages"   ) == 0 ){ info_storages = _TRUE_;   }
    else if(  strcmp( argv[i], "-b" ) == 0 || strcmp( argv[i], "--benchmark"  ) == 0 ){ bench = _TRUE_;           }
    else if( (strcmp( argv[i], "-l" ) == 0 || strcmp( argv[i], "--level"      ) == 0 ) && (i+1)<argc ){
      i++;
      if(       strcmp( argv[i], "error"   ) == 0 ){ set_debug( DBG_ERR   ); }
      else if(  strcmp( argv[i], "warning" ) == 0 ){ set_debug( DBG_WAR   ); }
      else if(  strcmp( argv[i], "info"    ) == 0 ){ set_debug( DBG_INF   ); }
      else if(  strcmp( argv[i], "verbose" ) == 0 ){ set_debug( DBG_VER   ); }
      else if(  strcmp( argv[i], "debug"   ) == 0 ){ set_debug( DBG_DBG   ); }
      else if(  strcmp( argv[i], "none"    ) == 0 ){ set_debug( DBG_NONE  ); }
    }
    else if( strcmp(argv[i], "send") == 0 ){
      if( (i+2) >= argc ){ msg( DBG_ERR, "main", "Missing parameters" ); return ERR_PARAM; }
      if( action != ACT_NONE ){ msg( DBG_ERR, "main", "Multiple actions specified" ); return ERR_PARAM; }
      action = ACT_SEND;
      strncpy( source, argv[++i], MTP_PATH_LEN );
      strncpy( dest,   argv[++i], MTP_PATH_LEN );
    }
    else if( strcmp(argv[i], "get") == 0 ){
      if( (i+2) >= argc ){ msg( DBG_ERR, "main", "Missing parameters" ); return ERR_PARAM; }
      if( action != ACT_NONE ){ msg( DBG_ERR, "main", "Multiple actions specified" ); return ERR_PARAM; }
      action = ACT_GET;
      strncpy( source, argv[++i], MTP_PATH_LEN );
      strncpy( dest,   argv[++i], MTP_PATH_LEN );
    }
    else if( strcmp(argv[i], "del") == 0 || strcmp(argv[i], "delete") == 0 ){
      if( (i+1) >= argc ){ msg( DBG_ERR, "main", "Missing parameters" ); return ERR_PARAM; }
      if( action != ACT_NONE ){ msg( DBG_ERR, "main", "Multiple actions specified" ); return ERR_PARAM; }
      action = ACT_DEL;
      strncpy( source, argv[++i], MTP_PATH_LEN );
    }
    else if( strcmp(argv[i], "list") == 0 ){
      if( (i+1) >= argc ){ msg( DBG_ERR, "main", "Missing parameters" ); return ERR_PARAM; }
      if( action != ACT_NONE ){ msg( DBG_ERR, "main", "Multiple actions specified" ); return ERR_PARAM; }
      action = ACT_LIST;
      strncpy( source, argv[++i], MTP_PATH_LEN );
    }
    else{ msg( DBG_WAR, "main", "Unknown argument: '%s'", argv[i] ); }
  }

  // Check params
  if( argc < 2 || phelp ){
    help();
    htmp_before_exit( &dev, NULL, bck_main );
    return EXIT_SUCCESS;
  }
  if( action == ACT_NONE && !( info_dev || info_storages ) ){
    msg( DBG_WAR, "main", "No action specified" );
    htmp_before_exit( &dev, NULL, bck_main );
    return ERR_PARAM;
  }

  if( get_debug() <= DBG_DBG ){
    msg( DBG_DBG, "main", "┌───── PARAMS" );
    msg( DBG_DBG, "main", "├─ action: %d", action      );
    msg( DBG_DBG, "main", "├─ source: %s", source      );
    msg( DBG_DBG, "main", "├─ dest:   %s", dest        );
    msg( DBG_DBG, "main", "├─ dbgl:   %d", get_debug() );
    msg( DBG_DBG, "main", "├─ colors: %s", is_msg_colors() ? (COL_BRIGHT_GREEN "on " COL_RESET) : "off" );
    msg( DBG_DBG, "main", "└───── PARAMS" );
  }
  // END: Command line args

  /*BENCHMARK*/ bck_tmp = bench_start( "LIBMTP_Init" );
  LIBMTP_Init();
  /*BENCHMARK*/ bench_end( bck_tmp );

  /*BENCHMARK*/ bck_tmp = bench_start( "LIBMTP_Detect_Raw_Devices" );
  int detect_raw_dev_status = LIBMTP_Detect_Raw_Devices( &devlist, &ndevs );
  /*BENCHMARK*/ bench_end( bck_tmp );
  switch( detect_raw_dev_status ){
    case LIBMTP_ERROR_MEMORY_ALLOCATION:
      msg( DBG_VER, "main", "ERROR: LIBMTP_ERROR_MEMORY_ALLOCATION" );
      break;

    case LIBMTP_ERROR_NO_DEVICE_ATTACHED:
      msg( DBG_VER, "main", "ERROR: No devices" );
      break;

    case LIBMTP_ERROR_NONE :
      msg( DBG_DBG, "main", "LIBMTP_ERROR_NONE - This means everything is ok" );
      break;

    default:
      msg( DBG_VER, "main", "Unknown error" );
      break;
  }

  // Opening Raw device
  if( ndevs < 1 ){
    msg( DBG_WAR, "main", "No devices connected" );
    htmp_before_exit( &dev, NULL, bck_main );
    return ERR_NODEVS;
  }

  /*BENCHMARK*/ bck_tmp = bench_start( "LIBMTP_Open_Raw_Device_Uncached" );
  dev = LIBMTP_Open_Raw_Device_Uncached( devlist );
  /*BENCHMARK*/ bench_end( bck_tmp );
  // devlist no longer needed, freeing it
  free( devlist );
  if( dev == NULL ){
    msg( DBG_ERR, "main", "Raw Device not opened" );
    htmp_before_exit( &dev, NULL, bck_main );
    return ERR_OPENDEV;
  }

  // Print Dev infos
  if( info_dev  ){ 
    print_dev_info( dev );
    htmp_before_exit( &dev, NULL, bck_main );
    return EXIT_SUCCESS;
  }

  // Opening storage 
  /*BENCHMARK*/ bck_tmp = bench_start( "LIBMTP_Get_Storage" );
  int get_storage_status = LIBMTP_Get_Storage( dev, LIBMTP_STORAGE_SORTBY_NOTSORTED );
  /*BENCHMARK*/ bench_end( bck_tmp );
  if( get_storage_status == -1 || dev->storage == NULL ){
    msg( DBG_ERR, "main", "Failed getting MTP storage" );
    htmp_before_exit( &dev, NULL, bck_main );
    return ERR_GETSTRG;
  }

  // Print Storages infos
  if( info_storages ){ 
    for( LIBMTP_devicestorage_t* tmp_strg = dev->storage; tmp_strg != NULL; tmp_strg = tmp_strg->next ){ print_storage_info( tmp_strg ); }
    htmp_before_exit( &dev, NULL, bck_main );
    return EXIT_SUCCESS;
  }

  // If SD card is present internal storage is listed as second storage, or at least it seems so.....
  if( dev->storage->next != NULL ){ dev->storage = dev->storage->next; }

  // Exploring root
  fstree = tree_new();
  // /*DBG-DELETE*/strncpy( fstree->name, "/", MAX_FN_NAME );
  exploreMTP( fstree, dev, PTP_GOH_ROOT_PARENT );
 
  // 
  // ACTIONS
  // 
  char  src[ MTP_PATH_LEN ] = "",
        dst[ MTP_PATH_LEN ] = "";
  
  switch( action ){
    case ACT_GET:
      strncpy( src, source, MTP_PATH_LEN);
      strncpy( dst, dest, MTP_PATH_LEN );
      
      exit_status = get_mtp_file( dev, fstree, src, dst );
      if( exit_status == EXIT_SUCCESS ){ msg( DBG_INF, "main", "Successfully get '%s' -> '%s'", source, dest ); }
      else{ msg( DBG_ERR, "main", "Failed getting '%s' -> '%s'", source, dest ); }
      break;

    case ACT_SEND:
      strncpy( src, source, MTP_PATH_LEN );
      strncpy( dst, dest, MTP_PATH_LEN );
      
      exit_status = send_mtp_file( dev, fstree, src, dst );
      if( exit_status == EXIT_SUCCESS ){ msg( DBG_INF, "main", "Successfully sent '%s' -> '%s'", source, dest ); }
      else{ msg( DBG_ERR, "main", "Failed sending '%s' -> '%s'", source, dest ); }
      break;

    case ACT_DEL:
      strncpy( src, source, MTP_PATH_LEN );

      exit_status = del_mtp_file( dev, fstree, src );
      if( exit_status == EXIT_SUCCESS ){ msg( DBG_INF, "main", "Successfully deleted '%s'", source ); }
      else{ msg( DBG_ERR, "main", "Failed deleting '%s'", source ); }

      break;

    case ACT_LIST:
      strncpy( src, source, MTP_PATH_LEN );
      
      tree* ts = obj_from_path( dev, src, fstree );
      exit_status = ( ts->root == NULL || mtp_isdir(ts) ? print_mtp_dir(ts) : print_mtp_obj(ts) ) ;
      if( exit_status == EXIT_SUCCESS ){ msg( DBG_INF, "main", "Successfully listed '%s'", source ); }
      else{ msg( DBG_ERR, "main", "Failed listing '%s'", source ); }
  
      break;

    default:
      msg( DBG_WAR, "main", "Unknown action %d", action );
      exit_status = ERR_PARAM;
      break;
  }

  htmp_before_exit( &dev, &fstree, bck_main );
  return exit_status;
}
